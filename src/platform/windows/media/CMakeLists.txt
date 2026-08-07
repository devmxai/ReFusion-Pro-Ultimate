add_library(refusion_platform_media STATIC
  WindowsMediaCapabilityProbe.cpp
)

add_library(ReFusion::PlatformMedia ALIAS refusion_platform_media)

target_include_directories(refusion_platform_media
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
)

target_link_libraries(refusion_platform_media
  PUBLIC
    ReFusion::RuntimeGpu
    ReFusion::RuntimeMedia
)

target_compile_features(refusion_platform_media PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_platform_media)
