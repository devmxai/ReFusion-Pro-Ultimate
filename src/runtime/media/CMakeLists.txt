add_library(refusion_runtime_media STATIC
  HardwareVideoDecode.cpp
  MediaCapability.cpp
)

add_library(ReFusion::RuntimeMedia ALIAS refusion_runtime_media)

target_include_directories(refusion_runtime_media
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(refusion_runtime_media PUBLIC ReFusion::RuntimeGpu)
target_compile_features(refusion_runtime_media PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_runtime_media)
