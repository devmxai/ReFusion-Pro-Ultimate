#include "refusion/adapters/skia/SkiaGpuContexts.hpp"

#if defined(REFUSION_SKIA_APPLE_MEDIA)
#include "refusion/platform/apple/AppleMediaSurface.hpp"
#endif

#import <Metal/Metal.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkSurface.h"
#include "include/core/SkYUVAInfo.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/GrYUVABackendTextures.h"
#include "include/gpu/ganesh/SkImageGanesh.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendSurface.h"
#include "include/gpu/ganesh/mtl/GrMtlDirectContext.h"
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/mtl/MtlBackendContext.h"
#include "include/ports/SkFontMgr_mac_ct.h"
#include "modules/skshaper/include/SkShaper.h"
#include "modules/skshaper/include/SkShaper_harfbuzz.h"
#include "modules/skunicode/include/SkUnicode_icu.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::adapters::skia {

struct SkiaGpuContexts::Implementation final {
  runtime::gpu::DeviceLease lease;
  sk_sp<GrDirectContext> ganesh;
  std::unique_ptr<skgpu::graphite::Context> graphite;
  sk_sp<SkFontMgr> font_manager;
  std::unique_ptr<SkShaper> shaper;
  core::CompositionSnapshot composition;
  std::shared_ptr<const runtime::media::NativeVideoSurfaceLease> decoded_video_fixture;
  sk_sp<SkImage> decoded_video_image;
};

namespace {

using runtime::presentation::FixtureFrame;
using runtime::presentation::FrameResult;
using runtime::presentation::FrameStatus;
using runtime::presentation::NativeFrameTarget;
using runtime::presentation::PixelFormat;

void draw_shaped_line(SkCanvas &canvas, SkShaper &shaper, const char *text, const SkFont &font,
                      const bool left_to_right, const SkPoint origin, const float width,
                      const SkPaint &paint) {
  SkTextBlobBuilderRunHandler handler(text, origin);
  shaper.shape(text, std::strlen(text), font, left_to_right, width, &handler);
  auto blob = handler.makeBlob();
  if (blob) {
    canvas.drawTextBlob(blob, 0.0F, 0.0F, paint);
  }
}

[[nodiscard]] SkColor to_sk_color(const core::ColorRgba8 &color, const double opacity) {
  const auto alpha =
      static_cast<std::uint8_t>(std::clamp(static_cast<double>(color.alpha) * opacity, 0.0, 255.0));
  return SkColorSetARGB(alpha, color.red, color.green, color.blue);
}

void draw_project(SkCanvas &canvas, SkShaper &shaper, SkFontMgr &font_manager,
                  const core::CompositionSnapshot &composition, const FixtureFrame &frame,
                  const float target_width, const float target_height) {
  const float scale_x = target_width / static_cast<float>(composition.canvas.width_pixels);
  const float scale_y = target_height / static_cast<float>(composition.canvas.height_pixels);
  canvas.save();
  canvas.scale(scale_x, scale_y);

  for (const auto &layer : composition.layers) {
    if (!layer.active_range.contains(frame.presentation_time_ns)) {
      continue;
    }
    const double position_x = core::evaluate_animated_property(
        layer, core::AnimatedProperty::position_x, frame.presentation_time_ns);
    const double position_y = core::evaluate_animated_property(
        layer, core::AnimatedProperty::position_y, frame.presentation_time_ns);
    const double layer_scale_x = core::evaluate_animated_property(
        layer, core::AnimatedProperty::scale_x, frame.presentation_time_ns);
    const double layer_scale_y = core::evaluate_animated_property(
        layer, core::AnimatedProperty::scale_y, frame.presentation_time_ns);
    const double rotation = core::evaluate_animated_property(
        layer, core::AnimatedProperty::rotation_degrees, frame.presentation_time_ns);
    const double opacity = core::evaluate_animated_property(layer, core::AnimatedProperty::opacity,
                                                            frame.presentation_time_ns);

    canvas.save();
    canvas.translate(static_cast<float>(position_x), static_cast<float>(position_y));
    canvas.rotate(static_cast<float>(rotation));
    canvas.scale(static_cast<float>(layer_scale_x), static_cast<float>(layer_scale_y));

    SkPaint paint;
    paint.setAntiAlias(true);
    if (const auto *shape = std::get_if<core::ShapeLayerContent>(&layer.content)) {
      paint.setColor(to_sk_color(shape->fill, opacity));
      const auto rect = SkRect::MakeXYWH(
          static_cast<float>(-shape->width * 0.5), static_cast<float>(-shape->height * 0.5),
          static_cast<float>(shape->width), static_cast<float>(shape->height));
      canvas.drawRRect(SkRRect::MakeRectXY(rect, static_cast<float>(shape->corner_radius),
                                           static_cast<float>(shape->corner_radius)),
                       paint);
    } else if (const auto *text = std::get_if<core::TextLayerContent>(&layer.content)) {
      paint.setColor(to_sk_color(text->fill, opacity));
      auto typeface =
          font_manager.matchFamilyStyle(text->font_family.c_str(), SkFontStyle::Normal());
      SkFont font(typeface, static_cast<float>(text->font_size));
      font.setEdging(SkFont::Edging::kAntiAlias);
      draw_shaped_line(canvas, shaper, text->text.c_str(), font, text->left_to_right,
                       SkPoint::Make(static_cast<float>(-text->layout_width * 0.5), 0.0F),
                       static_cast<float>(text->layout_width), paint);
    }
    canvas.restore();
  }
  canvas.restore();
}

void draw_decoded_video_fixture(SkCanvas &canvas, const SkImage &image, const float target_width,
                                const float target_height) {
  const float scale = std::min(target_width / static_cast<float>(image.width()),
                               target_height / static_cast<float>(image.height()));
  const float width = static_cast<float>(image.width()) * scale;
  const float height = static_cast<float>(image.height()) * scale;
  const auto destination = SkRect::MakeXYWH((target_width - width) * 0.5F,
                                            (target_height - height) * 0.5F, width, height);
  canvas.drawImageRect(&image, destination, SkSamplingOptions(SkFilterMode::kLinear), nullptr);
}

#if defined(REFUSION_SKIA_APPLE_MEDIA)
[[nodiscard]] sk_sp<SkImage> make_decoded_video_image(
    GrDirectContext &context,
    const std::shared_ptr<const runtime::media::NativeVideoSurfaceLease> &decoded_video_fixture,
    const runtime::gpu::DeviceIdentity &device_identity) {
  auto native_view = refusion::platform::apple::borrow_metal_video_surface(decoded_video_fixture);
  if (!native_view || !native_view->valid() ||
      decoded_video_fixture->info().device.adapter_id != device_identity.adapter_id ||
      decoded_video_fixture->info().device.generation != device_identity.generation) {
    throw std::invalid_argument(
        "Decoded video fixture does not belong to the Skia Metal generation");
  }

  id<MTLTexture> luma_texture =
      (__bridge id<MTLTexture>)(reinterpret_cast<void *>(native_view->luma_texture));
  id<MTLTexture> chroma_texture =
      (__bridge id<MTLTexture>)(reinterpret_cast<void *>(native_view->chroma_texture));
  if (luma_texture == nil || chroma_texture == nil ||
      luma_texture.pixelFormat != MTLPixelFormatR8Unorm ||
      chroma_texture.pixelFormat != MTLPixelFormatRG8Unorm ||
      static_cast<std::uint64_t>(luma_texture.device.registryID) != device_identity.adapter_id ||
      static_cast<std::uint64_t>(chroma_texture.device.registryID) != device_identity.adapter_id) {
    throw std::invalid_argument("Decoded video fixture exposed incompatible Metal texture planes");
  }

  std::array<GrBackendTexture, SkYUVAInfo::kMaxPlanes> textures;
  GrMtlTextureInfo luma_info;
  luma_info.fTexture.retain((__bridge GrMTLHandle)luma_texture);
  textures[0] = GrBackendTextures::MakeMtl(
      static_cast<int>(native_view->luma_width), static_cast<int>(native_view->luma_height),
      skgpu::Mipmapped::kNo, luma_info, "ReFusion decoded luma");
  GrMtlTextureInfo chroma_info;
  chroma_info.fTexture.retain((__bridge GrMTLHandle)chroma_texture);
  textures[1] = GrBackendTextures::MakeMtl(
      static_cast<int>(native_view->chroma_width), static_cast<int>(native_view->chroma_height),
      skgpu::Mipmapped::kNo, chroma_info, "ReFusion decoded chroma");

  const auto &profile = decoded_video_fixture->info().profile;
  const SkYUVAInfo yuva_info(
      SkISize::Make(static_cast<int>(profile.coded_width), static_cast<int>(profile.coded_height)),
      SkYUVAInfo::PlaneConfig::kY_UV, SkYUVAInfo::Subsampling::k420,
      kRec709_Limited_SkYUVColorSpace);
  const GrYUVABackendTextures yuva_textures(yuva_info, textures.data(), kTopLeft_GrSurfaceOrigin);
  if (!yuva_textures.isValid()) {
    throw std::runtime_error("Skia rejected the decoded NV12 plane layout");
  }
  auto image = SkImages::TextureFromYUVATextures(
      &context, yuva_textures,
      SkColorSpace::MakeRGB(SkNamedTransferFn::kRec709, SkNamedGamut::kSRGB));
  if (!image) {
    throw std::runtime_error("Skia could not wrap the decoded NV12 Metal surface");
  }
  return image;
}
#endif

}  // namespace

SkiaGpuContexts::SkiaGpuContexts(std::unique_ptr<Implementation> implementation)
    : implementation_(std::move(implementation)) {}

SkiaGpuContexts::~SkiaGpuContexts() = default;

std::unique_ptr<SkiaGpuContexts> SkiaGpuContexts::create(
    runtime::gpu::DeviceLease lease, core::CompositionSnapshot composition,
    std::shared_ptr<const runtime::media::NativeVideoSurfaceLease> decoded_video_fixture) {
  if (!lease.valid() || lease.identity().backend != runtime::gpu::Backend::metal) {
    throw std::invalid_argument("Skia Metal contexts require a valid Metal device lease");
  }
  const auto validation = core::validate_composition(composition);
  if (!validation.valid) {
    throw std::invalid_argument(validation.code + ": " + validation.message);
  }

  const auto handles = lease.native_handles();
  id<MTLDevice> device = (__bridge id<MTLDevice>)(reinterpret_cast<void *>(handles.device));
  id<MTLCommandQueue> command_queue =
      (__bridge id<MTLCommandQueue>)(reinterpret_cast<void *>(handles.command_queue));

  GrMtlBackendContext ganesh_backend;
  ganesh_backend.fDevice.retain((__bridge GrMTLHandle)device);
  ganesh_backend.fQueue.retain((__bridge GrMTLHandle)command_queue);
  auto ganesh = GrDirectContexts::MakeMetal(ganesh_backend);
  if (!ganesh) {
    throw std::runtime_error("Skia Ganesh rejected the engine-owned Metal device");
  }

  skgpu::graphite::MtlBackendContext graphite_backend;
  graphite_backend.fDevice.retain((__bridge CFTypeRef)device);
  graphite_backend.fQueue.retain((__bridge CFTypeRef)command_queue);
  skgpu::graphite::ContextOptions graphite_options;
  auto graphite = skgpu::graphite::ContextFactory::MakeMetal(graphite_backend, graphite_options);
  if (!graphite) {
    throw std::runtime_error("Skia Graphite rejected the engine-owned Metal device");
  }

  auto font_manager = SkFontMgr_New_CoreText(nullptr);
  auto unicode = SkUnicodes::ICU::Make();
  auto shaper = SkShapers::HB::ShaperDrivenWrapper(unicode, font_manager);
  if (!font_manager || !unicode || !shaper) {
    throw std::runtime_error("Skia failed to create the portable text shaping stack");
  }

  sk_sp<SkImage> decoded_video_image;
  if (decoded_video_fixture) {
#if defined(REFUSION_SKIA_APPLE_MEDIA)
    decoded_video_image =
        make_decoded_video_image(*ganesh, decoded_video_fixture, lease.identity());
#else
    throw std::invalid_argument("This Skia build has no admitted native video-surface bridge");
#endif
  }

  return std::unique_ptr<SkiaGpuContexts>(
      new SkiaGpuContexts(std::make_unique<Implementation>(Implementation{
          .lease = std::move(lease),
          .ganesh = std::move(ganesh),
          .graphite = std::move(graphite),
          .font_manager = std::move(font_manager),
          .shaper = std::move(shaper),
          .composition = std::move(composition),
          .decoded_video_fixture = std::move(decoded_video_fixture),
          .decoded_video_image = std::move(decoded_video_image),
      })));
}

bool SkiaGpuContexts::ganesh_ready() const noexcept {
  return implementation_ && static_cast<bool>(implementation_->ganesh);
}

bool SkiaGpuContexts::graphite_ready() const noexcept {
  return implementation_ && static_cast<bool>(implementation_->graphite);
}

const runtime::gpu::DeviceIdentity &SkiaGpuContexts::device_identity() const noexcept {
  return implementation_->lease.identity();
}

runtime::presentation::FrameResult SkiaGpuContexts::render(
    const runtime::presentation::NativeFrameTarget &target,
    const runtime::presentation::FixtureFrame &frame) {
  if (!implementation_ || !target.valid() || target.backend != runtime::gpu::Backend::metal ||
      target.pixel_format != PixelFormat::bgra8_unorm ||
      target.device_generation != implementation_->lease.identity().generation) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = "Skia rejected an incompatible viewport render target",
    };
  }

  id<MTLTexture> texture = (__bridge id<MTLTexture>)(reinterpret_cast<void *>(target.texture));
  const auto handles = implementation_->lease.native_handles();
  id<MTLDevice> expected_device =
      (__bridge id<MTLDevice>)(reinterpret_cast<void *>(handles.device));
  if (texture == nil || texture.device != expected_device ||
      texture.pixelFormat != MTLPixelFormatBGRA8Unorm) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = "Skia target does not belong to the engine Metal device",
    };
  }

  GrMtlTextureInfo texture_info;
  texture_info.fTexture.retain((__bridge GrMTLHandle)texture);
  const auto backend_target = GrBackendRenderTargets::MakeMtl(
      static_cast<int>(target.width_pixels), static_cast<int>(target.height_pixels), texture_info);
  auto surface = SkSurfaces::WrapBackendRenderTarget(
      implementation_->ganesh.get(), backend_target, kTopLeft_GrSurfaceOrigin,
      kBGRA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
  if (!surface) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = "Skia could not wrap the CAMetalLayer drawable texture",
    };
  }

  auto &canvas = *surface->getCanvas();
  canvas.clear(SK_ColorBLACK);
  draw_project(canvas, *implementation_->shaper, *implementation_->font_manager,
               implementation_->composition, frame, static_cast<float>(target.width_pixels),
               static_cast<float>(target.height_pixels));
  if (implementation_->decoded_video_image) {
    draw_decoded_video_fixture(canvas, *implementation_->decoded_video_image,
                               static_cast<float>(target.width_pixels),
                               static_cast<float>(target.height_pixels));
  }
  implementation_->ganesh->flushAndSubmit(surface.get(), GrSyncCpu::kNo);
  if (implementation_->ganesh->abandoned()) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = "Skia Metal context was abandoned during viewport submission",
    };
  }
  return FrameResult{.status = FrameStatus::presented};
}

}  // namespace refusion::adapters::skia
