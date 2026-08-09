include_guard(GLOBAL)

function(refusion_deploy_skia_runtime target)
  if(NOT WIN32)
    return()
  endif()
  if(NOT TARGET "${target}" OR NOT TARGET Skia::Skia)
    message(FATAL_ERROR
      "Skia runtime deployment requires existing ${target} and Skia::Skia targets")
  endif()
  get_target_property(skia_icu_data Skia::Skia REFUSION_ICU_DATA_FILE)
  if(NOT skia_icu_data)
    message(FATAL_ERROR "Skia::Skia does not expose its verified ICU data file")
  endif()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(d3dcompiler_arch x64)
  else()
    set(d3dcompiler_arch x86)
  endif()
  find_file(refusion_d3dcompiler_runtime
    NAMES d3dcompiler_47.dll
    PATHS
      "$ENV{WindowsSdkDir}/Redist/D3D/${d3dcompiler_arch}"
      "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/Redist/D3D/${d3dcompiler_arch}"
    NO_DEFAULT_PATH)
  if(NOT refusion_d3dcompiler_runtime)
    message(FATAL_ERROR
      "Windows SDK redistributable d3dcompiler_47.dll is required for Skia D3D12")
  endif()
  add_custom_command(TARGET "${target}" POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${skia_icu_data}" "$<TARGET_FILE_DIR:${target}>/icudtl.dat"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${refusion_d3dcompiler_runtime}"
            "$<TARGET_FILE_DIR:${target}>/d3dcompiler_47.dll"
    VERBATIM)
endfunction()

function(refusion_import_skia)
  if(TARGET Skia::Skia)
    return()
  endif()

  set(REFUSION_SKIA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/out/deps-src/skia"
      CACHE PATH "Verified official Skia source directory")
  set(REFUSION_SKIA_BUILD_DIR "${CMAKE_SOURCE_DIR}/out/deps-build/skia/macos-arm64-metal"
      CACHE PATH "Verified prebuilt Skia profile directory")
  set(REFUSION_SKIA_PROFILE "" CACHE STRING
      "Required verified Skia profile identity")
  if(REFUSION_SKIA_PROFILE STREQUAL "")
    if(IOS)
      set(REFUSION_SKIA_PROFILE "ios-arm64-metal-canary")
    elseif(ANDROID)
      set(REFUSION_SKIA_PROFILE "android-arm64-vulkan-canary")
    elseif(APPLE)
      set(REFUSION_SKIA_PROFILE "macos-arm64-metal")
    elseif(WIN32)
      set(REFUSION_SKIA_PROFILE "windows-x64-d3d12")
    else()
      message(FATAL_ERROR "No admitted Skia profile exists for this platform")
    endif()
  endif()

  file(REAL_PATH "${CMAKE_SOURCE_DIR}" refusion_repo_root)
  file(REAL_PATH "${REFUSION_SKIA_SOURCE_DIR}" refusion_skia_source_real)
  file(REAL_PATH "${REFUSION_SKIA_BUILD_DIR}" refusion_skia_build_real)
  cmake_path(IS_PREFIX refusion_repo_root "${refusion_skia_source_real}"
             NORMALIZE refusion_skia_source_is_local)
  cmake_path(IS_PREFIX refusion_repo_root "${refusion_skia_build_real}"
             NORMALIZE refusion_skia_build_is_local)
  if(NOT refusion_skia_source_is_local OR NOT refusion_skia_build_is_local)
    message(FATAL_ERROR
      "Skia sources and build artifacts must remain inside this ReFusion checkout")
  endif()

  file(READ "${CMAKE_SOURCE_DIR}/deps/manifest.lock.json" refusion_dependency_lock)
  string(JSON expected_skia_revision GET "${refusion_dependency_lock}"
         components skia revision)
  string(JSON expected_skia_origin GET "${refusion_dependency_lock}"
         components skia official_origin)

  find_package(Git REQUIRED)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${REFUSION_SKIA_SOURCE_DIR}" rev-parse HEAD
    OUTPUT_VARIABLE actual_skia_revision
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE revision_result
  )
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${REFUSION_SKIA_SOURCE_DIR}" remote get-url origin
    OUTPUT_VARIABLE actual_skia_origin
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE origin_result
  )
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${REFUSION_SKIA_SOURCE_DIR}" status --porcelain
    OUTPUT_VARIABLE skia_worktree_status
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE status_result
  )
  if(NOT revision_result EQUAL 0 OR NOT origin_result EQUAL 0 OR
     NOT status_result EQUAL 0)
    message(FATAL_ERROR "Skia source is absent or is not a Git checkout; run tools/bootstrap.py")
  endif()
  if(NOT actual_skia_revision STREQUAL expected_skia_revision)
    message(FATAL_ERROR
      "Skia revision mismatch: expected ${expected_skia_revision}, got ${actual_skia_revision}")
  endif()
  if(NOT actual_skia_origin STREQUAL expected_skia_origin)
    message(FATAL_ERROR
      "Skia origin mismatch: expected ${expected_skia_origin}, got ${actual_skia_origin}")
  endif()
  if(NOT skia_worktree_status STREQUAL "")
    message(FATAL_ERROR "Skia source worktree is modified; rematerialize it with --fresh")
  endif()

  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/bootstrap.py"
            verify-skia-materialization
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE materialization_result
    OUTPUT_VARIABLE materialization_output
    ERROR_VARIABLE materialization_error
  )
  if(NOT materialization_result EQUAL 0)
    message(FATAL_ERROR
      "Skia transitive materialization verification failed:\n${materialization_output}${materialization_error}")
  endif()

  set(build_record "${REFUSION_SKIA_BUILD_DIR}/refusion-build.json")
  if(NOT EXISTS "${build_record}")
    message(FATAL_ERROR
      "Verified Skia build record is missing; run tools/bootstrap.py build-skia --profile <profile>")
  endif()
  file(READ "${build_record}" skia_build_record)
  string(JSON build_record_schema GET "${skia_build_record}" schema_version)
  if(NOT build_record_schema EQUAL 2)
    message(FATAL_ERROR "Skia build record schema is stale; rebuild the profile")
  endif()
  string(JSON built_skia_origin GET "${skia_build_record}" source_origin)
  if(NOT built_skia_origin STREQUAL expected_skia_origin)
    message(FATAL_ERROR "Skia artifact build record has an unexpected source origin")
  endif()
  string(JSON built_skia_revision GET "${skia_build_record}" source_revision)
  if(NOT built_skia_revision STREQUAL expected_skia_revision)
    message(FATAL_ERROR "Skia artifact was built from an unexpected source revision")
  endif()
  string(JSON built_profile GET "${skia_build_record}" profile)
  if(NOT built_profile STREQUAL REFUSION_SKIA_PROFILE)
    message(FATAL_ERROR
      "Skia profile mismatch: expected ${REFUSION_SKIA_PROFILE}, got ${built_profile}")
  endif()
  file(READ "${CMAKE_SOURCE_DIR}/deps/profiles/skia/profiles.json"
       skia_profile_catalog)
  if(WIN32)
    string(JSON expected_source_patch GET "${skia_profile_catalog}"
           profiles "${built_profile}" source_patch)
    string(JSON built_source_patch GET "${skia_build_record}" source_patch)
    string(JSON expected_source_patch_sha256 GET "${skia_build_record}"
           source_patch_sha256)
    if(NOT built_source_patch STREQUAL expected_source_patch)
      message(FATAL_ERROR "Skia build used an unexpected Windows source patch")
    endif()
    file(REAL_PATH "${CMAKE_SOURCE_DIR}/${built_source_patch}"
         built_source_patch_real)
    file(REAL_PATH "${CMAKE_SOURCE_DIR}/deps/patches/skia"
         skia_source_patch_root)
    cmake_path(IS_PREFIX skia_source_patch_root "${built_source_patch_real}"
               NORMALIZE built_source_patch_is_local)
    if(NOT built_source_patch_is_local)
      message(FATAL_ERROR "Skia source patch is outside the admitted patch root")
    endif()
    file(SHA256 "${built_source_patch_real}" actual_source_patch_sha256)
    if(NOT actual_source_patch_sha256 STREQUAL expected_source_patch_sha256)
      message(FATAL_ERROR "Skia Windows source patch changed after the build")
    endif()
  endif()
  string(JSON expected_target_count LENGTH "${skia_profile_catalog}"
         profiles "${built_profile}" targets)
  string(JSON built_target_count LENGTH "${skia_build_record}" targets)
  if(NOT built_target_count EQUAL expected_target_count)
    message(FATAL_ERROR "Skia build target closure changed after the recorded build")
  endif()
  if(expected_target_count GREATER 0)
    math(EXPR last_target_index "${expected_target_count} - 1")
    foreach(target_index RANGE 0 ${last_target_index})
      string(JSON expected_target GET "${skia_profile_catalog}"
             profiles "${built_profile}" targets ${target_index})
      string(JSON built_target GET "${skia_build_record}" targets ${target_index})
      if(NOT built_target STREQUAL expected_target)
        message(FATAL_ERROR
          "Skia build target mismatch: expected ${expected_target}, got ${built_target}")
      endif()
    endforeach()
  endif()
  string(JSON built_args_path GET "${skia_build_record}" gn_args)
  string(JSON expected_args_sha256 GET "${skia_build_record}" gn_args_sha256)
  file(REAL_PATH "${CMAKE_SOURCE_DIR}/${built_args_path}" built_args_real)
  cmake_path(IS_PREFIX refusion_repo_root "${built_args_real}"
             NORMALIZE built_args_is_local)
  if(NOT built_args_is_local)
    message(FATAL_ERROR "Skia build record references a GN profile outside ReFusion")
  endif()
  file(SHA256 "${built_args_real}" actual_args_sha256)
  if(NOT actual_args_sha256 STREQUAL expected_args_sha256)
    message(FATAL_ERROR "Skia GN profile changed after the recorded build")
  endif()
  string(JSON dependency_record_path GET "${skia_build_record}" dependency_record)
  string(JSON expected_dependency_sha256 GET "${skia_build_record}"
         dependency_record_sha256)
  if(NOT dependency_record_path STREQUAL "out/deps-src/skia-dependencies.lock.json")
    message(FATAL_ERROR "Skia build record references an unexpected dependency inventory")
  endif()
  file(SHA256 "${CMAKE_SOURCE_DIR}/${dependency_record_path}"
       actual_dependency_sha256)
  if(NOT actual_dependency_sha256 STREQUAL expected_dependency_sha256)
    message(FATAL_ERROR "Skia dependency inventory changed after the recorded build")
  endif()
  string(JSON tracked_dependency_path GET "${skia_build_record}"
         tracked_dependency_lock)
  string(JSON expected_tracked_dependency_sha256 GET "${skia_build_record}"
         tracked_dependency_lock_sha256)
  file(REAL_PATH "${CMAKE_SOURCE_DIR}/${tracked_dependency_path}"
       tracked_dependency_real)
  file(REAL_PATH "${CMAKE_SOURCE_DIR}/deps/locks" tracked_dependency_root)
  cmake_path(IS_PREFIX tracked_dependency_root "${tracked_dependency_real}"
             NORMALIZE tracked_dependency_is_local)
  if(NOT tracked_dependency_is_local)
    message(FATAL_ERROR "Skia build record references an untracked dependency lock path")
  endif()
  file(SHA256 "${tracked_dependency_real}" actual_tracked_dependency_sha256)
  if(NOT actual_tracked_dependency_sha256 STREQUAL
         expected_tracked_dependency_sha256)
    message(FATAL_ERROR "Tracked Skia dependency lock changed after the build")
  endif()

  if(WIN32)
    set(skia_library "${REFUSION_SKIA_BUILD_DIR}/refusion_skia_bundle.lib")
  else()
    set(skia_library "${REFUSION_SKIA_BUILD_DIR}/librefusion_skia_bundle.a")
  endif()
  if(NOT EXISTS "${skia_library}")
    message(FATAL_ERROR "Skia artifact is missing: ${skia_library}")
  endif()
  string(JSON expected_artifact_sha256 GET "${skia_build_record}" artifact sha256)
  file(SHA256 "${skia_library}" actual_artifact_sha256)
  if(NOT actual_artifact_sha256 STREQUAL expected_artifact_sha256)
    message(FATAL_ERROR "Skia artifact digest differs from its build record")
  endif()

  add_library(Skia::Skia STATIC IMPORTED GLOBAL)
  set_target_properties(Skia::Skia PROPERTIES
    IMPORTED_LOCATION "${skia_library}"
    INTERFACE_INCLUDE_DIRECTORIES "${REFUSION_SKIA_SOURCE_DIR}"
    INTERFACE_COMPILE_FEATURES cxx_std_20
    INTERFACE_COMPILE_DEFINITIONS
      "SK_GANESH;SK_GRAPHITE;REFUSION_SKIA_REVISION=\"${expected_skia_revision}\""
  )

  if(IOS)
    find_library(REFUSION_CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(REFUSION_CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(REFUSION_CORE_TEXT_FRAMEWORK CoreText REQUIRED)
    find_library(REFUSION_FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(REFUSION_METAL_FRAMEWORK Metal REQUIRED)
    find_library(REFUSION_QUARTZ_CORE_FRAMEWORK QuartzCore REQUIRED)
    find_library(REFUSION_UIKIT_FRAMEWORK UIKit REQUIRED)
    set_property(TARGET Skia::Skia PROPERTY INTERFACE_LINK_LIBRARIES
      "${REFUSION_CORE_FOUNDATION_FRAMEWORK};${REFUSION_CORE_GRAPHICS_FRAMEWORK};${REFUSION_CORE_TEXT_FRAMEWORK};${REFUSION_FOUNDATION_FRAMEWORK};${REFUSION_METAL_FRAMEWORK};${REFUSION_QUARTZ_CORE_FRAMEWORK};${REFUSION_UIKIT_FRAMEWORK}")
    set_property(TARGET Skia::Skia APPEND PROPERTY
      INTERFACE_COMPILE_DEFINITIONS SK_METAL)
  elseif(APPLE)
    find_library(REFUSION_APPKIT_FRAMEWORK AppKit REQUIRED)
    find_library(REFUSION_APPLICATION_SERVICES_FRAMEWORK ApplicationServices REQUIRED)
    find_library(REFUSION_CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(REFUSION_CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(REFUSION_CORE_TEXT_FRAMEWORK CoreText REQUIRED)
    find_library(REFUSION_FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(REFUSION_METAL_FRAMEWORK Metal REQUIRED)
    set_property(TARGET Skia::Skia PROPERTY INTERFACE_LINK_LIBRARIES
      "${REFUSION_APPKIT_FRAMEWORK};${REFUSION_APPLICATION_SERVICES_FRAMEWORK};${REFUSION_CORE_FOUNDATION_FRAMEWORK};${REFUSION_CORE_GRAPHICS_FRAMEWORK};${REFUSION_CORE_TEXT_FRAMEWORK};${REFUSION_FOUNDATION_FRAMEWORK};${REFUSION_METAL_FRAMEWORK}")
    set_property(TARGET Skia::Skia APPEND PROPERTY
      INTERFACE_COMPILE_DEFINITIONS SK_METAL)
  elseif(WIN32)
    set(skia_icu_data "${REFUSION_SKIA_BUILD_DIR}/icudtl.dat")
    set(skia_icu_source
        "${REFUSION_SKIA_SOURCE_DIR}/third_party/externals/icu/common/icudtl.dat")
    if(NOT EXISTS "${skia_icu_data}" OR NOT EXISTS "${skia_icu_source}")
      message(FATAL_ERROR "Verified Skia ICU runtime data is missing")
    endif()
    file(SHA256 "${skia_icu_data}" skia_icu_data_sha256)
    file(SHA256 "${skia_icu_source}" skia_icu_source_sha256)
    if(NOT skia_icu_data_sha256 STREQUAL skia_icu_source_sha256)
      message(FATAL_ERROR "Skia ICU runtime data differs from the pinned source")
    endif()
    set_property(TARGET Skia::Skia PROPERTY
      REFUSION_ICU_DATA_FILE "${skia_icu_data}")
    set_property(TARGET Skia::Skia PROPERTY INTERFACE_LINK_LIBRARIES
      "d3d12;d3dcompiler;dxgi;dxguid")
    set_property(TARGET Skia::Skia APPEND PROPERTY
      INTERFACE_COMPILE_DEFINITIONS SK_DIRECT3D)
  endif()
endfunction()
