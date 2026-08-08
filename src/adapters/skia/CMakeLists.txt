include(Skia)
refusion_import_skia()

if(APPLE)
  set(refusion_skia_system_font_source
    system_fonts/SkiaSystemFontProviderApple.mm)
elseif(WIN32)
  set(refusion_skia_system_font_source
    system_fonts/SkiaSystemFontProviderWindows.cpp)
elseif(ANDROID)
  set(refusion_skia_system_font_source
    system_fonts/SkiaSystemFontProviderAndroid.cpp)
else()
  set(refusion_skia_system_font_source
    system_fonts/SkiaSystemFontProviderUnsupported.cpp)
endif()

add_library(refusion_skia_system_fonts STATIC
  ${refusion_skia_system_font_source})
add_library(ReFusion::SkiaSystemFonts ALIAS refusion_skia_system_fonts)
target_include_directories(refusion_skia_system_fonts
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(refusion_skia_system_fonts PRIVATE Skia::Skia)
target_compile_features(refusion_skia_system_fonts PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_skia_system_fonts)

set(refusion_skia_common_sources
  SkiaRuntime.cpp
  SkiaGpuComposition.cpp
  SkiaSceneCompositor.cpp
  SkiaTextLayout.cpp
  SkiaVisualProgramExecutor.cpp
)

add_library(refusion_skia_common STATIC ${refusion_skia_common_sources})
add_library(ReFusion::SkiaCommon ALIAS refusion_skia_common)

target_include_directories(refusion_skia_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(refusion_skia_common
  PUBLIC
    ReFusion::Core
    ReFusion::RuntimeGpu
    ReFusion::RuntimeMedia
    ReFusion::RuntimePresentation
    ReFusion::RuntimeRender
  PRIVATE Skia::Skia ReFusion::SkiaSystemFonts
)

target_compile_features(refusion_skia_common PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_skia_common)

add_library(refusion_skia_adapter INTERFACE)
add_library(ReFusion::SkiaAdapter ALIAS refusion_skia_adapter)
target_link_libraries(refusion_skia_adapter INTERFACE ReFusion::SkiaCommon)

if(APPLE)
  add_subdirectory(backends/metal)
  target_link_libraries(refusion_skia_adapter
    INTERFACE ReFusion::SkiaMetalBackend)
elseif(WIN32)
  add_subdirectory(backends/d3d12)
  target_link_libraries(refusion_skia_adapter
    INTERFACE ReFusion::SkiaD3D12Backend)
elseif(ANDROID)
  add_subdirectory(backends/vulkan)
  target_link_libraries(refusion_skia_adapter
    INTERFACE ReFusion::SkiaVulkanContractCanary)
endif()
