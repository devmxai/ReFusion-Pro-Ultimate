find_package(Threads REQUIRED)

add_executable(refusion-core-tests project_authority_test.cpp)
target_link_libraries(refusion-core-tests PRIVATE ReFusion::Core Threads::Threads)
refusion_enable_warnings(refusion-core-tests)

add_test(NAME refusion.project_authority COMMAND refusion-core-tests)

add_executable(refusion-semantic-authoring-intent-tests
  semantic_authoring_intent_test.cpp)
target_link_libraries(refusion-semantic-authoring-intent-tests
  PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-semantic-authoring-intent-tests)

add_test(NAME refusion.semantic_authoring_intents
  COMMAND refusion-semantic-authoring-intent-tests)

add_executable(refusion-project-document-tests project_document_test.cpp)
target_link_libraries(refusion-project-document-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-project-document-tests)

add_test(NAME refusion.project_document COMMAND refusion-project-document-tests)

add_executable(refusion-canonical-coordinates-tests
  canonical_coordinates_test.cpp)
target_link_libraries(refusion-canonical-coordinates-tests
  PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-canonical-coordinates-tests)

add_test(NAME refusion.canonical_coordinates
  COMMAND refusion-canonical-coordinates-tests)

add_executable(refusion-content-digest-tests content_digest_test.cpp)
target_link_libraries(refusion-content-digest-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-content-digest-tests)

add_test(NAME refusion.content_digest COMMAND refusion-content-digest-tests)

add_executable(refusion-canonical-text-tests canonical_text_test.cpp)
target_link_libraries(refusion-canonical-text-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-canonical-text-tests)

add_test(NAME refusion.canonical_text COMMAND refusion-canonical-text-tests)

add_executable(refusion-color-contract-tests color_contract_test.cpp)
target_link_libraries(refusion-color-contract-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-color-contract-tests)

add_test(NAME refusion.color_contract COMMAND refusion-color-contract-tests)

add_executable(refusion-text-layout-tests text_layout_test.cpp)
target_link_libraries(refusion-text-layout-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-text-layout-tests)

add_test(NAME refusion.text_layout COMMAND refusion-text-layout-tests)

add_executable(refusion-measured-alignment-tests measured_alignment_test.cpp)
target_link_libraries(refusion-measured-alignment-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-measured-alignment-tests)

add_test(NAME refusion.measured_alignment
  COMMAND refusion-measured-alignment-tests)

add_executable(refusion-visual-property-registry-tests
  visual_property_registry_test.cpp)
target_link_libraries(refusion-visual-property-registry-tests
  PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-visual-property-registry-tests)

add_test(NAME refusion.visual_property_registry
  COMMAND refusion-visual-property-registry-tests)

add_executable(refusion-visual-contribution-registry-tests
  visual_contribution_registry_test.cpp)
target_link_libraries(refusion-visual-contribution-registry-tests
  PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-visual-contribution-registry-tests)

add_test(NAME refusion.visual_contribution_registry
  COMMAND refusion-visual-contribution-registry-tests)

add_executable(refusion-project-creation-tests project_creation_test.cpp)
target_link_libraries(refusion-project-creation-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-project-creation-tests)

add_test(NAME refusion.project_creation COMMAND refusion-project-creation-tests)

add_executable(refusion-project-rfx-tests project_rfx_test.cpp)
target_link_libraries(refusion-project-rfx-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-project-rfx-tests)

add_test(NAME refusion.project_rfx COMMAND refusion-project-rfx-tests)

add_executable(refusion-media-project-schema-tests
  media_project_schema_test.cpp)
target_link_libraries(refusion-media-project-schema-tests
  PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-media-project-schema-tests)

add_test(NAME refusion.media_project_schema
  COMMAND refusion-media-project-schema-tests)

add_executable(refusion-media-index-tests media_index_test.cpp)
target_link_libraries(refusion-media-index-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-media-index-tests)

add_test(NAME refusion.media_index COMMAND refusion-media-index-tests)

add_executable(refusion-media-indexing-service-tests
  media_indexing_service_test.cpp)
target_link_libraries(refusion-media-indexing-service-tests
  PRIVATE ReFusion::Application)
refusion_enable_warnings(refusion-media-indexing-service-tests)

add_test(NAME refusion.media_indexing_service
  COMMAND refusion-media-indexing-service-tests)

add_executable(refusion-xplat-project-conformance-tests
  xplat_project_conformance_test.cpp)
target_link_libraries(refusion-xplat-project-conformance-tests
  PRIVATE ReFusion::Core)
target_compile_definitions(refusion-xplat-project-conformance-tests PRIVATE
  REFUSION_XPLAT_PROJECT_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/Project.rfx"
  REFUSION_XPLAT_PROJECT_RECEIPT_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/expected-project.txt"
  REFUSION_XPLAT_CANONICAL_PROJECT_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/expected-canonical.rfx")
refusion_enable_warnings(refusion-xplat-project-conformance-tests)

add_test(NAME refusion.xplat_project_conformance
  COMMAND refusion-xplat-project-conformance-tests)

add_executable(refusion-xplat-command-conformance-tests
  xplat_command_conformance_test.cpp)
target_link_libraries(refusion-xplat-command-conformance-tests
  PRIVATE ReFusion::Core)
target_compile_definitions(refusion-xplat-command-conformance-tests PRIVATE
  REFUSION_XPLAT_COMMAND_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/Project.rfx"
  REFUSION_XPLAT_COMMAND_RECEIPT_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/expected-commands.txt")
refusion_enable_warnings(refusion-xplat-command-conformance-tests)

add_test(NAME refusion.xplat_command_conformance
  COMMAND refusion-xplat-command-conformance-tests)

add_executable(refusion-agent-introspection-tests
  agent_introspection_test.cpp)
target_link_libraries(refusion-agent-introspection-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-agent-introspection-tests)

add_test(NAME refusion.agent_introspection
  COMMAND refusion-agent-introspection-tests)

add_executable(refusion-agent-authoring-guide-tests
  agent_authoring_guide_test.cpp)
target_link_libraries(refusion-agent-authoring-guide-tests
  PRIVATE ReFusion::Application)
refusion_enable_warnings(refusion-agent-authoring-guide-tests)

add_test(NAME refusion.agent_authoring_guide
  COMMAND refusion-agent-authoring-guide-tests)

add_executable(refusion-exp006g-regression-tests
  exp006g_regression_test.cpp)
target_link_libraries(refusion-exp006g-regression-tests PRIVATE ReFusion::Core)
target_compile_definitions(refusion-exp006g-regression-tests PRIVATE
  REFUSION_EXP006G_PROJECT_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/project/exp006g-reels/Project.rfx")
refusion_enable_warnings(refusion-exp006g-regression-tests)

add_test(NAME refusion.exp006g_regression
  COMMAND refusion-exp006g-regression-tests)

add_executable(refusion-project-clock-tests project_clock_test.cpp)
target_link_libraries(refusion-project-clock-tests PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-project-clock-tests)

add_test(NAME refusion.project_clock COMMAND refusion-project-clock-tests)

add_executable(refusion-application-host-tests application_host_test.cpp)
target_link_libraries(refusion-application-host-tests PRIVATE ReFusion::Application)
refusion_enable_warnings(refusion-application-host-tests)

add_test(NAME refusion.application_host COMMAND refusion-application-host-tests)

add_executable(refusion-visual-render-plan-tests visual_render_plan_test.cpp)
target_link_libraries(refusion-visual-render-plan-tests
  PRIVATE ReFusion::RuntimeRender)
refusion_enable_warnings(refusion-visual-render-plan-tests)

add_test(NAME refusion.visual_render_plan
  COMMAND refusion-visual-render-plan-tests)

add_executable(refusion-visual-output-contract-tests
  visual_output_contract_test.cpp)
target_link_libraries(refusion-visual-output-contract-tests
  PRIVATE ReFusion::RuntimeRender)
refusion_enable_warnings(refusion-visual-output-contract-tests)

add_test(NAME refusion.visual_output_contract
  COMMAND refusion-visual-output-contract-tests)

add_executable(refusion-xplat-render-plan-conformance-tests
  xplat_render_plan_conformance_test.cpp)
target_link_libraries(refusion-xplat-render-plan-conformance-tests
  PRIVATE ReFusion::RuntimeRender)
target_compile_definitions(refusion-xplat-render-plan-conformance-tests PRIVATE
  REFUSION_XPLAT_VISUAL_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/Project.rfx"
  REFUSION_XPLAT_VISUAL_RECEIPT_PATH="${CMAKE_SOURCE_DIR}/tests/fixtures/render-plan/xplat-visual-v1/expected-render-plan.txt")
refusion_enable_warnings(refusion-xplat-render-plan-conformance-tests)

add_test(NAME refusion.xplat_render_plan_conformance
  COMMAND refusion-xplat-render-plan-conformance-tests)

add_executable(refusion-viewport-presentation-tests viewport_presentation_test.cpp)
target_link_libraries(refusion-viewport-presentation-tests
  PRIVATE ReFusion::RuntimePresentation)
refusion_enable_warnings(refusion-viewport-presentation-tests)

add_test(NAME refusion.viewport_presentation
  COMMAND refusion-viewport-presentation-tests)

add_executable(refusion-media-capability-tests media_capability_test.cpp)
target_link_libraries(refusion-media-capability-tests
  PRIVATE ReFusion::RuntimeMedia)
refusion_enable_warnings(refusion-media-capability-tests)

add_test(NAME refusion.media_capability
  COMMAND refusion-media-capability-tests)

add_executable(refusion-hardware-video-decode-scheduler-tests
  hardware_video_decode_scheduler_test.cpp)
target_link_libraries(refusion-hardware-video-decode-scheduler-tests
  PRIVATE ReFusion::RuntimeMedia)
refusion_enable_warnings(refusion-hardware-video-decode-scheduler-tests)

add_test(NAME refusion.hardware_video_decode_scheduler
  COMMAND refusion-hardware-video-decode-scheduler-tests)

add_executable(refusion-media-index-decode-projection-tests
  media_index_decode_projection_test.cpp)
target_link_libraries(refusion-media-index-decode-projection-tests
  PRIVATE ReFusion::RuntimeMedia)
refusion_enable_warnings(refusion-media-index-decode-projection-tests)

add_test(NAME refusion.media_index_decode_projection
  COMMAND refusion-media-index-decode-projection-tests)

add_executable(refusion-gpu-observability-tests
  gpu_observability_test.cpp)
target_link_libraries(refusion-gpu-observability-tests
  PRIVATE ReFusion::RuntimeGpu)
refusion_enable_warnings(refusion-gpu-observability-tests)

add_test(NAME refusion.gpu_observability
  COMMAND refusion-gpu-observability-tests)
