#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "refusion/adapters/skia/SkiaGpuContexts.hpp"

#include "SkiaTextLayoutInternal.hpp"
#include "SkiaSurfacePolicy.hpp"
#include "SkiaVisualProgramExecutor.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/d3d/GrD3DBackendContext.h"
#include "include/gpu/ganesh/d3d/GrD3DBackendSurface.h"
#include "include/gpu/ganesh/d3d/GrD3DDirectContext.h"

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::adapters::skia {
namespace {

using Microsoft::WRL::ComPtr;
using runtime::presentation::BackendFrameTargetLease;
using runtime::presentation::FrameResult;
using runtime::presentation::FrameStatus;
using runtime::presentation::PixelFormat;
using runtime::presentation::PresentationFrameRequest;

[[nodiscard]] LUID adapter_luid(const std::uint64_t adapter_id) noexcept {
  return LUID{
      .LowPart = static_cast<DWORD>(adapter_id & 0xffffffffULL),
      .HighPart = static_cast<LONG>(
          static_cast<std::uint32_t>(adapter_id >> 32U)),
  };
}

[[nodiscard]] bool same_luid(const LUID left, const LUID right) noexcept {
  return left.LowPart == right.LowPart && left.HighPart == right.HighPart;
}

[[nodiscard]] ComPtr<IDXGIAdapter1> resolve_adapter(
    const runtime::gpu::DeviceIdentity& identity,
    ID3D12Device& device) {
  const auto expected_luid = adapter_luid(identity.adapter_id);
  if (!same_luid(device.GetAdapterLuid(), expected_luid)) {
    throw std::invalid_argument(
        "RFX-D3D12-IDENTITY-001: D3D12 device LUID differs from its lease");
  }

  ComPtr<IDXGIFactory6> factory;
  if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
    throw std::runtime_error(
        "RFX-D3D12-CONTEXT-001: DXGI factory creation failed");
  }
  for (UINT index = 0;; ++index) {
    ComPtr<IDXGIAdapter1> adapter;
    const HRESULT result = factory->EnumAdapters1(index, &adapter);
    if (result == DXGI_ERROR_NOT_FOUND) {
      break;
    }
    if (FAILED(result) || !adapter) {
      throw std::runtime_error(
          "RFX-D3D12-CONTEXT-002: DXGI adapter enumeration failed");
    }
    DXGI_ADAPTER_DESC1 description{};
    if (FAILED(adapter->GetDesc1(&description))) {
      throw std::runtime_error(
          "RFX-D3D12-CONTEXT-003: DXGI adapter description failed");
    }
    if (same_luid(description.AdapterLuid, expected_luid) &&
        (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
      return adapter;
    }
  }
  throw std::runtime_error(
      "RFX-D3D12-CONTEXT-004: leased hardware adapter was not found");
}

[[nodiscard]] FrameResult rejected(std::string diagnostic) {
  return FrameResult{
      .status = FrameStatus::rejected,
      .diagnostic = std::move(diagnostic),
  };
}

}  // namespace

struct SkiaGpuContexts::Implementation final {
  runtime::gpu::BackendDeviceLease lease;
  ComPtr<IDXGIAdapter1> adapter;
  sk_sp<GrDirectContext> ganesh;
  std::unique_ptr<SkiaTextLayoutEngine> text_layout_engine;
  SkiaVisualProgramExecutor visual_executor;
  std::shared_ptr<runtime::gpu::GpuObservabilityService> observability;
  std::unique_ptr<runtime::gpu::GpuObservedResourceLease> observed_context;
};

SkiaGpuContexts::SkiaGpuContexts(std::unique_ptr<Implementation> implementation)
    : implementation_(std::move(implementation)) {}

SkiaGpuContexts::~SkiaGpuContexts() = default;

std::unique_ptr<SkiaGpuContexts> SkiaGpuContexts::create(
    runtime::gpu::BackendDeviceLease lease,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue>
        decoded_video_queue,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability) {
  return create(std::move(lease), std::move(decoded_video_queue),
                std::move(observability),
                std::make_unique<SkiaTextLayoutEngine>());
}

std::unique_ptr<SkiaGpuContexts> SkiaGpuContexts::create(
    runtime::gpu::BackendDeviceLease lease,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue>
        decoded_video_queue,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability,
    std::unique_ptr<SkiaTextLayoutEngine> text_layout_engine) {
  if (!lease.valid() ||
      lease.identity().backend != runtime::gpu::Backend::direct3d12) {
    throw std::invalid_argument(
        "Skia D3D12 contexts require a valid D3D12 device lease");
  }
  if (decoded_video_queue) {
    throw std::invalid_argument(
        "RFX-D3D12-VIDEO-001: no admitted Windows native video-surface bridge exists");
  }
  if (observability && !observability->observes(lease.identity())) {
    throw std::invalid_argument(
        "GPU observability and Skia must share one device identity");
  }

  auto* device = static_cast<ID3D12Device*>(
      const_cast<void*>(lease.backend_private_device()));
  auto* command_queue = static_cast<ID3D12CommandQueue*>(
      const_cast<void*>(lease.backend_private_submission_queue()));
  if (device == nullptr || command_queue == nullptr) {
    throw std::invalid_argument(
        "RFX-D3D12-CONTEXT-005: device lease has empty native objects");
  }

  auto adapter = resolve_adapter(lease.identity(), *device);
  GrD3DBackendContext backend;
  backend.fAdapter.retain(adapter.Get());
  backend.fDevice.retain(device);
  backend.fQueue.retain(command_queue);
  auto ganesh = GrDirectContexts::MakeD3D(backend);
  if (!ganesh) {
    throw std::runtime_error(
        "RFX-D3D12-CONTEXT-006: Skia rejected the engine D3D12 context");
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

  return std::unique_ptr<SkiaGpuContexts>(new SkiaGpuContexts(
      std::make_unique<Implementation>(Implementation{
          .lease = std::move(lease),
          .adapter = std::move(adapter),
          .ganesh = std::move(ganesh),
          .text_layout_engine = std::move(text_layout_engine),
          .observability = std::move(observability),
          .observed_context = std::move(observed_context),
      })));
}

bool SkiaGpuContexts::ganesh_ready() const noexcept {
  return implementation_ && static_cast<bool>(implementation_->ganesh);
}

bool SkiaGpuContexts::graphite_ready() const noexcept {
  // Product rendering uses the single common Ganesh executor. Graphite remains
  // a separately qualified probe and is never a silent fallback.
  return false;
}

std::string SkiaGpuContexts::text_layout_engine_digest() const {
  if (!implementation_ || !implementation_->text_layout_engine) {
    return {};
  }
  return implementation_->text_layout_engine->layout_engine_digest();
}

std::optional<std::uint64_t>
SkiaGpuContexts::selected_video_source_frame_index() const noexcept {
  return std::nullopt;
}

const runtime::gpu::DeviceIdentity& SkiaGpuContexts::device_identity()
    const noexcept {
  return implementation_->lease.identity();
}

runtime::presentation::FrameResult SkiaGpuContexts::render(
    const BackendFrameTargetLease& target,
    const PresentationFrameRequest& frame) {
  if (!implementation_ || !target.valid() ||
      target.device.backend != runtime::gpu::Backend::direct3d12 ||
      target.pixel_format != PixelFormat::bgra8_unorm ||
      target.device != implementation_->lease.identity() || !frame.valid() ||
      frame.device != target.device) {
    return rejected("Skia rejected an incompatible D3D12 render target");
  }

  auto* resource = static_cast<ID3D12Resource*>(
      const_cast<void*>(target.backend_private_target()));
  if (resource == nullptr) {
    return rejected("Skia received an empty D3D12 back buffer");
  }
  ComPtr<ID3D12Device> resource_device;
  if (FAILED(resource->GetDevice(IID_PPV_ARGS(&resource_device))) ||
      resource_device.Get() != implementation_->lease.backend_private_device()) {
    return rejected(
        "Skia target does not belong to the engine D3D12 device");
  }
  const auto description = resource->GetDesc();
  if (description.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
      description.Width != target.width_pixels ||
      description.Height != target.height_pixels ||
      description.Format != DXGI_FORMAT_B8G8R8A8_UNORM ||
      description.SampleDesc.Count != 1) {
    return rejected("Skia received an incompatible DXGI back buffer");
  }

  GrD3DTextureResourceInfo resource_info(
      nullptr, nullptr, D3D12_RESOURCE_STATE_PRESENT,
      DXGI_FORMAT_B8G8R8A8_UNORM, 1, 1,
      DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN);
  // The raw-pointer constructor adopts a COM reference. The swapchain owns the
  // pointer supplied by the target lease, so Skia must retain its own reference.
  resource_info.fResource.retain(resource);
  const auto backend_target = GrBackendRenderTargets::MakeD3D(
      static_cast<int>(target.width_pixels),
      static_cast<int>(target.height_pixels), resource_info);
  auto surface = SkSurfaces::WrapBackendRenderTarget(
      implementation_->ganesh.get(), backend_target,
      kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType,
      SkColorSpace::MakeSRGB(), &visual_surface_props());
  if (!surface) {
    return rejected("Skia could not wrap the DXGI back buffer");
  }

  try {
    implementation_->visual_executor.execute(
        *surface, *implementation_->text_layout_engine,
        *frame.render_program, frame.project_time_ns,
        frame.transport_epoch_id, frame.output_consumer,
        frame.canvas_view, target.width_pixels, target.height_pixels);
  } catch (const std::exception& error) {
    return rejected(error.what());
  }

  if (implementation_->observability) {
    const auto observation = implementation_->observability->record_submission(
        runtime::gpu::GpuSubsystem::skia,
        implementation_->observability->issue_object_id(),
        implementation_->lease.identity().generation);
    if (!observation.accepted) {
      return rejected(observation.code + ": " + observation.diagnostic);
    }
  }

  GrFlushInfo flush_info;
  implementation_->ganesh->flush(
      surface.get(), SkSurfaces::BackendSurfaceAccess::kPresent, flush_info);
  implementation_->ganesh->submit();
  if (implementation_->ganesh->abandoned()) {
    return rejected(
        "Skia D3D12 context was abandoned during viewport submission");
  }
  return FrameResult{.status = FrameStatus::presented};
}

runtime::presentation::FrameResult SkiaGpuContexts::retire_frame_targets() {
  if (!implementation_ || !implementation_->ganesh) {
    return rejected("Skia D3D12 context is unavailable during target retirement");
  }
  static_cast<void>(implementation_->ganesh->submit(GrSyncCpu::kYes));
  implementation_->ganesh->performDeferredCleanup(
      std::chrono::milliseconds::zero());
  if (implementation_->ganesh->abandoned()) {
    return rejected("Skia D3D12 context was abandoned during target retirement");
  }
  return FrameResult{.status = FrameStatus::accepted};
}

}  // namespace refusion::adapters::skia
