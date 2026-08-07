find_package(Threads REQUIRED)

add_executable(refusion-core-tests project_authority_test.cpp)
target_link_libraries(refusion-core-tests PRIVATE ReFusion::Core Threads::Threads)
refusion_enable_warnings(refusion-core-tests)

add_test(NAME refusion.project_authority COMMAND refusion-core-tests)

add_executable(refusion-project-document-tests project_document_test.cpp)
target_link_libraries(refusion-project-document-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-project-document-tests)

add_test(NAME refusion.project_document COMMAND refusion-project-document-tests)

add_executable(refusion-application-host-tests application_host_test.cpp)
target_link_libraries(refusion-application-host-tests PRIVATE ReFusion::Application)
refusion_enable_warnings(refusion-application-host-tests)

add_test(NAME refusion.application_host COMMAND refusion-application-host-tests)

add_executable(refusion-viewport-presentation-tests viewport_presentation_test.cpp)
target_link_libraries(refusion-viewport-presentation-tests
  PRIVATE ReFusion::RuntimePresentation)
refusion_enable_warnings(refusion-viewport-presentation-tests)

add_test(NAME refusion.viewport_presentation
  COMMAND refusion-viewport-presentation-tests)
