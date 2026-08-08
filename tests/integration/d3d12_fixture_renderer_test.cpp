#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "XplatFontFixture.hpp"
#include "refusion/adapters/skia/SkiaGpuComposition.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/runtime/render/RenderPlanCompiler.hpp"

#include <d3d12.h>
#include <windows.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Microsoft::WRL::ComPtr;

void require(const bool condition, const std::string& diagnostic) {
  if (!condition) {
    throw std::runtime_error(diagnostic);
  }
}

void require_hresult(const HRESULT result, const std::string& diagnostic) {
  require(SUCCEEDED(result), diagnostic + " HRESULT=" +
                                 std::to_string(static_cast<long>(result)));
}

class EventHandle final {
 public:
  EventHandle() : value_(CreateEventW(nullptr, FALSE, FALSE, nullptr)) {
    require(value_ != nullptr, "D3D12 qualification could not create an event");
  }

  ~EventHandle() {
    if (value_ != nullptr) {
      CloseHandle(value_);
    }
  }

  EventHandle(const EventHandle&) = delete;
  EventHandle& operator=(const EventHandle&) = delete;

  [[nodiscard]] HANDLE get() const noexcept { return value_; }

 private:
  HANDLE value_{nullptr};
};

[[nodiscard]] refusion::runtime::render::VisualRenderProgram
conformance_render_program() {
  std::ifstream input(REFUSION_XPLAT_VISUAL_FIXTURE_PATH, std::ios::binary);
  require(static_cast<bool>(input),
          "D3D12 qualification could not open the visual fixture");
  const std::string source{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  const auto compiled = refusion::core::compile_project_rfx(source);
  require(compiled.succeeded(),
          "D3D12 qualification could not compile the visual fixture");
  return refusion::runtime::render::compile_visual_render_program(
      *compiled.project);
}

[[nodiscard]] ComPtr<ID3D12Resource> create_target_texture(
    ID3D12Device& device, const std::uint32_t width,
    const std::uint32_t height) {
  D3D12_HEAP_PROPERTIES heap{};
  heap.Type = D3D12_HEAP_TYPE_DEFAULT;
  heap.CreationNodeMask = 1;
  heap.VisibleNodeMask = 1;
  D3D12_RESOURCE_DESC description{};
  description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  description.Width = width;
  description.Height = height;
  description.DepthOrArraySize = 1;
  description.MipLevels = 1;
  description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  description.SampleDesc.Count = 1;
  description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  description.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  ComPtr<ID3D12Resource> texture;
  require_hresult(
      device.CreateCommittedResource(
          &heap, D3D12_HEAP_FLAG_NONE, &description,
          D3D12_RESOURCE_STATE_PRESENT, nullptr, IID_PPV_ARGS(&texture)),
      "D3D12 qualification could not create the target texture");
  return texture;
}

[[nodiscard]] refusion::runtime::presentation::BackendFrameTargetLease
target_lease(const refusion::runtime::gpu::DeviceIdentity& identity,
             const std::uint64_t target_id,
             const std::uint32_t width,
             const std::uint32_t height,
             const ComPtr<ID3D12Resource>& texture) {
  auto owner = std::make_shared<ComPtr<ID3D12Resource>>(texture);
  return {
      .device = identity,
      .pixel_format =
          refusion::runtime::presentation::PixelFormat::bgra8_unorm,
      .target_id = target_id,
      .width_pixels = width,
      .height_pixels = height,
      .backend_private_state =
          std::shared_ptr<const void>(owner, owner->Get()),
  };
}

void wait_for_queue(ID3D12Device& device, ID3D12CommandQueue& queue) {
  ComPtr<ID3D12Fence> fence;
  require_hresult(
      device.CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
      "D3D12 qualification could not create a fence");
  EventHandle event;
  constexpr std::uint64_t fence_value = 1;
  require_hresult(queue.Signal(fence.Get(), fence_value),
                  "D3D12 qualification could not signal its fence");
  require_hresult(fence->SetEventOnCompletion(fence_value, event.get()),
                  "D3D12 qualification could not arm its fence event");
  require(WaitForSingleObject(event.get(), 5'000) == WAIT_OBJECT_0,
          "RFX-D3D12-QUALIFICATION-TIMEOUT: queue did not finish in 5 s");
  require(fence->GetCompletedValue() != UINT64_MAX,
          "RFX-D3D12-QUALIFICATION-DEVICE-LOST: fence reported removal");
}

[[nodiscard]] std::vector<std::uint8_t> read_bgra(
    ID3D12Device& device, ID3D12CommandQueue& queue,
    ID3D12Resource& texture, const std::uint32_t width,
    const std::uint32_t height) {
  const auto texture_description = texture.GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
  UINT row_count = 0;
  UINT64 row_size = 0;
  UINT64 total_size = 0;
  device.GetCopyableFootprints(&texture_description, 0, 1, 0, &footprint,
                               &row_count, &row_size, &total_size);
  require(row_count == height && row_size == width * 4ULL && total_size != 0,
          "D3D12 qualification received an unexpected copy footprint");

  D3D12_HEAP_PROPERTIES readback_heap{};
  readback_heap.Type = D3D12_HEAP_TYPE_READBACK;
  readback_heap.CreationNodeMask = 1;
  readback_heap.VisibleNodeMask = 1;
  D3D12_RESOURCE_DESC buffer_description{};
  buffer_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  buffer_description.Width = total_size;
  buffer_description.Height = 1;
  buffer_description.DepthOrArraySize = 1;
  buffer_description.MipLevels = 1;
  buffer_description.SampleDesc.Count = 1;
  buffer_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  ComPtr<ID3D12Resource> readback;
  require_hresult(
      device.CreateCommittedResource(
          &readback_heap, D3D12_HEAP_FLAG_NONE, &buffer_description,
          D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)),
      "D3D12 qualification could not create the readback buffer");

  ComPtr<ID3D12CommandAllocator> allocator;
  require_hresult(
      device.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    IID_PPV_ARGS(&allocator)),
      "D3D12 qualification could not create a command allocator");
  ComPtr<ID3D12GraphicsCommandList> commands;
  require_hresult(
      device.CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                               allocator.Get(), nullptr,
                               IID_PPV_ARGS(&commands)),
      "D3D12 qualification could not create a command list");

  D3D12_RESOURCE_BARRIER to_copy{};
  to_copy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  to_copy.Transition.pResource = &texture;
  to_copy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  to_copy.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  to_copy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  commands->ResourceBarrier(1, &to_copy);
  D3D12_TEXTURE_COPY_LOCATION source{};
  source.pResource = &texture;
  source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  source.SubresourceIndex = 0;
  D3D12_TEXTURE_COPY_LOCATION destination{};
  destination.pResource = readback.Get();
  destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  destination.PlacedFootprint = footprint;
  commands->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
  std::swap(to_copy.Transition.StateBefore, to_copy.Transition.StateAfter);
  commands->ResourceBarrier(1, &to_copy);
  require_hresult(commands->Close(),
                  "D3D12 qualification could not close its command list");
  ID3D12CommandList* submitted[] = {commands.Get()};
  queue.ExecuteCommandLists(1, submitted);
  wait_for_queue(device, queue);

  const D3D12_RANGE read_range{0, static_cast<SIZE_T>(total_size)};
  void* mapped_bytes = nullptr;
  require_hresult(
      readback->Map(0, &read_range, &mapped_bytes),
      "D3D12 qualification could not map its readback buffer");
  const auto* mapped = static_cast<const std::uint8_t*>(mapped_bytes);
  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(width) * height * 4U);
  for (std::uint32_t row = 0; row < height; ++row) {
    std::memcpy(pixels.data() + static_cast<std::size_t>(row) * width * 4U,
                mapped + footprint.Offset +
                    static_cast<std::size_t>(row) * footprint.Footprint.RowPitch,
                static_cast<std::size_t>(width) * 4U);
  }
  constexpr D3D12_RANGE no_write{0, 0};
  readback->Unmap(0, &no_write);
  return pixels;
}

void qualify_pixels(const std::vector<std::uint8_t>& pixels) {
  std::size_t opaque = 0;
  std::size_t changed = 0;
  for (std::size_t index = 0; index + 3 < pixels.size(); index += 4) {
    const auto blue = pixels[index];
    const auto green = pixels[index + 1];
    const auto red = pixels[index + 2];
    opaque += pixels[index + 3] == 255 ? 1U : 0U;
    const auto delta = std::abs(static_cast<int>(red) - 10) +
                       std::abs(static_cast<int>(green) - 16) +
                       std::abs(static_cast<int>(blue) - 32);
    changed += delta > 24 ? 1U : 0U;
  }
  require(opaque == pixels.size() / 4,
          "D3D12 qualification output is not fully opaque");
  require(changed > 18'000,
          "D3D12 qualification foreground coverage is too small");
}

void write_reference_ppm(const std::string& path,
                         const std::vector<std::uint8_t>& pixels,
                         const std::uint32_t width,
                         const std::uint32_t height) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  require(static_cast<bool>(output),
          "D3D12 qualification could not open the requested capture");
  output << "P6\n" << width << ' ' << height << "\n255\n";
  for (std::size_t index = 0; index + 3 < pixels.size(); index += 4) {
    const char rgb[] = {static_cast<char>(pixels[index + 2]),
                        static_cast<char>(pixels[index + 1]),
                        static_cast<char>(pixels[index])};
    output.write(rgb, sizeof(rgb));
  }
  require(static_cast<bool>(output),
          "D3D12 qualification could not write the requested capture");
}

}  // namespace

int main() {
  using refusion::runtime::presentation::PresentationFrameRequest;
  using refusion::runtime::render::VisualOutputConsumer;
  constexpr std::uint32_t width = 640;
  constexpr std::uint32_t height = 360;

  auto device_service = refusion::platform::create_platform_gpu_device_service();
  auto native = device_service->borrow();
  auto* device = static_cast<ID3D12Device*>(
      const_cast<void*>(native.backend_private_device()));
  auto* queue = static_cast<ID3D12CommandQueue*>(
      const_cast<void*>(native.backend_private_submission_queue()));
  require(device != nullptr && queue != nullptr,
          "D3D12 qualification received an empty device lease");

  auto renderer = refusion::adapters::skia::create_skia_gpu_composition(
      device_service->borrow(), nullptr, nullptr,
      refusion::tests::make_xplat_font_fixture_resolver());
  auto program = std::make_shared<const
      refusion::runtime::render::VisualRenderProgram>(
          conformance_render_program());

  const auto preview_texture = create_target_texture(*device, width, height);
  const auto preview_target = target_lease(
      device_service->identity(), 1, width, height, preview_texture);
  const auto preview_result = renderer->render(
      preview_target,
      PresentationFrameRequest{
          .request_sequence = 60,
          .project_time_ns = 1'000'000'000,
          .transport_epoch_id = 77,
          .device = device_service->identity(),
          .output_consumer = VisualOutputConsumer::interactive_preview,
          .render_program = program,
      });
  require(preview_result.succeeded(), preview_result.diagnostic);
  const auto preview_pixels =
      read_bgra(*device, *queue, *preview_texture.Get(), width, height);
  qualify_pixels(preview_pixels);

  const auto offline_texture = create_target_texture(*device, width, height);
  const auto offline_target = target_lease(
      device_service->identity(), 2, width, height, offline_texture);
  const auto offline_result = renderer->render(
      offline_target,
      PresentationFrameRequest{
          .request_sequence = 60,
          .project_time_ns = 1'000'000'000,
          .transport_epoch_id = 0,
          .device = device_service->identity(),
          .output_consumer = VisualOutputConsumer::offline_export,
          .render_program = program,
      });
  require(offline_result.succeeded(), offline_result.diagnostic);
  const auto offline_pixels =
      read_bgra(*device, *queue, *offline_texture.Get(), width, height);
  require(offline_pixels == preview_pixels,
          "D3D12 Preview and Offline qualification pixels differ");

  if (const char* capture_path =
          std::getenv("REFUSION_XPLAT_CAPTURE_PPM");
      capture_path != nullptr && capture_path[0] != '\0') {
    write_reference_ppm(capture_path, preview_pixels, width, height);
  }
}
