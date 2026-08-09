if(NOT DEFINED CLI OR NOT DEFINED PROJECT OR NOT DEFINED SOURCE OR
   NOT DEFINED TEST_ROOT)
  message(FATAL_ERROR "CLI, PROJECT, SOURCE and TEST_ROOT are required")
endif()

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_ROOT}")
set(project_copy "${TEST_ROOT}/Project.rfx")
file(COPY_FILE "${PROJECT}" "${project_copy}" ONLY_IF_DIFFERENT)

execute_process(
  COMMAND "${CLI}" capabilities
  RESULT_VARIABLE capabilities_result
  OUTPUT_VARIABLE capabilities_output
  ERROR_VARIABLE capabilities_error)
if(NOT capabilities_result EQUAL 0 OR
   NOT capabilities_output MATCHES "\"import_video\":true" OR
   NOT capabilities_output MATCHES "\"relink_exact\":true")
  message(FATAL_ERROR
    "CLI did not advertise admitted media commands: ${capabilities_output}${capabilities_error}")
endif()

execute_process(
  COMMAND "${CLI}" commit import-video "${project_copy}" "${SOURCE}" 0
  RESULT_VARIABLE import_result
  OUTPUT_VARIABLE import_output
  ERROR_VARIABLE import_error)
if(NOT import_result EQUAL 0)
  message(FATAL_ERROR
    "CLI ImportVideo failed (${import_result}): ${import_output}${import_error}")
endif()
if(NOT import_output MATCHES "\"operation\":\"import-video\"" OR
   NOT import_output MATCHES "\"status\":\"accepted\"")
  message(FATAL_ERROR "CLI ImportVideo receipt is incomplete: ${import_output}")
endif()

file(SHA256 "${SOURCE}" source_sha256)
string(SUBSTRING "${source_sha256}" 0 24 source_token)
set(asset_id "ast_${source_token}")
set(asset_path "${TEST_ROOT}/Assets/Media/${asset_id}/original.mp4")
if(NOT EXISTS "${asset_path}")
  message(FATAL_ERROR "CLI ImportVideo did not materialize ${asset_path}")
endif()

file(READ "${project_copy}" accepted_project)
if(accepted_project MATCHES "${SOURCE}" OR
   accepted_project MATCHES "/Users/")
  message(FATAL_ERROR "CLI ImportVideo leaked a host path into Project.rfx")
endif()
file(REMOVE "${asset_path}")
if(EXISTS "${asset_path}")
  message(FATAL_ERROR "could not create the missing-Asset relink condition")
endif()

execute_process(
  COMMAND "${CLI}" commit relink-exact "${project_copy}" "${asset_id}" "${SOURCE}"
  RESULT_VARIABLE relink_result
  OUTPUT_VARIABLE relink_output
  ERROR_VARIABLE relink_error)
if(NOT relink_result EQUAL 0)
  message(FATAL_ERROR
    "CLI exact relink failed (${relink_result}): ${relink_output}${relink_error}")
endif()
if(NOT EXISTS "${asset_path}" OR
   NOT relink_output MATCHES "\"status\":\"restored\"")
  message(FATAL_ERROR "CLI exact relink receipt or Asset is missing")
endif()

file(READ "${project_copy}" relinked_project)
if(NOT relinked_project STREQUAL accepted_project)
  message(FATAL_ERROR "exact relink changed canonical project truth")
endif()

execute_process(
  COMMAND "${CLI}" validate "${project_copy}" --json
  RESULT_VARIABLE validate_result
  OUTPUT_VARIABLE validate_output
  ERROR_VARIABLE validate_error)
if(NOT validate_result EQUAL 0 OR
   NOT validate_output MATCHES "\"ok\":true")
  message(FATAL_ERROR
    "reopened CLI media project is invalid: ${validate_output}${validate_error}")
endif()
