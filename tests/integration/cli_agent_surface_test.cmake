if(NOT DEFINED CLI OR NOT DEFINED PROJECT OR NOT DEFINED TEST_ROOT)
  message(FATAL_ERROR "CLI, PROJECT and TEST_ROOT are required")
endif()

function(run_json expected_schema)
  execute_process(
    COMMAND "${CLI}" ${ARGN}
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE diagnostic)
  if(NOT status EQUAL 0)
    message(FATAL_ERROR "command failed (${status}): ${diagnostic}\n${output}")
  endif()
  string(JSON schema ERROR_VARIABLE json_error GET "${output}" schema)
  if(json_error OR NOT schema STREQUAL expected_schema)
    message(FATAL_ERROR
      "unexpected JSON schema '${schema}' (${json_error}): ${output}")
  endif()
  set(LAST_JSON "${output}" PARENT_SCOPE)
endfunction()

run_json("refusion.agent.outline.v1" outline "${PROJECT}")
run_json("refusion.agent.inspect.v1" inspect "${PROJECT}" layer:lyr_title)
run_json("refusion.agent.capabilities.v1" capabilities)
run_json("refusion.agent.validate.v1" validate "${PROJECT}" --json)
run_json("refusion.agent.lint.v1" lint "${PROJECT}")
run_json("refusion.agent.diff.v1" diff "${PROJECT}" "${PROJECT}")
run_json("refusion.agent.measure.v1" measure "${PROJECT}" 500000000 --json)

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_ROOT}")
file(COPY "${PROJECT}" DESTINATION "${TEST_ROOT}")
set(candidate "${TEST_ROOT}/Project.rfx")

run_json("refusion.agent.commit.v1" commit add-glow "${candidate}"
  lyr_title fx_cli_agent_test 12 "#7C5CFFFF")
string(JSON commit_ok GET "${LAST_JSON}" ok)
string(JSON revision GET "${LAST_JSON}" revision)
if(NOT commit_ok OR NOT revision EQUAL 2)
  message(FATAL_ERROR "typed AddEffect did not publish revision 2")
endif()

file(READ "${candidate}" before_rejected_commit)
execute_process(
  COMMAND "${CLI}" commit add-glow "${candidate}"
    lyr_title fx_cli_agent_test 12 "#7C5CFFFF"
  RESULT_VARIABLE rejected_status
  OUTPUT_VARIABLE rejected_output)
if(NOT rejected_status EQUAL 1)
  message(FATAL_ERROR "duplicate effect commit did not fail closed")
endif()
string(JSON rejected_ok GET "${rejected_output}" ok)
string(JSON rejected_code GET "${rejected_output}" diagnostic code)
if(rejected_ok OR NOT rejected_code STREQUAL "RFX-PROJECT-131")
  message(FATAL_ERROR "duplicate effect returned the wrong diagnostic")
endif()
file(READ "${candidate}" after_rejected_commit)
if(NOT before_rejected_commit STREQUAL after_rejected_commit)
  message(FATAL_ERROR "rejected commit changed Last-Known-Good bytes")
endif()

run_json("refusion.agent.commit.v1" commit align "${candidate}"
  layer:lyr_panel layer:lyr_background 0 left top geometry)
run_json("refusion.agent.commit.v1" commit group "${candidate}"
  grp_scene "Scene Root" layer:lyr_background group:grp_hero)
run_json("refusion.agent.validate.v1" validate "${candidate}" --json)
string(JSON final_revision GET "${LAST_JSON}" revision)
if(NOT final_revision EQUAL 4)
  message(FATAL_ERROR "typed commit sequence did not produce revision 4")
endif()

run_json("refusion.agent.outline.v1" outline "${candidate}")
string(JSON root_count LENGTH "${LAST_JSON}" roots)
string(JSON root GET "${LAST_JSON}" roots 0)
if(NOT root_count EQUAL 1 OR NOT root STREQUAL "group:grp_scene")
  message(FATAL_ERROR "group commit did not produce one semantic root")
endif()
