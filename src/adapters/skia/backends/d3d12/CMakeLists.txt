add_library(refusion_skia_d3d12_backend STATIC
  ../../SkiaGpuContextsD3D12.cpp
)

add_library(ReFusion::SkiaD3D12Backend ALIAS refusion_skia_d3d12_backend)

target_include_directories(refusion_skia_d3d12_backend
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(refusion_skia_d3d12_backend
  PUBLIC ReFusion::SkiaCommon
  PRIVATE Skia::Skia d3d12 dxgi dxguid
)

target_compile_features(refusion_skia_d3d12_backend PUBLIC cxx_std_20)
target_compile_definitions(refusion_skia_d3d12_backend PRIVATE
  WIN32_LEAN_AND_MEAN NOMINMAX)
refusion_enable_warnings(refusion_skia_d3d12_backend)
