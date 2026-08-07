#include "refusion/adapters/skia/SkiaGpuContexts.hpp"

#import <Metal/Metal.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
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
#include <cmath>
#include <cstring>
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
};

namespace {

using runtime::presentation::FixtureFrame;
using runtime::presentation::FrameResult;
using runtime::presentation::FrameStatus;
using runtime::presentation::NativeFrameTarget;
using runtime::presentation::PixelFormat;

void draw_shaped_line(SkCanvas& canvas,
                      SkShaper& shaper,
                      const char* text,
                      const SkFont& font,
                      const bool left_to_right,
                      const SkPoint origin,
                      const float width,
                      const SkPaint& paint) {
  SkTextBlobBuilderRunHandler handler(text, origin);
  shaper.shape(text,
               std::strlen(text),
               font,
               left_to_right,
               width,
               &handler);
  auto blob = handler.makeBlob();
  if (blob) {
    canvas.drawTextBlob(blob, 0.0F, 0.0F, paint);
  }
}

void draw_fixture(SkCanvas& canvas,
                  SkShaper& shaper,
                  SkFontMgr& font_manager,
                  const FixtureFrame& frame,
                  const float width,
                  const float height) {
  canvas.clear(SkColorSetARGB(255, 5, 6, 10));

  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(SkColorSetARGB(255, 17, 20, 29));
  const float margin = std::max(24.0F, width * 0.055F);
  const SkRect panel_rect = SkRect::MakeLTRB(
      margin, margin, width - margin, height - margin);
  canvas.drawRRect(SkRRect::MakeRectXY(panel_rect, 28.0F, 28.0F), panel);

  SkPaint accent;
  accent.setAntiAlias(true);
  accent.setColor(SkColorSetARGB(255, 124, 92, 255));
  const float seconds = static_cast<float>(frame.presentation_time_ns) / 1'000'000'000.0F;
  const float motion = 0.5F + 0.5F * std::sin(seconds * 1.5F);
  const float shape_size = std::clamp(width * 0.12F, 72.0F, 150.0F);
  const float shape_x = margin + 42.0F + motion * std::max(0.0F, width * 0.12F);
  const float shape_y = height * 0.5F - shape_size * 0.5F;
  canvas.drawRRect(
      SkRRect::MakeRectXY(
          SkRect::MakeXYWH(shape_x, shape_y, shape_size, shape_size),
          shape_size * 0.24F,
          shape_size * 0.24F),
      accent);

  SkPaint glow;
  glow.setAntiAlias(true);
  glow.setColor(SkColorSetARGB(80, 72, 211, 255));
  canvas.drawCircle(shape_x + shape_size * 0.72F,
                    shape_y + shape_size * 0.28F,
                    shape_size * 0.18F,
                    glow);

  auto typeface = font_manager.matchFamilyStyle(
      "Arial", SkFontStyle::Normal());
  SkFont title_font(typeface, std::clamp(width * 0.052F, 34.0F, 62.0F));
  title_font.setEdging(SkFont::Edging::kAntiAlias);
  SkFont arabic_font(typeface, std::clamp(width * 0.036F, 28.0F, 46.0F));
  arabic_font.setEdging(SkFont::Edging::kAntiAlias);

  SkPaint title_paint;
  title_paint.setAntiAlias(true);
  title_paint.setColor(SkColorSetARGB(255, 242, 244, 248));
  const float text_x = std::max(width * 0.42F, shape_x + shape_size + 42.0F);
  draw_shaped_line(canvas,
                   shaper,
                   "ReFusion",
                   title_font,
                   true,
                   SkPoint::Make(text_x, height * 0.45F),
                   width - text_x - margin,
                   title_paint);

  SkPaint arabic_paint;
  arabic_paint.setAntiAlias(true);
  arabic_paint.setColor(SkColorSetARGB(255, 159, 168, 190));
  draw_shaped_line(canvas,
                   shaper,
                   "استوديو فيديو يعمل بالمحرك",
                   arabic_font,
                   false,
                   SkPoint::Make(text_x, height * 0.55F),
                   width - text_x - margin,
                   arabic_paint);

  SkPaint rail;
  rail.setAntiAlias(true);
  rail.setColor(SkColorSetARGB(255, 42, 48, 64));
  const SkRect rail_rect = SkRect::MakeXYWH(
      margin + 36.0F, height - margin - 42.0F, width - (margin + 36.0F) * 2.0F, 6.0F);
  canvas.drawRRect(SkRRect::MakeRectXY(rail_rect, 3.0F, 3.0F), rail);

  SkPaint progress;
  progress.setAntiAlias(true);
  progress.setColor(SkColorSetARGB(255, 72, 211, 255));
  canvas.drawRRect(
      SkRRect::MakeRectXY(
          SkRect::MakeXYWH(rail_rect.x(), rail_rect.y(), rail_rect.width() * motion, 6.0F),
          3.0F,
          3.0F),
      progress);
}

}  // namespace

SkiaGpuContexts::SkiaGpuContexts(std::unique_ptr<Implementation> implementation)
    : implementation_(std::move(implementation)) {}

SkiaGpuContexts::~SkiaGpuContexts() = default;

std::unique_ptr<SkiaGpuContexts> SkiaGpuContexts::create(
    runtime::gpu::DeviceLease lease) {
  if (!lease.valid() || lease.identity().backend != runtime::gpu::Backend::metal) {
    throw std::invalid_argument("Skia Metal contexts require a valid Metal device lease");
  }

  const auto handles = lease.native_handles();
  id<MTLDevice> device = (__bridge id<MTLDevice>)(
      reinterpret_cast<void*>(handles.device));
  id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)(
      reinterpret_cast<void*>(handles.command_queue));

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
  auto graphite = skgpu::graphite::ContextFactory::MakeMetal(
      graphite_backend, graphite_options);
  if (!graphite) {
    throw std::runtime_error("Skia Graphite rejected the engine-owned Metal device");
  }

  auto font_manager = SkFontMgr_New_CoreText(nullptr);
  auto unicode = SkUnicodes::ICU::Make();
  auto shaper = SkShapers::HB::ShaperDrivenWrapper(unicode, font_manager);
  if (!font_manager || !unicode || !shaper) {
    throw std::runtime_error("Skia failed to create the portable text shaping stack");
  }

  return std::unique_ptr<SkiaGpuContexts>(new SkiaGpuContexts(
      std::make_unique<Implementation>(Implementation{
          .lease = std::move(lease),
          .ganesh = std::move(ganesh),
          .graphite = std::move(graphite),
          .font_manager = std::move(font_manager),
          .shaper = std::move(shaper),
      })));
}

bool SkiaGpuContexts::ganesh_ready() const noexcept {
  return implementation_ && static_cast<bool>(implementation_->ganesh);
}

bool SkiaGpuContexts::graphite_ready() const noexcept {
  return implementation_ && static_cast<bool>(implementation_->graphite);
}

const runtime::gpu::DeviceIdentity& SkiaGpuContexts::device_identity() const noexcept {
  return implementation_->lease.identity();
}

runtime::presentation::FrameResult SkiaGpuContexts::render(
    const runtime::presentation::NativeFrameTarget& target,
    const runtime::presentation::FixtureFrame& frame) {
  if (!implementation_ || !target.valid() ||
      target.backend != runtime::gpu::Backend::metal ||
      target.pixel_format != PixelFormat::bgra8_unorm ||
      target.device_generation != implementation_->lease.identity().generation) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = "Skia rejected an incompatible viewport render target",
    };
  }

  id<MTLTexture> texture = (__bridge id<MTLTexture>)(
      reinterpret_cast<void*>(target.texture));
  const auto handles = implementation_->lease.native_handles();
  id<MTLDevice> expected_device = (__bridge id<MTLDevice>)(
      reinterpret_cast<void*>(handles.device));
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
      static_cast<int>(target.width_pixels),
      static_cast<int>(target.height_pixels),
      texture_info);
  auto surface = SkSurfaces::WrapBackendRenderTarget(
      implementation_->ganesh.get(),
      backend_target,
      kTopLeft_GrSurfaceOrigin,
      kBGRA_8888_SkColorType,
      SkColorSpace::MakeSRGB(),
      nullptr);
  if (!surface) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = "Skia could not wrap the CAMetalLayer drawable texture",
    };
  }

  draw_fixture(*surface->getCanvas(),
               *implementation_->shaper,
               *implementation_->font_manager,
               frame,
               static_cast<float>(target.width_pixels),
               static_cast<float>(target.height_pixels));
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
