#include "refusion/adapters/skia/SkiaGpuContexts.hpp"

#include "SkiaTextLayoutInternal.hpp"
#include "SkiaVisualProgramExecutor.hpp"

#if defined(REFUSION_SKIA_APPLE_MEDIA)
#include "refusion/platform/apple/AppleMediaSurface.hpp"
#endif

#import <Metal/Metal.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
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

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace refusion::adapters::skia {

struct SkiaGpuContexts::Implementation final {
  struct DecodedVideoFrame final {
    std::uint64_t lease_id{0};
    std::uint64_t source_frame_index{0};
    sk_sp<SkImage> image;
  };

  runtime::gpu::BackendDeviceLease lease;
  sk_sp<GrDirectContext> ganesh;
  std::unique_ptr<SkiaTextLayoutEngine> text_layout_engine;
  std::shared_ptr<const runtime::media::DecodedSurfaceQueue> decoded_video_queue;
  std::vector<DecodedVideoFrame> decoded_video_frames;
  std::shared_ptr<runtime::gpu::GpuObservabilityService> observability;
  std::unique_ptr<runtime::gpu::GpuObservedResourceLease> observed_context;
  std::unique_ptr<std::atomic_uint64_t> selected_video_source_frame_index{
      std::make_unique<std::atomic_uint64_t>(std::numeric_limits<std::uint64_t>::max())};
};

namespace {

using runtime::presentation::PresentationFrameRequest;
using runtime::presentation::FrameResult;
using runtime::presentation::FrameStatus;
using runtime::presentation::BackendFrameTargetLease;
using runtime::presentation::PixelFormat;

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
    runtime::gpu::BackendDeviceLease lease,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue> decoded_video_queue,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability) {
  return create(std::move(lease), std::move(decoded_video_queue),
                std::move(observability),
                std::make_unique<SkiaTextLayoutEngine>());
}

std::unique_ptr<SkiaGpuContexts> SkiaGpuContexts::create(
    runtime::gpu::BackendDeviceLease lease,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue> decoded_video_queue,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability,
    std::unique_ptr<SkiaTextLayoutEngine> text_layout_engine) {
  if (!lease.valid() || lease.identity().backend != runtime::gpu::Backend::metal) {
    throw std::invalid_argument("Skia Metal contexts require a valid Metal device lease");
  }
  if (observability && !observability->observes(lease.identity())) {
    throw std::invalid_argument(
        "GPU observability and Skia must share one device identity");
  }
  id<MTLDevice> device = (__bridge id<MTLDevice>)(
      const_cast<void *>(lease.backend_private_device()));
  id<MTLCommandQueue> command_queue =
      (__bridge id<MTLCommandQueue>)(const_cast<void *>(
          lease.backend_private_submission_queue()));

  GrMtlBackendContext ganesh_backend;
  ganesh_backend.fDevice.retain((__bridge GrMTLHandle)device);
  ganesh_backend.fQueue.retain((__bridge GrMTLHandle)command_queue);
  auto ganesh = GrDirectContexts::MakeMetal(ganesh_backend);
  if (!ganesh) {
    throw std::runtime_error("Skia Ganesh rejected the engine-owned Metal device");
  }

  if (!text_layout_engine) {
    text_layout_engine = std::make_unique<SkiaTextLayoutEngine>();
  }

  std::unique_ptr<runtime::gpu::GpuObservedResourceLease> observed_context;
  if (observability) {
    observed_context =
        std::make_unique<runtime::gpu::GpuObservedResourceLease>(
            observability, runtime::gpu::GpuSubsystem::skia,
            runtime::gpu::GpuResourceKind::render_context,
            lease.identity().generation, 0);
  }

  std::vector<Implementation::DecodedVideoFrame> decoded_video_frames;
  if (decoded_video_queue) {
    const auto &queue_device = decoded_video_queue->device_identity();
    if (queue_device.backend != lease.identity().backend ||
        queue_device.adapter_id != lease.identity().adapter_id ||
        queue_device.generation != lease.identity().generation) {
      throw std::invalid_argument("Decoded video queue does not belong to the Skia GPU generation");
    }
#if defined(REFUSION_SKIA_APPLE_MEDIA)
    decoded_video_frames.reserve(decoded_video_queue->size());
    for (std::size_t index = 0; index < decoded_video_queue->size(); ++index) {
      const auto &surface = decoded_video_queue->frame(index);
      decoded_video_frames.push_back({
          .lease_id = surface->info().lease_id,
          .source_frame_index = surface->info().source_frame_index,
          .image = make_decoded_video_image(*ganesh, surface, lease.identity()),
      });
    }
#else
    throw std::invalid_argument("This Skia build has no admitted native video-surface bridge");
#endif
  }

  return std::unique_ptr<SkiaGpuContexts>(
      new SkiaGpuContexts(std::make_unique<Implementation>(Implementation{
          .lease = std::move(lease),
          .ganesh = std::move(ganesh),
          .text_layout_engine = std::move(text_layout_engine),
          .decoded_video_queue = std::move(decoded_video_queue),
          .decoded_video_frames = std::move(decoded_video_frames),
          .observability = std::move(observability),
          .observed_context = std::move(observed_context),
      })));
}

bool SkiaGpuContexts::ganesh_ready() const noexcept {
  return implementation_ && static_cast<bool>(implementation_->ganesh);
}

bool SkiaGpuContexts::graphite_ready() const noexcept {
  // Graphite is deliberately not created in the product rendering path.
  // A future Graphite experiment must own an independent probe target and
  // evidence; the qualified preview path has exactly one Ganesh context.
  return false;
}

std::string SkiaGpuContexts::text_layout_engine_digest() const {
  if (!implementation_ || !implementation_->text_layout_engine) {
    return {};
  }
  return implementation_->text_layout_engine->layout_engine_digest();
}

std::optional<std::uint64_t> SkiaGpuContexts::selected_video_source_frame_index() const noexcept {
  if (!implementation_) {
    return std::nullopt;
  }
  const auto index = implementation_->selected_video_source_frame_index->load();
  if (index == std::numeric_limits<std::uint64_t>::max()) {
    return std::nullopt;
  }
  return index;
}

const runtime::gpu::DeviceIdentity &SkiaGpuContexts::device_identity() const noexcept {
  return implementation_->lease.identity();
}

runtime::presentation::FrameResult SkiaGpuContexts::render(
    const runtime::presentation::BackendFrameTargetLease &target,
    const runtime::presentation::PresentationFrameRequest &frame) {
  if (!implementation_ || !target.valid() ||
      target.device.backend != runtime::gpu::Backend::metal ||
      target.pixel_format != PixelFormat::bgra8_unorm ||
      target.device != implementation_->lease.identity() || !frame.valid() ||
      frame.device != target.device) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = "Skia rejected an incompatible viewport render target",
    };
  }

  id<MTLTexture> texture = (__bridge id<MTLTexture>)(
      const_cast<void *>(target.backend_private_target()));
  id<MTLDevice> expected_device =
      (__bridge id<MTLDevice>)(const_cast<void *>(
          implementation_->lease.backend_private_device()));
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
  try {
    execute_visual_render_program(
        canvas, *implementation_->text_layout_engine, *frame.render_program,
        frame.project_time_ns, frame.transport_epoch_id,
        static_cast<float>(target.width_pixels),
        static_cast<float>(target.height_pixels));
  } catch (const std::exception &error) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = error.what(),
    };
  }
  if (implementation_->decoded_video_queue) {
    if (frame.project_time_ns >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      return FrameResult{
          .status = FrameStatus::rejected,
          .diagnostic = "Skia rejected ProjectTime outside the exact media domain",
      };
    }
    const auto selected_surface = implementation_->decoded_video_queue->select_at({
        .value = static_cast<std::int64_t>(frame.project_time_ns),
        .timescale = 1'000'000'000,
    });
    if (selected_surface) {
      const auto selected_image = std::find_if(
          implementation_->decoded_video_frames.begin(),
          implementation_->decoded_video_frames.end(), [&selected_surface](const auto &candidate) {
            return candidate.lease_id == selected_surface->info().lease_id;
          });
      if (selected_image == implementation_->decoded_video_frames.end() || !selected_image->image) {
        return FrameResult{
            .status = FrameStatus::rejected,
            .diagnostic = "Skia could not resolve the PTS-selected decoded surface",
        };
      }
      draw_decoded_video_fixture(canvas, *selected_image->image,
                                 static_cast<float>(target.width_pixels),
                                 static_cast<float>(target.height_pixels));
      implementation_->selected_video_source_frame_index->store(selected_image->source_frame_index);
    } else {
      implementation_->selected_video_source_frame_index->store(
          std::numeric_limits<std::uint64_t>::max());
    }
  }
  if (implementation_->observability) {
    const auto observation = implementation_->observability->record_submission(
        runtime::gpu::GpuSubsystem::skia,
        implementation_->observability->issue_object_id(),
        implementation_->lease.identity().generation);
    if (!observation.accepted) {
      return FrameResult{
          .status = FrameStatus::rejected,
          .diagnostic = observation.code + ": " + observation.diagnostic,
      };
    }
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
