add_library(refusion_platform_gpu STATIC
  D3D12GpuDeviceService.cpp
  DxgiViewportPresenter.cpp
)

add_library(ReFusion::PlatformGpu ALIAS refusion_platform_gpu)

target_include_directories(refusion_platform_gpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
)

target_link_libraries(refusion_platform_gpu
  PUBLIC
    ReFusion::RuntimeGpu
    ReFusion::RuntimePresentation
  PRIVATE d3d12 dxgi dxguid
)

target_compile_features(refusion_platform_gpu PUBLIC cxx_std_20)
target_compile_definitions(refusion_platform_gpu PRIVATE
  WIN32_LEAN_AND_MEAN NOMINMAX)
refusion_enable_warnings(refusion_platform_gpu)
