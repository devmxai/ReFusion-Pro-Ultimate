#include "refusion/platform/PlatformViewportPresenter.hpp"

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstdint>
#include <memory>
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
    if (extent_.valid()) {
      return resize(extent_);
    }
    return FrameResult{.status = FrameStatus::accepted};
  }

  void detach() noexcept override {
    [metal_layer_ removeFromSuperlayer];
    metal_layer_ = nil;
    host_view_ = nil;
    visible_ = false;
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

  void set_visible(const bool visible) noexcept override { visible_ = visible; }

  [[nodiscard]] FrameResult present(const FixtureFrame& frame) override {
    ++telemetry_.frame_requests;
    @autoreleasepool {
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
          .device_generation = device_service_.identity().generation,
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

      auto device_lease = device_service_.borrow();
      const auto handles = device_lease.native_handles();
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
  ViewportExtent extent_;
  PresentationTelemetry telemetry_;
  bool visible_{false};
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
