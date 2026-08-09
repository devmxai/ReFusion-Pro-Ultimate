if(TARGET ReFusion::PlatformGpu)
  add_executable(refusion-platform-gpu-tests platform_gpu_test.cpp)
  target_link_libraries(refusion-platform-gpu-tests PRIVATE ReFusion::PlatformGpu)
  refusion_enable_warnings(refusion-platform-gpu-tests)
  add_test(NAME refusion.platform_gpu COMMAND refusion-platform-gpu-tests)

  if(APPLE)
    add_executable(refusion-macos-gpu-lifecycle-notification-tests
      macos_gpu_lifecycle_notification_test.mm)
    target_link_libraries(refusion-macos-gpu-lifecycle-notification-tests
      PRIVATE ReFusion::PlatformGpu "-framework AppKit")
    target_compile_options(refusion-macos-gpu-lifecycle-notification-tests
      PRIVATE -fobjc-arc)
    refusion_enable_warnings(refusion-macos-gpu-lifecycle-notification-tests)
    add_test(NAME refusion.macos_gpu_lifecycle_notifications
      COMMAND refusion-macos-gpu-lifecycle-notification-tests)
  endif()
endif()

add_test(NAME refusion.cli_agent_surface
  COMMAND ${CMAKE_COMMAND}
    -DCLI=$<TARGET_FILE:refusion-cli>
    -DPROJECT=${CMAKE_SOURCE_DIR}/examples/projects/rfx-authoring-experiment-01/Project.rfx
    -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cli-agent-surface
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_agent_surface_test.cmake)

if(TARGET ReFusion::FfmpegDemux)
  add_executable(refusion-ffmpeg-media-demux-tests
    ffmpeg_media_demux_test.cpp)
  target_link_libraries(refusion-ffmpeg-media-demux-tests
    PRIVATE ReFusion::FfmpegDemux ReFusion::RuntimeMedia)
  target_compile_definitions(refusion-ffmpeg-media-demux-tests PRIVATE
    REFUSION_VIDEO_IMPORT_FIXTURE_ROOT="${CMAKE_SOURCE_DIR}/tests/fixtures/media/video-import-v1")
  refusion_enable_warnings(refusion-ffmpeg-media-demux-tests)
  add_test(NAME refusion.ffmpeg_media_demux
    COMMAND refusion-ffmpeg-media-demux-tests)

  if(TARGET ReFusion::ProjectFileAdapter)
    add_executable(refusion-video-import-transaction-tests
      video_import_transaction_test.cpp)
    target_link_libraries(refusion-video-import-transaction-tests
      PRIVATE ReFusion::FfmpegDemux ReFusion::ProjectFileAdapter)
    target_compile_definitions(refusion-video-import-transaction-tests PRIVATE
      REFUSION_VIDEO_IMPORT_SOURCE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/media/video-import-v1/mp4-vfr-bframes-aac-offset/source.mp4")
    refusion_enable_warnings(refusion-video-import-transaction-tests)
    add_test(NAME refusion.video_import_transaction
      COMMAND refusion-video-import-transaction-tests)
  endif()
endif()

if(APPLE AND TARGET ReFusion::PlatformMedia)
  add_executable(refusion-apple-media-capability-tests
    apple_media_capability_test.cpp)
  target_link_libraries(refusion-apple-media-capability-tests
    PRIVATE ReFusion::PlatformGpu ReFusion::PlatformMedia)
  refusion_enable_warnings(refusion-apple-media-capability-tests)
  add_test(NAME refusion.apple_media_capability
    COMMAND refusion-apple-media-capability-tests)

  add_executable(refusion-apple-hardware-decode-tests
    apple_hardware_decode_test.cpp)
  target_link_libraries(refusion-apple-hardware-decode-tests
    PRIVATE ReFusion::PlatformGpu ReFusion::PlatformMedia)
  target_compile_definitions(refusion-apple-hardware-decode-tests PRIVATE
    REFUSION_TEST_H264_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/media/h264-cfr-320x180-gop1/video.h264")
  refusion_enable_warnings(refusion-apple-hardware-decode-tests)
  add_test(NAME refusion.apple_hardware_decode
    COMMAND refusion-apple-hardware-decode-tests)

  add_executable(refusion-apple-hardware-decode-sequence-tests
    apple_hardware_decode_sequence_test.cpp)
  target_link_libraries(refusion-apple-hardware-decode-sequence-tests
    PRIVATE ReFusion::PlatformGpu ReFusion::PlatformMedia)
  target_compile_definitions(
    refusion-apple-hardware-decode-sequence-tests PRIVATE
    REFUSION_TEST_H264_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/media/h264-cfr-320x180-gop1/video.h264")
  refusion_enable_warnings(refusion-apple-hardware-decode-sequence-tests)
  add_test(NAME refusion.apple_hardware_decode_sequence
    COMMAND refusion-apple-hardware-decode-sequence-tests)

  add_executable(refusion-apple-hardware-decode-seek-tests
    apple_hardware_decode_seek_test.cpp)
  target_link_libraries(refusion-apple-hardware-decode-seek-tests
    PRIVATE ReFusion::Core ReFusion::PlatformGpu ReFusion::PlatformMedia)
  target_compile_definitions(
    refusion-apple-hardware-decode-seek-tests PRIVATE
    REFUSION_TEST_H264_SEEK_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/media/h264-vfr-320x180-long-gop-bframes/video.h264")
  refusion_enable_warnings(refusion-apple-hardware-decode-seek-tests)
  add_test(NAME refusion.apple_hardware_decode_seek
    COMMAND refusion-apple-hardware-decode-seek-tests)
endif()

if(TARGET ReFusion::SkiaAdapter)
  add_executable(refusion-skia-text-layout-tests skia_text_layout_test.cpp)
  target_link_libraries(refusion-skia-text-layout-tests
    PRIVATE ReFusion::SkiaAdapter)
  refusion_deploy_skia_runtime(refusion-skia-text-layout-tests)
  target_compile_definitions(refusion-skia-text-layout-tests PRIVATE
    REFUSION_NOTO_SANS_LATIN_PATH="${REFUSION_DEPENDENCY_SOURCE_DIR}/noto_sans_latin_baseline/font.ttf"
    REFUSION_NOTO_SANS_ARABIC_PATH="${REFUSION_DEPENDENCY_SOURCE_DIR}/noto_sans_arabic_baseline/font.ttf"
    REFUSION_NOTO_LAYOUT_RECEIPT_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/fonts/xplat-noto-v1/expected-layout.txt")
  refusion_enable_warnings(refusion-skia-text-layout-tests)
  add_test(NAME refusion.skia_text_layout
    COMMAND refusion-skia-text-layout-tests)
  add_test(NAME refusion.cli_measure
    COMMAND $<TARGET_FILE:refusion-cli> measure
      ${CMAKE_SOURCE_DIR}/examples/projects/rfx-authoring-experiment-01/Project.rfx
      500000000)
  add_test(NAME refusion.cli_agent_measured_alignment
    COMMAND ${CMAKE_COMMAND}
      -DCLI=$<TARGET_FILE:refusion-cli>
      -DPROJECT=${CMAKE_SOURCE_DIR}/examples/projects/rfx-authoring-experiment-01/Project.rfx
      -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cli-agent-measurement
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_agent_measurement_test.cmake)

  add_executable(refusion-exp006g-visual-regression-tests
    exp006g_visual_regression_test.cpp)
  target_link_libraries(refusion-exp006g-visual-regression-tests
    PRIVATE ReFusion::SkiaAdapter)
  refusion_deploy_skia_runtime(refusion-exp006g-visual-regression-tests)
  target_compile_definitions(refusion-exp006g-visual-regression-tests PRIVATE
    REFUSION_EXP006G_PROJECT_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/project/exp006g-reels/Project.rfx")
  refusion_enable_warnings(refusion-exp006g-visual-regression-tests)
  add_test(NAME refusion.exp006g_visual_regression
    COMMAND refusion-exp006g-visual-regression-tests)

  add_executable(refusion-skia-adapter-tests skia_adapter_test.cpp)
  target_link_libraries(refusion-skia-adapter-tests PRIVATE ReFusion::SkiaAdapter)
  refusion_deploy_skia_runtime(refusion-skia-adapter-tests)
  if((APPLE OR WIN32) AND TARGET ReFusion::PlatformGpu)
    target_link_libraries(refusion-skia-adapter-tests PRIVATE ReFusion::PlatformGpu)
    target_compile_definitions(refusion-skia-adapter-tests PRIVATE
      REFUSION_TEST_SKIA_GPU_CONTEXTS=1)

    if(WIN32)
      add_executable(refusion-d3d12-fixture-renderer-tests
        d3d12_fixture_renderer_test.cpp)
      target_link_libraries(refusion-d3d12-fixture-renderer-tests
        PRIVATE
          ReFusion::PlatformGpu
          ReFusion::SkiaAdapter
          d3d12
          dxgi)
      refusion_deploy_skia_runtime(refusion-d3d12-fixture-renderer-tests)
      target_compile_definitions(refusion-d3d12-fixture-renderer-tests PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        REFUSION_XPLAT_VISUAL_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/Project.rfx"
        REFUSION_NOTO_SANS_ARABIC_PATH="${REFUSION_DEPENDENCY_SOURCE_DIR}/noto_sans_arabic_baseline/font.ttf")
      refusion_enable_warnings(refusion-d3d12-fixture-renderer-tests)
      add_test(NAME refusion.d3d12_fixture_renderer
        COMMAND refusion-d3d12-fixture-renderer-tests)

      add_executable(refusion-d3d12-viewport-presenter-tests
        d3d12_viewport_presenter_test.cpp)
      target_link_libraries(refusion-d3d12-viewport-presenter-tests
        PRIVATE
          ReFusion::PlatformGpu
          ReFusion::SkiaAdapter
          d3d12
          dxgi)
      refusion_deploy_skia_runtime(refusion-d3d12-viewport-presenter-tests)
      target_compile_definitions(refusion-d3d12-viewport-presenter-tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
      refusion_enable_warnings(refusion-d3d12-viewport-presenter-tests)
      add_test(NAME refusion.d3d12_viewport_presenter
        COMMAND refusion-d3d12-viewport-presenter-tests)
    endif()

    if(APPLE)
    add_executable(refusion-skia-fixture-renderer-tests
      skia_fixture_renderer_test.mm)
    target_link_libraries(refusion-skia-fixture-renderer-tests
      PRIVATE
        ReFusion::PlatformGpu
        ReFusion::SkiaAdapter
        "-framework Metal")
    target_compile_options(refusion-skia-fixture-renderer-tests PRIVATE -fobjc-arc)
    target_compile_definitions(refusion-skia-fixture-renderer-tests PRIVATE
      REFUSION_XPLAT_VISUAL_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/Project.rfx"
      REFUSION_NOTO_SANS_ARABIC_PATH="${REFUSION_DEPENDENCY_SOURCE_DIR}/noto_sans_arabic_baseline/font.ttf")
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

    if(TARGET ReFusion::PlatformMedia)
      add_executable(refusion-skia-decoded-video-surface-tests
        skia_decoded_video_surface_test.mm)
      target_link_libraries(refusion-skia-decoded-video-surface-tests
        PRIVATE
          ReFusion::PlatformGpu
          ReFusion::PlatformMedia
          ReFusion::SkiaAdapter
          "-framework Metal")
      target_compile_definitions(
        refusion-skia-decoded-video-surface-tests PRIVATE
        REFUSION_TEST_H264_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/media/h264-vfr-320x180-long-gop-bframes/video.h264")
      target_compile_options(refusion-skia-decoded-video-surface-tests
        PRIVATE -fobjc-arc)
      refusion_enable_warnings(refusion-skia-decoded-video-surface-tests)
      add_test(NAME refusion.skia_decoded_video_surface
        COMMAND refusion-skia-decoded-video-surface-tests)

      add_executable(refusion-gpu-observability-integration-tests
        gpu_observability_integration_test.mm)
      target_link_libraries(refusion-gpu-observability-integration-tests
        PRIVATE
          ReFusion::PlatformGpu
          ReFusion::PlatformMedia
          ReFusion::SkiaAdapter
          "-framework AppKit"
          "-framework Metal"
          "-framework QuartzCore")
      target_compile_definitions(
        refusion-gpu-observability-integration-tests PRIVATE
        REFUSION_TEST_H264_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/media/h264-vfr-320x180-long-gop-bframes/video.h264")
      target_compile_options(refusion-gpu-observability-integration-tests
        PRIVATE -fobjc-arc)
      refusion_enable_warnings(refusion-gpu-observability-integration-tests)
      add_test(NAME refusion.gpu_observability_integration
        COMMAND refusion-gpu-observability-integration-tests)
    endif()
    endif()
  endif()
  refusion_enable_warnings(refusion-skia-adapter-tests)
  add_test(NAME refusion.skia_adapter COMMAND refusion-skia-adapter-tests)
endif()

if(TARGET ReFusion::StudioBridge)
  add_executable(refusion-studio-bridge-tests studio_bridge_test.cpp)
  target_link_libraries(refusion-studio-bridge-tests PRIVATE ReFusion::StudioBridge)
  refusion_enable_warnings(refusion-studio-bridge-tests)
  add_test(NAME refusion.studio_bridge COMMAND refusion-studio-bridge-tests)

  add_executable(refusion-exp006g-authoring-parity-tests
    exp006g_authoring_parity_test.cpp)
  target_link_libraries(refusion-exp006g-authoring-parity-tests
    PRIVATE ReFusion::StudioBridge)
  target_compile_definitions(refusion-exp006g-authoring-parity-tests PRIVATE
    REFUSION_EXP006G_PROJECT_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/project/exp006g-reels/Project.rfx")
  refusion_enable_warnings(refusion-exp006g-authoring-parity-tests)
  add_test(NAME refusion.exp006g_authoring_parity
    COMMAND refusion-exp006g-authoring-parity-tests)
endif()

if(TARGET ReFusion::ProjectFileAdapter)
  add_executable(refusion-project-file-adapter-tests
    project_file_adapter_test.cpp)
  target_link_libraries(refusion-project-file-adapter-tests
    PRIVATE ReFusion::ProjectFileAdapter)
  target_compile_definitions(refusion-project-file-adapter-tests PRIVATE
    REFUSION_TEST_PROJECT_PATH="${CMAKE_SOURCE_DIR}/examples/projects/rfx-authoring-experiment-01/Project.rfx")
  refusion_enable_warnings(refusion-project-file-adapter-tests)
  add_test(NAME refusion.project_file_adapter
    COMMAND refusion-project-file-adapter-tests)

  add_executable(refusion-project-workspace-creator-tests
    project_workspace_creator_test.cpp)
  target_link_libraries(refusion-project-workspace-creator-tests
    PRIVATE ReFusion::ProjectFileAdapter)
  refusion_enable_warnings(refusion-project-workspace-creator-tests)
  add_test(NAME refusion.project_workspace_creator
    COMMAND refusion-project-workspace-creator-tests)
endif()

if(TARGET ReFusion::ProjectLauncher)
  add_executable(refusion-project-launcher-tests
    project_launcher_test.cpp)
  target_link_libraries(refusion-project-launcher-tests
    PRIVATE ReFusion::ProjectLauncher)
  refusion_enable_warnings(refusion-project-launcher-tests)
  add_test(NAME refusion.project_launcher
    COMMAND refusion-project-launcher-tests)
endif()

if(TARGET ReFusion::ProjectLiveReload)
  add_executable(refusion-project-live-reload-tests
    project_live_reload_test.cpp)
  target_link_libraries(refusion-project-live-reload-tests
    PRIVATE ReFusion::ProjectLiveReload)
  refusion_enable_warnings(refusion-project-live-reload-tests)
  add_test(NAME refusion.project_live_reload
    COMMAND refusion-project-live-reload-tests)
endif()

get_property(refusion_qt_runtime_directory GLOBAL
  PROPERTY REFUSION_QT_RUNTIME_DIRECTORY)
if(WIN32 AND refusion_qt_runtime_directory)
  set_tests_properties(
    refusion.studio_bridge
    refusion.exp006g_authoring_parity
    refusion.project_file_adapter
    refusion.project_workspace_creator
    refusion.project_launcher
    refusion.project_live_reload
    PROPERTIES
      ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${refusion_qt_runtime_directory}")
endif()
