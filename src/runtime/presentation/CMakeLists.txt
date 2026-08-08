add_library(refusion_runtime_presentation STATIC
  ViewportPresentation.cpp
)

add_library(ReFusion::RuntimePresentation ALIAS refusion_runtime_presentation)

find_package(Threads REQUIRED)

target_include_directories(refusion_runtime_presentation
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(refusion_runtime_presentation
  PUBLIC ReFusion::Core ReFusion::RuntimeGpu ReFusion::RuntimeRender
  PRIVATE Threads::Threads
)

target_compile_features(refusion_runtime_presentation PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_runtime_presentation)
