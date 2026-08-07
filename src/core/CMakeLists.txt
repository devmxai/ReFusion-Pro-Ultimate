add_library(refusion_core STATIC
  ProjectDocument.cpp
  ProjectAuthority.cpp
  ProjectClock.cpp
)

add_library(ReFusion::Core ALIAS refusion_core)

target_include_directories(refusion_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(refusion_core PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_core)
