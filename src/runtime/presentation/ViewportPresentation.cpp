#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <cmath>
#include <limits>

namespace refusion::runtime::presentation {
namespace {

[[nodiscard]] std::uint32_t scaled_extent(const std::uint32_t points,
                                          const float scale) noexcept {
  if (points == 0 || !std::isfinite(scale) || scale <= 0.0F) {
    return 0;
  }
  const double pixels = static_cast<double>(points) * static_cast<double>(scale);
  if (pixels > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
    return 0;
  }
  return static_cast<std::uint32_t>(std::lround(pixels));
}

}  // namespace

bool ViewportExtent::valid() const noexcept {
  return width_pixels() != 0 && height_pixels() != 0;
}

std::uint32_t ViewportExtent::width_pixels() const noexcept {
  return scaled_extent(width_points, pixels_per_point);
}

std::uint32_t ViewportExtent::height_pixels() const noexcept {
  return scaled_extent(height_points, pixels_per_point);
}

bool NativeViewportHost::valid() const noexcept { return handle != 0; }

bool NativeFrameTarget::valid() const noexcept {
  return texture != 0 && width_pixels != 0 && height_pixels != 0 &&
         device_generation != 0;
}

bool FrameResult::succeeded() const noexcept {
  return status == FrameStatus::accepted || status == FrameStatus::presented;
}

bool PresentationTelemetry::zero_cpu_pixel_transfer() const noexcept {
  return cpu_pixel_maps == 0 && cpu_pixel_uploads == 0 && gpu_readbacks == 0 &&
         unattributed_gpu_copies == 0;
}

ViewportRenderSession::ViewportRenderSession(ViewportPresenter& presenter) noexcept
    : presenter_(presenter), epoch_(std::chrono::steady_clock::now()) {}

FrameResult ViewportRenderSession::attach(const NativeViewportHost host) {
  return presenter_.attach(host);
}

void ViewportRenderSession::detach() noexcept { presenter_.detach(); }

FrameResult ViewportRenderSession::resize(const ViewportExtent extent) {
  return presenter_.resize(extent);
}

void ViewportRenderSession::set_visible(const bool visible) noexcept {
  presenter_.set_visible(visible);
}

FrameResult ViewportRenderSession::render_once() {
  const auto elapsed = std::chrono::steady_clock::now() - epoch_;
  const auto presentation_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
  return presenter_.present(FixtureFrame{
      .frame_index = next_frame_index_++,
      .presentation_time_ns = static_cast<std::uint64_t>(presentation_time),
  });
}

PresentationTelemetry ViewportRenderSession::telemetry() const noexcept {
  return presenter_.telemetry();
}

}  // namespace refusion::runtime::presentation
