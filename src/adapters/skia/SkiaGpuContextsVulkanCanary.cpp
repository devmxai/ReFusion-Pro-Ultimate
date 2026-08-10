#include "refusion/adapters/skia/SkiaGpuContexts.hpp"

#include "SkiaTextLayoutInternal.hpp"

#include "include/gpu/ganesh/vk/GrVkBackendContext.h"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::adapters::skia {

struct SkiaGpuContexts::Implementation final {
  runtime::gpu::BackendDeviceLease lease;
  std::unique_ptr<SkiaTextLayoutEngine> text_layout_engine;
};

SkiaGpuContexts::SkiaGpuContexts(std::unique_ptr<Implementation> implementation)
    : implementation_(std::move(implementation)) {}

SkiaGpuContexts::~SkiaGpuContexts() = default;

std::unique_ptr<SkiaGpuContexts> SkiaGpuContexts::create(
    runtime::gpu::BackendDeviceLease lease,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue> decoded_video,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability) {
  return create(std::move(lease), std::move(decoded_video),
                std::move(observability),
                std::make_unique<SkiaTextLayoutEngine>());
}

std::unique_ptr<SkiaGpuContexts> SkiaGpuContexts::create(
    runtime::gpu::BackendDeviceLease lease,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue> decoded_video,
    std::shared_ptr<runtime::gpu::GpuObservabilityService>,
    std::unique_ptr<SkiaTextLayoutEngine> text_layout_engine) {
  if (!lease.valid() ||
      lease.identity().backend != runtime::gpu::Backend::vulkan) {
    throw std::invalid_argument(
        "Android Skia canary requires a Vulkan device lease");
  }
  if (decoded_video) {
    throw std::invalid_argument(
        "RFX-ANDROID-CANARY-VIDEO: native video import is not admitted in G1");
  }
  if (!text_layout_engine) {
    text_layout_engine = std::make_unique<SkiaTextLayoutEngine>();
  }
  return std::unique_ptr<SkiaGpuContexts>(new SkiaGpuContexts(
      std::make_unique<Implementation>(Implementation{
          .lease = std::move(lease),
          .text_layout_engine = std::move(text_layout_engine),
      }))));
}

bool SkiaGpuContexts::ganesh_ready() const noexcept { return false; }

bool SkiaGpuContexts::graphite_ready() const noexcept { return false; }

std::string SkiaGpuContexts::text_layout_engine_digest() const {
  return implementation_ && implementation_->text_layout_engine
             ? implementation_->text_layout_engine->layout_engine_digest()
             : std::string{};
}

std::optional<std::uint64_t>
SkiaGpuContexts::selected_video_source_frame_index() const noexcept {
  return std::nullopt;
}

bool SkiaGpuContexts::publish_decoded_video_queue(
    std::string,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue> queue) noexcept {
  return queue == nullptr;
}

const runtime::gpu::DeviceIdentity& SkiaGpuContexts::device_identity()
    const noexcept {
  return implementation_->lease.identity();
}

runtime::presentation::FrameResult SkiaGpuContexts::render(
    const runtime::presentation::BackendFrameTargetLease&,
    const runtime::presentation::PresentationFrameRequest&) {
  return runtime::presentation::FrameResult{
      .status = runtime::presentation::FrameStatus::rejected,
      .diagnostic =
          "RFX-ANDROID-CANARY-NOT-PRODUCT: Vulkan rendering remains gated to G9",
  };
}

runtime::presentation::FrameResult SkiaGpuContexts::retire_frame_targets() {
  return runtime::presentation::FrameResult{
      .status = runtime::presentation::FrameStatus::accepted,
  };
}

}  // namespace refusion::adapters::skia
