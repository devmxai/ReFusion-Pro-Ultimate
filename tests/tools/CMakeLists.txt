find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(
  NAME refusion.repo_policy
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/rfdev_policy_test.py"
)

add_test(
  NAME refusion.visual_capture_comparison
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/visual_capture_comparison_test.py"
)

add_test(
  NAME refusion.machine_dependency_cache
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/machine_cache_test.py"
)
