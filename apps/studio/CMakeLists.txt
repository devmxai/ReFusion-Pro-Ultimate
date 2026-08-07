if(REFUSION_RELEASE_BUILD)
  if(REFUSION_QT_COMMERCIAL_SDK_ROOT STREQUAL "")
    message(FATAL_ERROR
      "Release Studio requires REFUSION_QT_COMMERCIAL_SDK_ROOT beneath out/toolchains/qt-commercial")
  endif()
  if(NOT EXISTS "${REFUSION_QT_COMMERCIAL_RECEIPT}")
    message(FATAL_ERROR
      "Release Studio requires a private Qt Commercial entitlement receipt")
  endif()
  file(REAL_PATH "${CMAKE_SOURCE_DIR}/out/toolchains/qt-commercial"
       refusion_qt_admitted_root)
  file(REAL_PATH "${REFUSION_QT_COMMERCIAL_SDK_ROOT}" refusion_qt_sdk_root)
  cmake_path(IS_PREFIX refusion_qt_admitted_root "${refusion_qt_sdk_root}"
             NORMALIZE refusion_qt_is_local)
  if(NOT refusion_qt_is_local)
    message(FATAL_ERROR
      "Qt Commercial SDK must be materialized inside ReFusion out/toolchains/qt-commercial")
  endif()
  list(PREPEND CMAKE_PREFIX_PATH "${refusion_qt_sdk_root}")
endif()

find_package(Qt6 6.11.1 EXACT REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

if(REFUSION_RELEASE_BUILD)
  file(REAL_PATH "${Qt6_DIR}" refusion_qt6_dir)
  cmake_path(IS_PREFIX refusion_qt_sdk_root "${refusion_qt6_dir}"
             NORMALIZE refusion_qt_matches_sdk)
  if(NOT refusion_qt_matches_sdk)
    message(FATAL_ERROR
      "Release build resolved Qt outside the admitted Commercial SDK: ${Qt6_DIR}")
  endif()
  file(READ "${REFUSION_QT_COMMERCIAL_RECEIPT}" refusion_qt_receipt)
  string(JSON refusion_qt_receipt_status GET "${refusion_qt_receipt}" status)
  string(JSON refusion_qt_receipt_lane GET "${refusion_qt_receipt}" license_lane)
  string(JSON refusion_qt_receipt_version GET "${refusion_qt_receipt}" qt_version)
  if(NOT refusion_qt_receipt_status STREQUAL "approved" OR
     NOT refusion_qt_receipt_lane STREQUAL "commercial" OR
     NOT refusion_qt_receipt_version STREQUAL "6.11.1")
    message(FATAL_ERROR
      "Qt receipt does not approve Commercial Qt 6.11.1 for this release build")
  endif()
endif()

qt_standard_project_setup(REQUIRES 6.11)

add_library(refusion_studio_bridge STATIC
  StudioBridge.cpp
  StudioBridge.hpp
)

add_library(ReFusion::StudioBridge ALIAS refusion_studio_bridge)

target_include_directories(refusion_studio_bridge
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(refusion_studio_bridge
  PUBLIC
    ReFusion::Application
    Qt6::Core
)

refusion_enable_warnings(refusion_studio_bridge)

add_library(refusion_project_file_adapter STATIC
  adapters/QtJsonProjectFileAdapter.cpp
  adapters/QtJsonProjectFileAdapter.hpp
)

add_library(ReFusion::ProjectFileAdapter ALIAS refusion_project_file_adapter)

target_include_directories(refusion_project_file_adapter
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(refusion_project_file_adapter
  PUBLIC
    ReFusion::Application
    Qt6::Core
)

refusion_enable_warnings(refusion_project_file_adapter)

set(refusion_studio_sources
  main.cpp
  StudioRuntimeComposition.hpp
)

set(refusion_studio_visual_runtime OFF)
if(APPLE AND TARGET ReFusion::PlatformGpu AND TARGET ReFusion::SkiaAdapter)
  list(APPEND refusion_studio_sources
    EngineViewportWindow.cpp
    EngineViewportWindow.hpp
    composition/VisualRuntimeComposition.cpp
  )
  set(refusion_studio_visual_runtime ON)
else()
  list(APPEND refusion_studio_sources composition/NullRuntimeComposition.cpp)
endif()

qt_add_executable(refusion-studio ${refusion_studio_sources})

target_compile_definitions(refusion-studio PRIVATE
  REFUSION_DEFAULT_PROJECT_PATH="${CMAKE_SOURCE_DIR}/examples/projects/reels-demo-30s/ReFusionDemoReel.refusion.json"
)

target_include_directories(refusion-studio
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

qt_add_qml_module(refusion-studio
  URI ReFusion.Studio
  VERSION 1.0
  QML_FILES Main.qml
)

target_link_libraries(refusion-studio
  PRIVATE
    ReFusion::StudioBridge
    ReFusion::ProjectFileAdapter
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

if(refusion_studio_visual_runtime)
  target_link_libraries(refusion-studio
    PRIVATE
      ReFusion::PlatformGpu
      ReFusion::SkiaAdapter
  )
endif()

refusion_enable_warnings(refusion-studio)
