add_library(refusion_skia_metal_backend STATIC
  ../../SkiaGpuContextsMetal.mm
)

add_library(ReFusion::SkiaMetalBackend ALIAS refusion_skia_metal_backend)

target_include_directories(refusion_skia_metal_backend
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(refusion_skia_metal_backend
  PUBLIC ReFusion::SkiaCommon
  PRIVATE Skia::Skia
)

if(TARGET ReFusion::PlatformMedia)
  target_link_libraries(refusion_skia_metal_backend
    PRIVATE ReFusion::PlatformMedia)
  target_compile_definitions(refusion_skia_metal_backend PRIVATE
    REFUSION_SKIA_APPLE_MEDIA=1)
endif()

target_compile_features(refusion_skia_metal_backend PUBLIC cxx_std_20)
target_compile_options(refusion_skia_metal_backend PRIVATE -fobjc-arc)
refusion_enable_warnings(refusion_skia_metal_backend)
