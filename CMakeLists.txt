cmake_minimum_required(VERSION 3.30)

project(ReFusion VERSION 0.0.1 LANGUAGES CXX)

option(REFUSION_BUILD_STUDIO "Build the Qt command-only Studio shell" OFF)
option(REFUSION_BUILD_CLI "Build the host command-line application" ON)
option(REFUSION_BUILD_PLATFORM_GPU "Build the native engine-owned GPU service" OFF)
option(REFUSION_BUILD_PLATFORM_MEDIA
       "Build the strict native hardware-media capability probe" OFF)
option(REFUSION_ENABLE_SKIA "Build the Skia adapter from a verified local source tree" OFF)
option(REFUSION_STRICT_WARNINGS "Treat warnings in ReFusion-owned code as errors" ON)
option(REFUSION_RELEASE_BUILD "Enable fail-closed redistributable build gates" OFF)
set(REFUSION_QT_COMMERCIAL_SDK_ROOT "" CACHE PATH
    "ReFusion-local Qt Commercial SDK root admitted for release builds")
set(REFUSION_QT_COMMERCIAL_RECEIPT "" CACHE FILEPATH
    "Private Qt Commercial entitlement receipt admitted for release builds")

if(APPLE AND (REFUSION_BUILD_PLATFORM_GPU OR REFUSION_BUILD_PLATFORM_MEDIA OR
              REFUSION_ENABLE_SKIA))
  enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps"
)
include(RefusionWarnings)
include(CTest)

add_subdirectory(src/core)
add_subdirectory(src/application)
add_subdirectory(src/runtime)

if(REFUSION_BUILD_PLATFORM_GPU OR REFUSION_BUILD_PLATFORM_MEDIA)
  add_subdirectory(src/platform)
endif()

if(REFUSION_ENABLE_SKIA)
  add_subdirectory(src/adapters)
endif()

if(REFUSION_BUILD_CLI)
  add_subdirectory(apps/cli)
endif()

if(REFUSION_BUILD_STUDIO)
  add_subdirectory(apps/studio)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests/unit)
  add_subdirectory(tests/integration)
  add_subdirectory(tests/tools)
endif()
