include(Skia)
refusion_import_skia()

set(refusion_skia_adapter_sources SkiaRuntime.cpp)
if(APPLE)
  list(APPEND refusion_skia_adapter_sources SkiaGpuContextsMetal.mm)
endif()

add_library(refusion_skia_adapter STATIC ${refusion_skia_adapter_sources})

add_library(ReFusion::SkiaAdapter ALIAS refusion_skia_adapter)

target_include_directories(refusion_skia_adapter
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(refusion_skia_adapter
  PUBLIC ReFusion::RuntimeGpu
  PRIVATE Skia::Skia
)

target_compile_features(refusion_skia_adapter PUBLIC cxx_std_20)
if(APPLE)
  target_compile_options(refusion_skia_adapter PRIVATE
    $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
endif()
refusion_enable_warnings(refusion_skia_adapter)
