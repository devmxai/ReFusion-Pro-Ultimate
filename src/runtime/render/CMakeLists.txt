add_library(refusion_runtime_render STATIC
  RenderPlanCompiler.cpp
  VisualOutputContract.cpp
)

add_library(ReFusion::RuntimeRender ALIAS refusion_runtime_render)

target_include_directories(refusion_runtime_render
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(refusion_runtime_render PUBLIC ReFusion::Core)
target_compile_features(refusion_runtime_render PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_runtime_render)
