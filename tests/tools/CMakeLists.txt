find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(
  NAME refusion.repo_policy
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/rfdev_policy_test.py"
)
