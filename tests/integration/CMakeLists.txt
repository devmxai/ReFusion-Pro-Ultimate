if(TARGET ReFusion::PlatformGpu)
  add_executable(refusion-platform-gpu-tests platform_gpu_test.cpp)
  target_link_libraries(refusion-platform-gpu-tests PRIVATE ReFusion::PlatformGpu)
  refusion_enable_warnings(refusion-platform-gpu-tests)
  add_test(NAME refusion.platform_gpu COMMAND refusion-platform-gpu-tests)
endif()

if(TARGET ReFusion::SkiaAdapter)
  add_executable(refusion-skia-adapter-tests skia_adapter_test.cpp)
  target_link_libraries(refusion-skia-adapter-tests PRIVATE ReFusion::SkiaAdapter)
  if(APPLE AND TARGET ReFusion::PlatformGpu)
    target_link_libraries(refusion-skia-adapter-tests PRIVATE ReFusion::PlatformGpu)
    target_compile_definitions(refusion-skia-adapter-tests PRIVATE
      REFUSION_TEST_SKIA_GPU_CONTEXTS=1)

    add_executable(refusion-skia-fixture-renderer-tests
      skia_fixture_renderer_test.mm)
    target_link_libraries(refusion-skia-fixture-renderer-tests
      PRIVATE
        ReFusion::PlatformGpu
        ReFusion::SkiaAdapter
        "-framework Metal")
    target_compile_options(refusion-skia-fixture-renderer-tests PRIVATE -fobjc-arc)
    refusion_enable_warnings(refusion-skia-fixture-renderer-tests)
    add_test(NAME refusion.skia_fixture_renderer
      COMMAND refusion-skia-fixture-renderer-tests)

    add_executable(refusion-metal-viewport-presenter-tests
      metal_viewport_presenter_test.mm)
    target_link_libraries(refusion-metal-viewport-presenter-tests
      PRIVATE
        ReFusion::PlatformGpu
        ReFusion::SkiaAdapter
        "-framework AppKit"
        "-framework Metal"
        "-framework QuartzCore")
    target_compile_options(refusion-metal-viewport-presenter-tests PRIVATE -fobjc-arc)
    refusion_enable_warnings(refusion-metal-viewport-presenter-tests)
    add_test(NAME refusion.metal_viewport_presenter
      COMMAND refusion-metal-viewport-presenter-tests)
  endif()
  refusion_enable_warnings(refusion-skia-adapter-tests)
  add_test(NAME refusion.skia_adapter COMMAND refusion-skia-adapter-tests)
endif()

if(TARGET ReFusion::StudioBridge)
  add_executable(refusion-studio-bridge-tests studio_bridge_test.cpp)
  target_link_libraries(refusion-studio-bridge-tests PRIVATE ReFusion::StudioBridge)
  refusion_enable_warnings(refusion-studio-bridge-tests)
  add_test(NAME refusion.studio_bridge COMMAND refusion-studio-bridge-tests)
endif()

if(TARGET ReFusion::ProjectFileAdapter)
  add_executable(refusion-project-file-adapter-tests
    project_file_adapter_test.cpp)
  target_link_libraries(refusion-project-file-adapter-tests
    PRIVATE ReFusion::ProjectFileAdapter)
  target_compile_definitions(refusion-project-file-adapter-tests PRIVATE
    REFUSION_TEST_PROJECT_PATH="${CMAKE_SOURCE_DIR}/examples/projects/reels-demo-30s/ReFusionDemoReel.refusion.json")
  refusion_enable_warnings(refusion-project-file-adapter-tests)
  add_test(NAME refusion.project_file_adapter
    COMMAND refusion-project-file-adapter-tests)
endif()
