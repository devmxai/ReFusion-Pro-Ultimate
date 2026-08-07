add_library(refusion_platform_gpu STATIC
  D3D12GpuDeviceService.cpp
)

add_library(ReFusion::PlatformGpu ALIAS refusion_platform_gpu)

target_include_directories(refusion_platform_gpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
)

target_link_libraries(refusion_platform_gpu
  PUBLIC ReFusion::RuntimeGpu
  PRIVATE d3d12 dxgi dxguid
)

target_compile_features(refusion_platform_gpu PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_platform_gpu)
