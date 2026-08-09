#include "refusion/platform/PlatformViewportPresenter.hpp"

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstdint>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::platform {
namespace {

using runtime::presentation::PresentationFrameRequest;
using runtime::presentation::FrameResult;
using runtime::presentation::FrameStatus;
using runtime::presentation::BackendFrameTargetLease;
using runtime::presentation::NativeViewportHostLease;
using runtime::presentation::NativeWindowSystem;
using runtime::presentation::PresentationTargetProfile;
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

[[nodiscard]] runtime::gpu::GpuThermalState process_thermal_state() noexcept {
  switch (NSProcessInfo.processInfo.thermalState) {
    case NSProcessInfoThermalStateNominal:
      return runtime::gpu::GpuThermalState::nominal;
    case NSProcessInfoThermalStateFair:
      return runtime::gpu::GpuThermalState::fair;
    case NSProcessInfoThermalStateSerious:
      return runtime::gpu::GpuThermalState::serious;
    case NSProcessInfoThermalStateCritical:
      return runtime::gpu::GpuThermalState::critical;
  }
  return runtime::gpu::GpuThermalState::unknown;
}

class MetalViewportPresenter final : public ViewportPresenter {
 public:
  MetalViewportPresenter(runtime::gpu::GpuDeviceService& device_service,
                         ViewportFrameRenderer& frame_renderer,
                         std::shared_ptr<runtime::gpu::GpuObservabilityService>
                             observability)
      : device_service_(device_service), frame_renderer_(frame_renderer),
        observability_(std::move(observability)) {
    const auto& device = device_service_.identity();
    const auto& renderer = frame_renderer_.device_identity();
    if (device.backend != runtime::gpu::Backend::metal ||
        renderer.backend != runtime::gpu::Backend::metal ||
        device.adapter_id != renderer.adapter_id ||
        device.generation != renderer.generation) {
      throw std::invalid_argument(
          "Metal presenter and frame renderer must share one GPU generation");
    }
    if (observability_ && !observability_->observes(device)) {
      throw std::invalid_argument(
          "GPU observability and Metal presentation must share one device "
          "identity");
    }
    telemetry_.device_generation = device.generation;
    telemetry_.device_status = runtime::gpu::DeviceStatus::ready;
  }

  ~MetalViewportPresenter() override { detach(); }

  [[nodiscard]] runtime::gpu::DeviceIdentity device_identity()
      const noexcept override {
    return device_service_.identity();
  }

  [[nodiscard]] FrameResult attach(const NativeViewportHostLease host) override {
    if (!host.valid() || host.window_system != NativeWindowSystem::cocoa_view) {
      ++telemetry_.rejected_frames;
      return rejected("Metal presenter requires a valid Cocoa NSView host");
    }

    detach();
    NSView* view = (__bridge NSView*)(
        const_cast<void *>(host.backend_private_host()));
    if (view == nil || ![view isKindOfClass:[NSView class]]) {
      ++telemetry_.rejected_frames;
      return rejected("Cocoa viewport host is not an NSView");
    }

    auto device_lease = device_service_.borrow();
    id<MTLDevice> device = (__bridge id<MTLDevice>)(const_cast<void *>(
        device_lease.backend_private_device()));
    if (device == nil) {
      ++telemetry_.rejected_frames;
      return rejected("Metal presenter received an empty engine device");
    }

    [view setWantsLayer:YES];
    if (view.layer == nil) {
      view.layer = [CALayer layer];
    }

    presentation_profile_ =
        runtime::presentation::kFallbackSdrPresentationProfile;
    MTLPixelFormat metal_pixel_format = MTLPixelFormatBGRA8Unorm;
    CGColorSpaceRef presentation_color_space = nullptr;
    MTLTextureDescriptor* high_precision_probe_descriptor =
        [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                     width:1
                                    height:1
                                 mipmapped:NO];
    high_precision_probe_descriptor.usage = MTLTextureUsageRenderTarget;
    high_precision_probe_descriptor.storageMode = MTLStorageModePrivate;
    id<MTLTexture> high_precision_probe =
        [device newTextureWithDescriptor:high_precision_probe_descriptor];
    if (high_precision_probe != nil) {
      presentation_color_space =
          CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearSRGB);
      if (presentation_color_space != nullptr) {
        presentation_profile_ =
            runtime::presentation::kHighPrecisionSdrPresentationProfile;
        metal_pixel_format = MTLPixelFormatRGBA16Float;
      }
    }
    if (presentation_color_space == nullptr) {
      presentation_color_space =
          CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    }
    if (presentation_color_space == nullptr) {
      ++telemetry_.rejected_frames;
      return rejected(
          "Metal presenter could not create an admitted SDR color space");
    }

    CAMetalLayer* metal_layer = [CAMetalLayer layer];
    metal_layer.name = @"ReFusion.EngineViewport";
    metal_layer.device = device;
    metal_layer.pixelFormat = metal_pixel_format;
    metal_layer.colorspace = presentation_color_space;
    CGColorSpaceRelease(presentation_color_space);
    metal_layer.framebufferOnly = YES;
    metal_layer.opaque = YES;
    metal_layer.backgroundColor = NSColor.blackColor.CGColor;
    metal_layer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
    metal_layer.frame = view.bounds;
    metal_layer.maximumDrawableCount = 3;
    metal_layer.allowsNextDrawableTimeout = YES;
    metal_layer.displaySyncEnabled = YES;
    [view.layer addSublayer:metal_layer];

    if (observability_) {
      try {
        observed_layer_ =
            std::make_unique<runtime::gpu::GpuObservedResourceLease>(
                observability_, runtime::gpu::GpuSubsystem::presentation,
                runtime::gpu::GpuResourceKind::viewport_layer,
                device_lease.identity().generation, 0);
      } catch (const std::exception& error) {
        [metal_layer removeFromSuperlayer];
        metal_layer = nil;
        ++telemetry_.rejected_frames;
        return rejected(std::string("RFX-GPU-OBS-LAYER: ") + error.what());
      }
    }

    host_view_ = view;
    metal_layer_ = metal_layer;
    telemetry_.presentation_profile = presentation_profile_;
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
    observed_layer_.reset();
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

  [[nodiscard]] FrameResult present(
      const PresentationFrameRequest& frame) override {
    ++telemetry_.frame_requests;
    @autoreleasepool {
      if (observability_) {
        const auto thermal = observability_->record_thermal_sample(
            runtime::gpu::GpuSubsystem::presentation,
            process_thermal_state());
        if (!thermal.accepted) {
          ++telemetry_.rejected_frames;
          return rejected(thermal.code + ": " + thermal.diagnostic);
        }
      }
      const auto health = device_service_.health();
      telemetry_.device_status = health.status;
      telemetry_.device_event_sequence = health.event_sequence;
      if (health.status == runtime::gpu::DeviceStatus::suspended) {
        ++telemetry_.skipped_frames;
        ++telemetry_.device_suspended_frames;
        return skipped(health.code + ": " + health.diagnostic);
      }
      if (health.status == runtime::gpu::DeviceStatus::lost) {
        if (observability_) {
          static_cast<void>(observability_->observe_device_loss(health.identity));
        }
        ++telemetry_.rejected_frames;
        ++telemetry_.device_loss_rejections;
        if (!health.generation_matches(frame_renderer_.device_identity())) {
          ++telemetry_.stale_generation_rejections;
          if (observability_) {
            static_cast<void>(observability_->reject_stale_generation(
                runtime::gpu::GpuSubsystem::presentation,
                frame_renderer_.device_identity().generation));
          }
        }
        return rejected(health.code + ": " + health.diagnostic);
      }
      if (!health.generation_matches(frame_renderer_.device_identity()) ||
          health.identity.generation != telemetry_.device_generation ||
          (frame.device.generation != 0 && frame.device != health.identity)) {
        ++telemetry_.rejected_frames;
        ++telemetry_.stale_generation_rejections;
        if (observability_) {
          static_cast<void>(observability_->reject_stale_generation(
              runtime::gpu::GpuSubsystem::presentation,
              frame_renderer_.device_identity().generation));
        }
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
      std::shared_ptr<runtime::gpu::GpuObservedResourceLease>
          observed_drawable;
      if (observability_) {
        try {
          const auto resident_bytes =
              texture.allocatedSize != 0
                  ? static_cast<std::uint64_t>(texture.allocatedSize)
                  : static_cast<std::uint64_t>(texture.width) *
                        texture.height * presentation_profile_.bytes_per_pixel();
          observed_drawable =
              std::make_shared<runtime::gpu::GpuObservedResourceLease>(
                  observability_, runtime::gpu::GpuSubsystem::presentation,
                  runtime::gpu::GpuResourceKind::drawable,
                  health.identity.generation, resident_bytes);
        } catch (const std::exception& error) {
          ++telemetry_.rejected_frames;
          return rejected(std::string("RFX-GPU-OBS-DRAWABLE: ") +
                          error.what());
        }
      }
      const BackendFrameTargetLease target{
          .device = health.identity,
          .presentation_profile = presentation_profile_,
          .target_id = next_target_id_++,
          .width_pixels = static_cast<std::uint32_t>(texture.width),
          .height_pixels = static_cast<std::uint32_t>(texture.height),
          .backend_private_state = std::shared_ptr<const void>(
              CFBridgingRetain(texture),
              [](const void* value) { CFRelease(value); }),
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

      std::optional<runtime::gpu::BackendDeviceLease> device_lease;
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
      id<MTLCommandQueue> command_queue =
          (__bridge id<MTLCommandQueue>)(const_cast<void *>(
              device_lease->backend_private_submission_queue()));
      id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
      if (command_buffer == nil) {
        ++telemetry_.rejected_frames;
        return rejected("Metal failed to allocate the presentation command buffer");
      }
      command_buffer.label = @"ReFusion Present";
      std::shared_ptr<runtime::gpu::GpuObservedFenceLease> observed_fence;
      auto submission_start = std::chrono::steady_clock::now();
      if (observability_) {
        const auto submission = observability_->record_submission(
            runtime::gpu::GpuSubsystem::presentation,
            observability_->issue_object_id(), health.identity.generation);
        if (!submission.accepted) {
          ++telemetry_.rejected_frames;
          return rejected(submission.code + ": " + submission.diagnostic);
        }
        try {
          observed_fence =
              std::make_shared<runtime::gpu::GpuObservedFenceLease>(
                  observability_, runtime::gpu::GpuSubsystem::presentation,
                  health.identity.generation);
        } catch (const std::exception& error) {
          ++telemetry_.rejected_frames;
          return rejected(std::string("RFX-GPU-OBS-FENCE: ") + error.what());
        }
        [command_buffer
            addCompletedHandler:^(id<MTLCommandBuffer>) {
              const auto elapsed =
                  std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now() - submission_start);
              if (elapsed.count() >= 0) {
                static_cast<void>(observed_fence->complete(
                    static_cast<std::uint64_t>(elapsed.count())));
              }
              static_cast<void>(observed_drawable);
            }];
      }
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
  std::shared_ptr<runtime::gpu::GpuObservabilityService> observability_;
  std::unique_ptr<runtime::gpu::GpuObservedResourceLease> observed_layer_;
  PresentationTargetProfile presentation_profile_;
  bool visible_{false};
  std::uint64_t next_target_id_{1};
  std::atomic_bool occluded_{false};
  bool last_occluded_{false};
};

}  // namespace

runtime::presentation::NativeWindowSystem
platform_native_window_system() noexcept {
  return runtime::presentation::NativeWindowSystem::cocoa_view;
}

runtime::presentation::NativeViewportHostLease
acquire_platform_viewport_host(const std::uintptr_t native_handle) {
  NSView* view = (__bridge NSView*)(reinterpret_cast<void*>(native_handle));
  if (view == nil || ![view isKindOfClass:[NSView class]]) {
    throw std::invalid_argument(
        "RFX-VIEWPORT-HOST-001: native Cocoa host is not an NSView");
  }
  static std::atomic_uint64_t next_host_id{1};
  return runtime::presentation::NativeViewportHostLease{
      .window_system = runtime::presentation::NativeWindowSystem::cocoa_view,
      .host_id = next_host_id.fetch_add(1),
      .backend_private_state = std::shared_ptr<const void>(
          CFBridgingRetain(view),
          [](const void* value) { CFRelease(value); }),
  };
}

std::unique_ptr<runtime::presentation::ViewportPresenter>
create_platform_viewport_presenter(
    runtime::gpu::GpuDeviceService& device_service,
    runtime::presentation::ViewportFrameRenderer& frame_renderer,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability) {
  return std::make_unique<MetalViewportPresenter>(
      device_service, frame_renderer, std::move(observability));
}

}  // namespace refusion::platform
