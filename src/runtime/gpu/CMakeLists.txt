add_library(refusion_runtime_gpu STATIC
  GpuDeviceService.cpp
  GpuObservability.cpp
)

add_library(ReFusion::RuntimeGpu ALIAS refusion_runtime_gpu)

target_include_directories(refusion_runtime_gpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(refusion_runtime_gpu PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_runtime_gpu)
