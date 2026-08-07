add_library(refusion_platform_media STATIC
  AppleMediaCapabilityProbe.mm
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
  PRIVATE
    "-framework CoreMedia"
    "-framework CoreVideo"
    "-framework Foundation"
    "-framework Metal"
    "-framework VideoToolbox"
)

target_compile_features(refusion_platform_media PUBLIC cxx_std_20)
target_compile_options(refusion_platform_media PRIVATE -fobjc-arc)
refusion_enable_warnings(refusion_platform_media)
