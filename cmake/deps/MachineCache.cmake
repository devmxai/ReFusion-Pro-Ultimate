include_guard(GLOBAL)

function(refusion_machine_cache_arguments output)
  set(arguments machine-cache)
  if(NOT REFUSION_MACHINE_CACHE_ROOT STREQUAL "")
    list(APPEND arguments --root "${REFUSION_MACHINE_CACHE_ROOT}")
  endif()
  set(${output} "${arguments}" PARENT_SCOPE)
endfunction()

function(refusion_resolve_skia_machine_cache)
  set(REFUSION_MACHINE_CACHE_ACTIVE FALSE CACHE INTERNAL
      "A verified development machine-cache entry is active" FORCE)
  set(local_source "${CMAKE_SOURCE_DIR}/out/deps-src/skia")
  set(local_build
      "${CMAKE_SOURCE_DIR}/out/deps-build/skia/${REFUSION_SKIA_PROFILE}")
  if(REFUSION_SKIA_SOURCE_DIR STREQUAL local_source AND
     REFUSION_SKIA_BUILD_DIR STREQUAL local_build AND
     EXISTS "${local_source}" AND EXISTS "${local_build}")
    set(REFUSION_DEPENDENCY_SOURCE_DIR
        "${CMAKE_SOURCE_DIR}/out/deps-src" CACHE PATH
        "Verified dependency source root" FORCE)
    return()
  endif()

  set(should_resolve FALSE)
  if(REFUSION_SKIA_SOURCE_DIR STREQUAL local_source AND
     REFUSION_SKIA_BUILD_DIR STREQUAL local_build)
    set(should_resolve TRUE)
  elseif(NOT REFUSION_MACHINE_CACHE_ROOT STREQUAL "" AND
         EXISTS "${REFUSION_SKIA_SOURCE_DIR}" AND
         EXISTS "${REFUSION_SKIA_BUILD_DIR}")
    file(REAL_PATH "${REFUSION_MACHINE_CACHE_ROOT}" cache_root)
    file(REAL_PATH "${REFUSION_SKIA_SOURCE_DIR}" current_source)
    file(REAL_PATH "${REFUSION_SKIA_BUILD_DIR}" current_build)
    cmake_path(IS_PREFIX cache_root "${current_source}" NORMALIZE source_cached)
    cmake_path(IS_PREFIX cache_root "${current_build}" NORMALIZE build_cached)
    if(source_cached AND build_cached)
      set(should_resolve TRUE)
    endif()
  endif()
  if(NOT should_resolve)
    return()
  endif()

  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  refusion_machine_cache_arguments(cache_arguments)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/bootstrap.py"
            ${cache_arguments} resolve-skia --profile "${REFUSION_SKIA_PROFILE}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE cache_result
    OUTPUT_VARIABLE cache_output
    ERROR_VARIABLE cache_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT cache_result EQUAL 0)
    return()
  endif()

  string(JSON cache_root GET "${cache_output}" machine_cache_root)
  string(JSON source_cache GET "${cache_output}" source_cache)
  string(JSON skia_source GET "${cache_output}" skia_source)
  string(JSON skia_build GET "${cache_output}" skia_build)
  set(REFUSION_MACHINE_CACHE_ROOT "${cache_root}" CACHE PATH
      "Verified ReFusion machine dependency cache" FORCE)
  set(REFUSION_DEPENDENCY_SOURCE_DIR "${source_cache}" CACHE PATH
      "Verified dependency source root" FORCE)
  set(REFUSION_SKIA_SOURCE_DIR "${skia_source}" CACHE PATH
      "Verified official Skia source directory" FORCE)
  set(REFUSION_SKIA_BUILD_DIR "${skia_build}" CACHE PATH
      "Verified prebuilt Skia profile directory" FORCE)
  set(REFUSION_MACHINE_CACHE_ACTIVE TRUE CACHE INTERNAL
      "A verified development machine-cache entry is active" FORCE)
endfunction()

function(refusion_resolve_qt_machine_cache)
  if(REFUSION_RELEASE_BUILD OR Qt6_DIR OR CMAKE_PREFIX_PATH)
    return()
  endif()

  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  refusion_machine_cache_arguments(cache_arguments)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/bootstrap.py"
            ${cache_arguments} resolve-qt
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE cache_result
    OUTPUT_VARIABLE cache_output
    ERROR_VARIABLE cache_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT cache_result EQUAL 0)
    return()
  endif()

  string(JSON cache_root GET "${cache_output}" machine_cache_root)
  string(JSON qt_root GET "${cache_output}" qt_root)
  list(PREPEND CMAKE_PREFIX_PATH "${qt_root}")
  set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
  set(REFUSION_MACHINE_CACHE_ROOT "${cache_root}" CACHE PATH
      "Verified ReFusion machine dependency cache" FORCE)
  set(REFUSION_QT_MACHINE_CACHE_ROOT "${qt_root}" CACHE INTERNAL
      "Verified development Qt SDK from the machine cache" FORCE)
endfunction()
