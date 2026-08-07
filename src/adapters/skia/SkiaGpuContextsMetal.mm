#include "refusion/adapters/skia/SkiaGpuContexts.hpp"

#import <Metal/Metal.h>

#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "include/gpu/ganesh/mtl/GrMtlDirectContext.h"
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/mtl/MtlBackendContext.h"

#include <memory>
#include <stdexcept>
#include <utility>

namespace refusion::adapters::skia {

struct SkiaGpuContexts::Implementation final {
  runtime::gpu::DeviceLease lease;
  sk_sp<GrDirectContext> ganesh;
  std::unique_ptr<skgpu::graphite::Context> graphite;
};

SkiaGpuContexts::SkiaGpuContexts(std::unique_ptr<Implementation> implementation)
    : implementation_(std::move(implementation)) {}

SkiaGpuContexts::~SkiaGpuContexts() = default;
SkiaGpuContexts::SkiaGpuContexts(SkiaGpuContexts&&) noexcept = default;
SkiaGpuContexts& SkiaGpuContexts::operator=(SkiaGpuContexts&&) noexcept = default;

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

  return std::unique_ptr<SkiaGpuContexts>(new SkiaGpuContexts(
      std::make_unique<Implementation>(Implementation{
          .lease = std::move(lease),
          .ganesh = std::move(ganesh),
          .graphite = std::move(graphite),
      })));
}

bool SkiaGpuContexts::ganesh_ready() const noexcept {
  return implementation_ && static_cast<bool>(implementation_->ganesh);
}

bool SkiaGpuContexts::graphite_ready() const noexcept {
  return implementation_ && static_cast<bool>(implementation_->graphite);
}

}  // namespace refusion::adapters::skia
