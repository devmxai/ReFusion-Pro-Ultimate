set(refusion_cli_atomic_adapter adapters/AtomicProjectFilePosix.cpp)
if(WIN32)
  set(refusion_cli_atomic_adapter adapters/AtomicProjectFileWindows.cpp)
endif()

add_executable(refusion-cli
  AgentJson.cpp
  adapters/AtomicProjectFile.cpp
  ${refusion_cli_atomic_adapter}
  main.cpp)
target_link_libraries(refusion-cli PRIVATE ReFusion::Application)
if(TARGET ReFusion::SkiaAdapter)
  target_link_libraries(refusion-cli PRIVATE ReFusion::SkiaAdapter)
  target_compile_definitions(refusion-cli PRIVATE
    REFUSION_CLI_SKIA_MEASUREMENT=1)
  refusion_deploy_skia_runtime(refusion-cli)
endif()
refusion_enable_warnings(refusion-cli)
