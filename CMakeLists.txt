cmake_minimum_required(VERSION 3.30)

project(ReFusion VERSION 0.0.1 LANGUAGES CXX)

option(REFUSION_BUILD_STUDIO "Build the Qt command-only Studio shell" OFF)
option(REFUSION_STRICT_WARNINGS "Treat warnings in ReFusion-owned code as errors" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(RefusionWarnings)
include(CTest)

add_subdirectory(src/core)
add_subdirectory(apps/cli)

if(REFUSION_BUILD_STUDIO)
  add_subdirectory(apps/studio)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests/unit)
endif()

