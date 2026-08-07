#include "refusion/platform/PlatformViewportPresenter.hpp"

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstdint>
#include <atomic>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::platform {
namespace {

using runtime::presentation::FixtureFrame;
using runtime::presentation::FrameResult;
using runtime::presentation::FrameStatus;
using runtime::presentation::NativeFrameTarget;
using runtime::presentation::NativeViewportHost;
using runtime::presentation::NativeWindowSystem;
using runtime::presentation::PixelFormat;
using runtime::presentation::PresentationTelemetry;
using runtime::presentation::ViewportExtent;
using runtime::presentation::ViewportFrameRenderer;
using runtime::presentation::ViewportPresenter;

[[nodiscard]] FrameResult rejected(std::string diagnostic) {
  return FrameResult{
      .status = FrameStatus::rejected,
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] FrameResult skipped(std::string diagnostic) {
  return FrameResult{
      .status = FrameStatus::skipped,
      .diagnostic = std::move(diagnostic),
  };
}

class MetalViewportPresenter final : public ViewportPresenter {
 public:
  MetalViewportPresenter(runtime::gpu::GpuDeviceService& device_service,
                         ViewportFrameRenderer& frame_renderer)
      : device_service_(device_service), frame_renderer_(frame_renderer) {
    const auto& device = device_service_.identity();
    const auto& renderer = frame_renderer_.device_identity();
    if (device.backend != runtime::gpu::Backend::metal ||
        renderer.backend != runtime::gpu::Backend::metal ||
        device.adapter_id != renderer.adapter_id ||
        device.generation != renderer.generation) {
      throw std::invalid_argument(
          "Metal presenter and frame renderer must share one GPU generation");
    }
    telemetry_.device_generation = device.generation;
    telemetry_.device_status = runtime::gpu::DeviceStatus::ready;
  }

  ~MetalViewportPresenter() override { detach(); }

  [[nodiscard]] FrameResult attach(const NativeViewportHost host) override {
    if (!host.valid() || host.window_system != NativeWindowSystem::cocoa_view) {
      ++telemetry_.rejected_frames;
      return rejected("Metal presenter requires a valid Cocoa NSView host");
    }

    detach();
    NSView* view = (__bridge NSView*)(reinterpret_cast<void*>(host.handle));
    if (view == nil || ![view isKindOfClass:[NSView class]]) {
      ++telemetry_.rejected_frames;
      return rejected("Cocoa viewport host is not an NSView");
    }

    auto device_lease = device_service_.borrow();
    const auto handles = device_lease.native_handles();
    id<MTLDevice> device = (__bridge id<MTLDevice>)(
        reinterpret_cast<void*>(handles.device));
    if (device == nil) {
      ++telemetry_.rejected_frames;
      return rejected("Metal presenter received an empty engine device");
    }

    [view setWantsLayer:YES];
    if (view.layer == nil) {
      view.layer = [CALayer layer];
    }

    CAMetalLayer* metal_layer = [CAMetalLayer layer];
    metal_layer.name = @"ReFusion.EngineViewport";
    metal_layer.device = device;
    metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metal_layer.framebufferOnly = YES;
    metal_layer.opaque = YES;
    metal_layer.backgroundColor = NSColor.blackColor.CGColor;
    metal_layer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
    metal_layer.frame = view.bounds;
    metal_layer.maximumDrawableCount = 3;
    metal_layer.allowsNextDrawableTimeout = YES;
    metal_layer.displaySyncEnabled = YES;
    [view.layer addSublayer:metal_layer];

    host_view_ = view;
    metal_layer_ = metal_layer;
    NSWindow* window = view.window;
    if (window != nil) {
      occluded_.store(
          (window.occlusionState & NSWindowOcclusionStateVisible) == 0);
      occlusion_observer_ = [NSNotificationCenter.defaultCenter
          addObserverForName:NSWindowDidChangeOcclusionStateNotification
                      object:window
                       queue:nil
                  usingBlock:^(NSNotification* notification) {
                    NSWindow* observed_window = notification.object;
                    occluded_.store(
                        (observed_window.occlusionState &
                         NSWindowOcclusionStateVisible) == 0);
                  }];
    }
    if (extent_.valid()) {
      return resize(extent_);
    }
    return FrameResult{.status = FrameStatus::accepted};
  }

  void detach() noexcept override {
    if (occlusion_observer_ != nil) {
      [NSNotificationCenter.defaultCenter removeObserver:occlusion_observer_];
      occlusion_observer_ = nil;
    }
    [metal_layer_ removeFromSuperlayer];
    metal_layer_ = nil;
    host_view_ = nil;
    visible_ = false;
    occluded_.store(false);
    last_occluded_ = false;
  }

  [[nodiscard]] FrameResult resize(const ViewportExtent extent) override {
    if (!extent.valid()) {
      ++telemetry_.rejected_frames;
      return rejected("Viewport extent is empty or invalid");
    }
    extent_ = extent;
    if (metal_layer_ == nil) {
      return skipped("Viewport host is not attached yet");
    }
    metal_layer_.contentsScale = extent.pixels_per_point;
    metal_layer_.drawableSize = CGSizeMake(
        static_cast<CGFloat>(extent.width_pixels()),
        static_cast<CGFloat>(extent.height_pixels()));
    if (host_view_ != nil) {
      metal_layer_.frame = host_view_.bounds;
    }
    return FrameResult{.status = FrameStatus::accepted};
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

  [[nodiscard]] FrameResult present(const FixtureFrame& frame) override {
    ++telemetry_.frame_requests;
    @autoreleasepool {
      const auto health = device_service_.health();
      telemetry_.device_status = health.status;
      telemetry_.device_event_sequence = health.event_sequence;
      if (health.status == runtime::gpu::DeviceStatus::suspended) {
        ++telemetry_.skipped_frames;
        ++telemetry_.device_suspended_frames;
        return skipped(health.code + ": " + health.diagnostic);
      }
      if (health.status == runtime::gpu::DeviceStatus::lost) {
        ++telemetry_.rejected_frames;
        ++telemetry_.device_loss_rejections;
        if (!health.generation_matches(frame_renderer_.device_identity())) {
          ++telemetry_.stale_generation_rejections;
        }
        return rejected(health.code + ": " + health.diagnostic);
      }
      if (!health.generation_matches(frame_renderer_.device_identity()) ||
          health.identity.generation != telemetry_.device_generation) {
        ++telemetry_.rejected_frames;
        ++telemetry_.stale_generation_rejections;
        return rejected(
            "RFX-GPU-STALE-GENERATION: presenter rejected stale GPU resources");
      }
      const bool occluded = occluded_.load();
      if (occluded != last_occluded_) {
        if (occluded) {
          ++telemetry_.occlusion_suspends;
        } else {
          ++telemetry_.occlusion_resumes;
        }
        last_occluded_ = occluded;
      }
      if (occluded) {
        ++telemetry_.skipped_frames;
        ++telemetry_.occluded_frames;
        return skipped("RFX-VIEWPORT-OCCLUDED: native window is fully occluded");
      }
      if (metal_layer_ == nil || !extent_.valid() || !visible_) {
        ++telemetry_.skipped_frames;
        return skipped("Viewport is detached, hidden, or has no drawable extent");
      }

      id<CAMetalDrawable> drawable = [metal_layer_ nextDrawable];
      if (drawable == nil) {
        ++telemetry_.skipped_frames;
        return skipped("CAMetalLayer did not provide a drawable");
      }
      ++telemetry_.drawable_acquisitions;

      id<MTLTexture> texture = drawable.texture;
      const NativeFrameTarget target{
          .backend = runtime::gpu::Backend::metal,
          .pixel_format = PixelFormat::bgra8_unorm,
          .texture = reinterpret_cast<std::uintptr_t>((__bridge void*)texture),
          .width_pixels = static_cast<std::uint32_t>(texture.width),
          .height_pixels = static_cast<std::uint32_t>(texture.height),
          .device_generation = health.identity.generation,
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

      std::optional<runtime::gpu::DeviceLease> device_lease;
      try {
        device_lease.emplace(device_service_.borrow());
      } catch (const std::exception& error) {
        const auto failed_health = device_service_.health();
        telemetry_.device_status = failed_health.status;
        telemetry_.device_event_sequence = failed_health.event_sequence;
        if (failed_health.status == runtime::gpu::DeviceStatus::suspended) {
          ++telemetry_.skipped_frames;
          ++telemetry_.device_suspended_frames;
          return skipped(failed_health.code + ": " + failed_health.diagnostic);
        }
        ++telemetry_.rejected_frames;
        ++telemetry_.device_loss_rejections;
        if (!failed_health.generation_matches(frame_renderer_.device_identity())) {
          ++telemetry_.stale_generation_rejections;
        }
        return rejected(std::string("RFX-GPU-BORROW: ") + error.what());
      }
      const auto handles = device_lease->native_handles();
      id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)(
          reinterpret_cast<void*>(handles.command_queue));
      id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
      if (command_buffer == nil) {
        ++telemetry_.rejected_frames;
        return rejected("Metal failed to allocate the presentation command buffer");
      }
      command_buffer.label = @"ReFusion Present";
      [command_buffer presentDrawable:drawable];
      [command_buffer commit];
      ++telemetry_.present_submissions;
      return FrameResult{.status = FrameStatus::presented};
    }
  }

  [[nodiscard]] PresentationTelemetry telemetry() const noexcept override {
    return telemetry_;
  }

 private:
  runtime::gpu::GpuDeviceService& device_service_;
  ViewportFrameRenderer& frame_renderer_;
  __weak NSView* host_view_{nil};
  __strong CAMetalLayer* metal_layer_{nil};
  __strong id occlusion_observer_{nil};
  ViewportExtent extent_;
  PresentationTelemetry telemetry_;
  bool visible_{false};
  std::atomic_bool occluded_{false};
  bool last_occluded_{false};
};

}  // namespace

runtime::presentation::NativeWindowSystem
platform_native_window_system() noexcept {
  return runtime::presentation::NativeWindowSystem::cocoa_view;
}

std::unique_ptr<runtime::presentation::ViewportPresenter>
create_platform_viewport_presenter(
    runtime::gpu::GpuDeviceService& device_service,
    runtime::presentation::ViewportFrameRenderer& frame_renderer) {
  return std::make_unique<MetalViewportPresenter>(device_service, frame_renderer);
}

}  // namespace refusion::platform
