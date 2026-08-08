add_library(refusion_application STATIC
  AgentAuthoringGuide.cpp
  ApplicationHost.cpp
  ProjectCreationService.cpp
  VisualContributionCommands.cpp
)

add_library(ReFusion::Application ALIAS refusion_application)

target_include_directories(refusion_application
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(refusion_application PUBLIC ReFusion::Core)
target_compile_features(refusion_application PUBLIC cxx_std_20)
refusion_enable_warnings(refusion_application)
