#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "refusion/platform/PlatformViewportPresenter.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <windows.h>
#include <wrl/client.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::platform {
namespace {

using Microsoft::WRL::ComPtr;
using runtime::presentation::BackendFrameTargetLease;
using runtime::presentation::FrameFailureKind;
using runtime::presentation::FrameResult;
using runtime::presentation::FrameStatus;
using runtime::presentation::NativeViewportHostLease;
using runtime::presentation::NativeWindowSystem;
using runtime::presentation::PixelFormat;
using runtime::presentation::PresentationFrameRequest;
using runtime::presentation::PresentationTelemetry;
using runtime::presentation::ViewportExtent;
using runtime::presentation::ViewportFrameRenderer;
using runtime::presentation::ViewportPresenter;

constexpr UINT kBufferCount = 3;
constexpr DWORD kFenceWaitTimeoutMs = 2'000;

[[nodiscard]] FrameResult rejected(
    std::string diagnostic,
    const FrameFailureKind failure = FrameFailureKind::incompatible,
    std::string code = "RFX-DXGI-REJECTED") {
  return FrameResult{
      .status = FrameStatus::rejected,
      .failure = failure,
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] FrameResult skipped(
    std::string diagnostic,
    const FrameFailureKind failure = FrameFailureKind::unavailable,
    std::string code = "RFX-DXGI-SKIPPED") {
  return FrameResult{
      .status = FrameStatus::skipped,
      .failure = failure,
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

struct Win32HostRegistration final {
  HWND window{nullptr};
  DWORD owner_thread_id{0};
};

struct DxgiSwapchainState final {
  ComPtr<IDXGISwapChain3> swapchain;
  std::array<ComPtr<ID3D12Resource>, kBufferCount> buffers;
  ComPtr<ID3D12Fence> fence;
  HANDLE fence_event{nullptr};
  std::array<std::uint64_t, kBufferCount> fence_values{};
  std::array<std::shared_ptr<runtime::gpu::GpuObservedResourceLease>,
             kBufferCount>
      observed_buffers;
  std::array<std::shared_ptr<runtime::gpu::GpuObservedFenceLease>,
             kBufferCount>
      observed_fences;
  std::array<std::chrono::steady_clock::time_point, kBufferCount>
      submission_times{};
  std::uint64_t next_fence_value{1};

  ~DxgiSwapchainState() {
    if (fence_event != nullptr) {
      CloseHandle(fence_event);
    }
  }
};

class DxgiViewportPresenter final : public ViewportPresenter {
 public:
  DxgiViewportPresenter(
      runtime::gpu::GpuDeviceService& device_service,
      ViewportFrameRenderer& frame_renderer,
      std::shared_ptr<runtime::gpu::GpuObservabilityService> observability)
      : device_service_(device_service), frame_renderer_(frame_renderer),
        observability_(std::move(observability)) {
    const auto device = device_service_.identity();
    if (device.backend != runtime::gpu::Backend::direct3d12 ||
        frame_renderer_.device_identity() != device) {
      throw std::invalid_argument(
          "D3D12 presenter and renderer must share one complete GPU identity");
    }
    if (observability_ && !observability_->observes(device)) {
      throw std::invalid_argument(
          "GPU observability and DXGI presentation must share one device identity");
    }
    telemetry_.device_generation = device.generation;
    telemetry_.device_status = runtime::gpu::DeviceStatus::ready;
  }

  ~DxgiViewportPresenter() override { detach(); }

  [[nodiscard]] runtime::gpu::DeviceIdentity device_identity()
      const noexcept override {
    return device_service_.identity();
  }

  [[nodiscard]] FrameResult attach(NativeViewportHostLease host) override {
    if (!host.valid() ||
        host.window_system != NativeWindowSystem::win32_hwnd) {
      ++telemetry_.rejected_frames;
      return rejected("DXGI presenter requires a valid Win32 HWND host lease");
    }
    const auto* registration = static_cast<const Win32HostRegistration*>(
        host.backend_private_host());
    if (registration == nullptr || registration->window == nullptr ||
        !IsWindow(registration->window)) {
      ++telemetry_.rejected_frames;
      return rejected("Win32 viewport host is not a live HWND");
    }

    detach();
    host_ = std::move(host);
    window_ = registration->window;
    if (extent_.valid()) {
      return create_or_resize_swapchain();
    }
    return FrameResult{.status = FrameStatus::accepted};
  }

  void detach() noexcept override {
    if (swapchain_state_) {
      static_cast<void>(wait_for_gpu_idle());
      static_cast<void>(frame_renderer_.retire_frame_targets());
      swapchain_state_.reset();
    }
    host_ = {};
    window_ = nullptr;
    visible_ = false;
    occluded_ = false;
  }

  [[nodiscard]] FrameResult resize(const ViewportExtent extent) override {
    if (!extent.valid()) {
      ++telemetry_.rejected_frames;
      return rejected("Viewport extent is empty or invalid");
    }
    extent_ = extent;
    if (window_ == nullptr) {
      return skipped("Viewport host is not attached yet");
    }
    return create_or_resize_swapchain();
  }

  void set_visible(const bool visible) noexcept override {
    if (visible_ == visible) {
      return;
    }
    visible_ = visible;
    if (visible) {
      ++telemetry_.visibility_resumes;
    } else {
      ++telemetry_.visibility_suspends;
    }
  }

  [[nodiscard]] FrameResult present(
      const PresentationFrameRequest& frame) override {
    ++telemetry_.frame_requests;
    const auto health = device_service_.health();
    telemetry_.device_status = health.status;
    telemetry_.device_event_sequence = health.event_sequence;
    if (health.status == runtime::gpu::DeviceStatus::suspended) {
      ++telemetry_.skipped_frames;
      ++telemetry_.device_suspended_frames;
      return skipped(health.code + ": " + health.diagnostic,
                     FrameFailureKind::unavailable, health.code);
    }
    if (health.status == runtime::gpu::DeviceStatus::lost) {
      return reject_device_loss(health);
    }
    if (health.identity != frame_renderer_.device_identity() ||
        health.identity.generation != telemetry_.device_generation ||
        (frame.device.generation != 0 && frame.device != health.identity)) {
      return reject_stale_generation();
    }
    if (!visible_ || window_ == nullptr || !IsWindow(window_) ||
        !extent_.valid() || !swapchain_state_) {
      ++telemetry_.skipped_frames;
      return skipped("Viewport is detached, hidden, or has no DXGI extent");
    }

    const HRESULT visibility =
        swapchain_state_->swapchain->Present(0, DXGI_PRESENT_TEST);
    if (visibility == DXGI_STATUS_OCCLUDED) {
      if (!occluded_) {
        ++telemetry_.occlusion_suspends;
      }
      occluded_ = true;
      ++telemetry_.skipped_frames;
      ++telemetry_.occluded_frames;
      return skipped("Win32 window is occluded", FrameFailureKind::occluded,
                     "RFX-VIEWPORT-OCCLUDED");
    }
    if (FAILED(visibility)) {
      return report_native_failure(
          "RFX-DXGI-VISIBILITY",
          "swapchain visibility test failed", visibility);
    }
    if (occluded_) {
      ++telemetry_.occlusion_resumes;
      occluded_ = false;
    }

    const UINT buffer_index =
        swapchain_state_->swapchain->GetCurrentBackBufferIndex();
    auto waited = wait_for_buffer(buffer_index);
    if (!waited.succeeded()) {
      return waited;
    }
    auto& buffer = swapchain_state_->buffers[buffer_index];
    if (!buffer) {
      ++telemetry_.rejected_frames;
      return rejected("DXGI current back buffer is empty");
    }
    ++telemetry_.drawable_acquisitions;

    std::shared_ptr<runtime::gpu::GpuObservedResourceLease> observed_buffer;
    if (observability_) {
      try {
        const auto resident_bytes =
            static_cast<std::uint64_t>(extent_.width_pixels()) *
            extent_.height_pixels() * 4ULL;
        observed_buffer =
            std::make_shared<runtime::gpu::GpuObservedResourceLease>(
                observability_, runtime::gpu::GpuSubsystem::presentation,
                runtime::gpu::GpuResourceKind::drawable,
                health.identity.generation, resident_bytes);
      } catch (const std::exception& error) {
        ++telemetry_.rejected_frames;
        return rejected(std::string("RFX-GPU-OBS-DXGI: ") + error.what());
      }
    }

    const BackendFrameTargetLease target{
        .device = health.identity,
        .pixel_format = PixelFormat::bgra8_unorm,
        .target_id = next_target_id_++,
        .width_pixels = extent_.width_pixels(),
        .height_pixels = extent_.height_pixels(),
        .backend_private_state = std::shared_ptr<const void>(
            swapchain_state_, buffer.Get()),
    };
    auto rendered = frame_renderer_.render(target, frame);
    if (!rendered.succeeded()) {
      if (rendered.status == FrameStatus::skipped) {
        ++telemetry_.skipped_frames;
      } else {
        ++telemetry_.rejected_frames;
      }
      return rendered;
    }
    ++telemetry_.renderer_submissions;

    const HRESULT presented = swapchain_state_->swapchain->Present(1, 0);
    if (FAILED(presented)) {
      return report_native_failure("RFX-DXGI-PRESENT", "Present failed",
                                   presented);
    }

    std::optional<runtime::gpu::BackendDeviceLease> device_lease;
    try {
      device_lease.emplace(device_service_.borrow());
    } catch (const std::exception& error) {
      ++telemetry_.rejected_frames;
      return rejected(std::string("RFX-GPU-BORROW: ") + error.what());
    }
    auto* queue = static_cast<ID3D12CommandQueue*>(const_cast<void*>(
        device_lease->backend_private_submission_queue()));
    const auto fence_value = swapchain_state_->next_fence_value++;
    if (queue == nullptr) {
      return report_native_failure("RFX-D3D12-FENCE-QUEUE",
                                   "presentation queue is unavailable",
                                   E_POINTER);
    }
    const HRESULT signaled =
        queue->Signal(swapchain_state_->fence.Get(), fence_value);
    if (FAILED(signaled)) {
      return report_native_failure("RFX-D3D12-FENCE-SIGNAL",
                                   "queue signal failed", signaled);
    }
    swapchain_state_->fence_values[buffer_index] = fence_value;
    swapchain_state_->observed_buffers[buffer_index] =
        std::move(observed_buffer);
    if (observability_) {
      try {
        swapchain_state_->observed_fences[buffer_index] =
            std::make_shared<runtime::gpu::GpuObservedFenceLease>(
                observability_, runtime::gpu::GpuSubsystem::presentation,
                health.identity.generation);
        swapchain_state_->submission_times[buffer_index] =
            std::chrono::steady_clock::now();
      } catch (const std::exception& error) {
        ++telemetry_.rejected_frames;
        return rejected(std::string("RFX-GPU-OBS-FENCE: ") + error.what());
      }
    }
    ++telemetry_.present_submissions;
    return FrameResult{.status = FrameStatus::presented};
  }

  [[nodiscard]] PresentationTelemetry telemetry() const noexcept override {
    return telemetry_;
  }

 private:
  [[nodiscard]] FrameResult create_or_resize_swapchain() {
    if (window_ == nullptr || !IsWindow(window_) || !extent_.valid()) {
      ++telemetry_.rejected_frames;
      return rejected("DXGI swapchain requires a live HWND and valid extent");
    }
    auto device_lease = device_service_.borrow();
    auto* device = static_cast<ID3D12Device*>(
        const_cast<void*>(device_lease.backend_private_device()));
    auto* queue = static_cast<ID3D12CommandQueue*>(const_cast<void*>(
        device_lease.backend_private_submission_queue()));
    if (device == nullptr || queue == nullptr) {
      ++telemetry_.rejected_frames;
      return rejected("D3D12 device lease has empty native objects");
    }

    if (!swapchain_state_) {
      auto state = std::make_shared<DxgiSwapchainState>();
      ComPtr<IDXGIFactory6> factory;
      if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
        ++telemetry_.rejected_frames;
        return rejected("DXGI factory creation failed");
      }
      DXGI_SWAP_CHAIN_DESC1 description{};
      description.Width = extent_.width_pixels();
      description.Height = extent_.height_pixels();
      description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      description.Stereo = FALSE;
      description.SampleDesc.Count = 1;
      description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      description.BufferCount = kBufferCount;
      description.Scaling = DXGI_SCALING_STRETCH;
      description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

      ComPtr<IDXGISwapChain1> swapchain;
      if (FAILED(factory->CreateSwapChainForHwnd(
              queue, window_, &description, nullptr, nullptr, &swapchain)) ||
          FAILED(factory->MakeWindowAssociation(window_, DXGI_MWA_NO_ALT_ENTER)) ||
          FAILED(swapchain.As(&state->swapchain))) {
        ++telemetry_.rejected_frames;
        return rejected("DXGI HWND swapchain creation failed");
      }
      if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                     IID_PPV_ARGS(&state->fence)))) {
        ++telemetry_.rejected_frames;
        return rejected("D3D12 presentation fence creation failed");
      }
      state->fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
      if (state->fence_event == nullptr) {
        ++telemetry_.rejected_frames;
        return rejected("D3D12 presentation fence event creation failed");
      }
      swapchain_state_ = std::move(state);
    } else {
      auto waited = wait_for_gpu_idle();
      if (!waited.succeeded()) {
        return waited;
      }
      auto retired = frame_renderer_.retire_frame_targets();
      if (!retired.succeeded()) {
        ++telemetry_.rejected_frames;
        return retired;
      }
      for (auto& buffer : swapchain_state_->buffers) {
        buffer.Reset();
      }
      const HRESULT resized = swapchain_state_->swapchain->ResizeBuffers(
          kBufferCount, extent_.width_pixels(), extent_.height_pixels(),
          DXGI_FORMAT_B8G8R8A8_UNORM, 0);
      if (FAILED(resized)) {
        return report_native_failure("RFX-DXGI-RESIZE",
                                     "ResizeBuffers failed", resized);
      }
    }

    constexpr auto color_space =
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    UINT color_space_support = 0;
    const HRESULT color_space_query =
        swapchain_state_->swapchain->CheckColorSpaceSupport(
            color_space, &color_space_support);
    if (FAILED(color_space_query)) {
      return report_native_failure(
          "RFX-DXGI-COLOR-QUERY",
          "CheckColorSpaceSupport failed for the Desktop-v1 SDR profile",
          color_space_query);
    }
    if ((color_space_support &
         DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == 0) {
      ++telemetry_.rejected_frames;
      return rejected(
          "DXGI output does not support the required sRGB/Rec.709 SDR color "
          "space",
          FrameFailureKind::incompatible, "RFX-DXGI-COLOR-UNSUPPORTED");
    }
    const HRESULT color_space_set =
        swapchain_state_->swapchain->SetColorSpace1(color_space);
    if (FAILED(color_space_set)) {
      return report_native_failure(
          "RFX-DXGI-COLOR-SET",
          "SetColorSpace1 failed for the Desktop-v1 SDR profile",
          color_space_set);
    }

    for (UINT index = 0; index < kBufferCount; ++index) {
      if (FAILED(swapchain_state_->swapchain->GetBuffer(
              index, IID_PPV_ARGS(&swapchain_state_->buffers[index])))) {
        ++telemetry_.rejected_frames;
        return rejected("DXGI back-buffer acquisition failed");
      }
    }
    return FrameResult{.status = FrameStatus::accepted};
  }

  [[nodiscard]] FrameResult wait_for_buffer(const UINT buffer_index) {
    const auto value = swapchain_state_->fence_values[buffer_index];
    const auto completed = swapchain_state_->fence->GetCompletedValue();
    if (completed == UINT64_MAX) {
      return report_native_failure(
          "RFX-D3D12-FENCE-REMOVED",
          "fence reported a removed D3D12 device", DXGI_ERROR_DEVICE_REMOVED);
    }
    if (value != 0 && completed < value) {
      const HRESULT armed = swapchain_state_->fence->SetEventOnCompletion(
          value, swapchain_state_->fence_event);
      if (FAILED(armed)) {
        return report_native_failure(
            "RFX-D3D12-FENCE-ARM",
            "SetEventOnCompletion failed", armed);
      }
      const DWORD wait_result = WaitForSingleObject(
          swapchain_state_->fence_event, kFenceWaitTimeoutMs);
      if (wait_result == WAIT_TIMEOUT) {
        ++telemetry_.native_wait_timeouts;
        return report_native_failure(
            "RFX-D3D12-FENCE-TIMEOUT",
            "back-buffer fence exceeded the 2000 ms fail-closed budget",
            DXGI_ERROR_DEVICE_HUNG, FrameFailureKind::timed_out);
      }
      if (wait_result != WAIT_OBJECT_0) {
        return report_native_failure(
            "RFX-D3D12-FENCE-WAIT",
            "waiting for a back-buffer fence failed",
            HRESULT_FROM_WIN32(GetLastError()));
      }
    }
    if (auto& observed = swapchain_state_->observed_fences[buffer_index]) {
      const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now() -
          swapchain_state_->submission_times[buffer_index]);
      static_cast<void>(observed->complete(
          static_cast<std::uint64_t>(elapsed.count())));
      observed.reset();
    }
    swapchain_state_->observed_buffers[buffer_index].reset();
    return FrameResult{.status = FrameStatus::accepted};
  }

  [[nodiscard]] FrameResult wait_for_gpu_idle() {
    if (!swapchain_state_) {
      return FrameResult{.status = FrameStatus::accepted};
    }
    for (UINT index = 0; index < kBufferCount; ++index) {
      auto result = wait_for_buffer(index);
      if (!result.succeeded()) {
        return result;
      }
    }
    return FrameResult{.status = FrameStatus::accepted};
  }

  [[nodiscard]] FrameResult reject_stale_generation() {
    ++telemetry_.rejected_frames;
    ++telemetry_.stale_generation_rejections;
    if (observability_) {
      static_cast<void>(observability_->reject_stale_generation(
          runtime::gpu::GpuSubsystem::presentation,
          frame_renderer_.device_identity().generation));
    }
    return rejected(
        "presenter rejected stale GPU resources",
        FrameFailureKind::stale_generation,
        "RFX-GPU-STALE-GENERATION");
  }

  [[nodiscard]] FrameResult reject_device_loss(
      const runtime::gpu::DeviceHealth& health) {
    if (observability_) {
      static_cast<void>(observability_->observe_device_loss(health.identity));
    }
    ++telemetry_.rejected_frames;
    ++telemetry_.device_loss_rejections;
    if (!health.generation_matches(frame_renderer_.device_identity())) {
      ++telemetry_.stale_generation_rejections;
    }
    return rejected(health.diagnostic, FrameFailureKind::device_lost,
                    health.code.empty() ? "RFX-GPU-DEVICE-LOST"
                                        : health.code);
  }

  [[nodiscard]] FrameResult report_native_failure(
      std::string code, std::string diagnostic, const HRESULT failure,
      const FrameFailureKind kind = FrameFailureKind::device_lost) {
    std::string detail = code + ": " + std::move(diagnostic) +
                         " (HRESULT=" +
                         std::to_string(static_cast<long long>(failure)) + ")";
    try {
      const auto lease = device_service_.borrow();
      auto* device = static_cast<ID3D12Device*>(
          const_cast<void*>(lease.backend_private_device()));
      if (device != nullptr) {
        const HRESULT removed_reason = device->GetDeviceRemovedReason();
        if (FAILED(removed_reason)) {
          detail += " (removed_reason=" +
                    std::to_string(
                        static_cast<long long>(removed_reason)) +
                    ")";
        }
      }
    } catch (const std::exception&) {
      // The device service may already be lost. The original HRESULT and
      // stable diagnostic code remain sufficient for fail-closed evidence.
    }
    const auto health = device_service_.report_device_loss(detail);
    telemetry_.device_status = health.status;
    telemetry_.device_event_sequence = health.event_sequence;
    auto result = reject_device_loss(health);
    result.failure = kind;
    result.code = std::move(code);
    result.diagnostic = std::move(detail);
    return result;
  }

  runtime::gpu::GpuDeviceService& device_service_;
  ViewportFrameRenderer& frame_renderer_;
  std::shared_ptr<runtime::gpu::GpuObservabilityService> observability_;
  NativeViewportHostLease host_;
  HWND window_{nullptr};
  ViewportExtent extent_;
  std::shared_ptr<DxgiSwapchainState> swapchain_state_;
  PresentationTelemetry telemetry_;
  std::uint64_t next_target_id_{1};
  bool visible_{false};
  bool occluded_{false};
};

}  // namespace

runtime::presentation::NativeWindowSystem
platform_native_window_system() noexcept {
  return runtime::presentation::NativeWindowSystem::win32_hwnd;
}

runtime::presentation::NativeViewportHostLease
acquire_platform_viewport_host(const std::uintptr_t native_handle) {
  auto* window = reinterpret_cast<HWND>(native_handle);
  if (window == nullptr || !IsWindow(window)) {
    throw std::invalid_argument(
        "RFX-VIEWPORT-HOST-001: native Win32 host is not a live HWND");
  }
  const DWORD owner_thread_id = GetWindowThreadProcessId(window, nullptr);
  if (owner_thread_id == 0) {
    throw std::invalid_argument(
        "RFX-VIEWPORT-HOST-002: HWND has no owning UI thread");
  }
  static std::atomic_uint64_t next_host_id{1};
  return runtime::presentation::NativeViewportHostLease{
      .window_system = runtime::presentation::NativeWindowSystem::win32_hwnd,
      .host_id = next_host_id.fetch_add(1),
      .backend_private_state = std::make_shared<const Win32HostRegistration>(
          Win32HostRegistration{
              .window = window,
              .owner_thread_id = owner_thread_id,
          }),
  };
}

std::unique_ptr<runtime::presentation::ViewportPresenter>
create_platform_viewport_presenter(
    runtime::gpu::GpuDeviceService& device_service,
    runtime::presentation::ViewportFrameRenderer& frame_renderer,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability) {
  return std::make_unique<DxgiViewportPresenter>(
      device_service, frame_renderer, std::move(observability));
}

}  // namespace refusion::platform
