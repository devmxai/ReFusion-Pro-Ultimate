add_library(refusion_platform_gpu STATIC
  MetalGpuDeviceService.mm
)

add_library(ReFusion::PlatformGpu ALIAS refusion_platform_gpu)

target_include_directories(refusion_platform_gpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
)

target_link_libraries(refusion_platform_gpu
  PUBLIC ReFusion::RuntimeGpu
  PRIVATE "-framework Foundation" "-framework Metal"
)

target_compile_features(refusion_platform_gpu PUBLIC cxx_std_20)
target_compile_options(refusion_platform_gpu PRIVATE -fobjc-arc)
refusion_enable_warnings(refusion_platform_gpu)
