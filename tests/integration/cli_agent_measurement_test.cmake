execute_process(
  COMMAND "${CLI}" measure "${PROJECT}" 500000000 --json
  RESULT_VARIABLE measure_status
  OUTPUT_VARIABLE measurement
  ERROR_VARIABLE measure_error)
if(NOT measure_status EQUAL 0)
  message(FATAL_ERROR "Skia Agent measurement failed: ${measure_error}")
endif()
string(JSON schema GET "${measurement}" schema)
string(JSON layout_digest GET "${measurement}" layout_engine_digest)
if(NOT schema STREQUAL "refusion.agent.measure.v1" OR
   layout_digest STREQUAL "")
  message(FATAL_ERROR "Agent measurement has no admitted layout digest")
endif()
string(JSON node_count LENGTH "${measurement}" nodes)
math(EXPR last_node "${node_count} - 1")
set(found_text OFF)
foreach(index RANGE 0 ${last_node})
  string(JSON node_ref GET "${measurement}" nodes ${index} ref)
  if(node_ref STREQUAL "layer:lyr_title")
    string(JSON font_digest GET "${measurement}" nodes ${index}
      resolved_font_digest)
    string(JSON logical_type TYPE "${measurement}" nodes ${index}
      logical_world_px)
    string(JSON ink_type TYPE "${measurement}" nodes ${index} ink_world_px)
    if(font_digest STREQUAL "" OR logical_type STREQUAL "NULL" OR
       ink_type STREQUAL "NULL")
      message(FATAL_ERROR "Text measurement omitted Font/logical/ink data")
    endif()
    set(found_text ON)
  endif()
endforeach()
if(NOT found_text)
  message(FATAL_ERROR "Agent measurement omitted lyr_title")
endif()

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_ROOT}")
file(COPY "${PROJECT}" DESTINATION "${TEST_ROOT}")
set(candidate "${TEST_ROOT}/Project.rfx")
execute_process(
  COMMAND "${CLI}" commit align "${candidate}"
    layer:lyr_title layer:lyr_background 500000000 left top logical
  RESULT_VARIABLE align_status
  OUTPUT_VARIABLE alignment
  ERROR_VARIABLE align_error)
if(NOT align_status EQUAL 0)
  message(FATAL_ERROR "typed logical alignment failed: ${align_error}\n${alignment}")
endif()
string(JSON align_ok GET "${alignment}" ok)
string(JSON revision GET "${alignment}" revision)
if(NOT align_ok OR NOT revision EQUAL 2)
  message(FATAL_ERROR "typed logical alignment did not publish revision 2")
endif()
