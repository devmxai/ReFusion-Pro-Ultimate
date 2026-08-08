add_library(refusion_skia_vulkan_contract_canary STATIC
  ../../SkiaGpuContextsVulkanCanary.cpp
)

add_library(ReFusion::SkiaVulkanContractCanary ALIAS
  refusion_skia_vulkan_contract_canary)

target_include_directories(refusion_skia_vulkan_contract_canary
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

find_library(REFUSION_VULKAN_LIBRARY vulkan REQUIRED)

target_link_libraries(refusion_skia_vulkan_contract_canary
  PUBLIC ReFusion::SkiaCommon
  PRIVATE Skia::Skia ${REFUSION_VULKAN_LIBRARY}
)

target_compile_features(refusion_skia_vulkan_contract_canary PUBLIC cxx_std_20)
target_compile_definitions(refusion_skia_vulkan_contract_canary PRIVATE
  REFUSION_ANDROID_CONTRACT_CANARY=1)
refusion_enable_warnings(refusion_skia_vulkan_contract_canary)
