find_package(Qt6 6.11.1 EXACT REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.11)

qt_add_executable(refusion-studio
  main.cpp
  StudioBridge.cpp
  StudioBridge.hpp
)

qt_add_qml_module(refusion-studio
  URI ReFusion.Studio
  VERSION 1.0
  QML_FILES Main.qml
)

target_link_libraries(refusion-studio
  PRIVATE
    ReFusion::Core
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

refusion_enable_warnings(refusion-studio)

