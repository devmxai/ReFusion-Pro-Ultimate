find_package(Threads REQUIRED)

add_executable(refusion-core-tests project_authority_test.cpp)
target_link_libraries(refusion-core-tests PRIVATE ReFusion::Core Threads::Threads)
refusion_enable_warnings(refusion-core-tests)

add_test(NAME refusion.project_authority COMMAND refusion-core-tests)
