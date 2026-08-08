add_library(refusion_platform_gpu STATIC
  AndroidVulkanContractCanary.cpp
)

add_library(ReFusion::PlatformGpu ALIAS refusion_platform_gpu)

target_include_directories(refusion_platform_gpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
)

find_library(REFUSION_ANDROID_LIBRARY android REQUIRED)
find_library(REFUSION_VULKAN_LIBRARY vulkan REQUIRED)

target_link_libraries(refusion_platform_gpu
  PUBLIC
    ReFusion::RuntimeGpu
    ReFusion::RuntimePresentation
  PRIVATE
    ${REFUSION_ANDROID_LIBRARY}
    ${REFUSION_VULKAN_LIBRARY}
)

target_compile_features(refusion_platform_gpu PUBLIC cxx_std_20)
target_compile_definitions(refusion_platform_gpu PRIVATE
  REFUSION_ANDROID_CONTRACT_CANARY=1)
refusion_enable_warnings(refusion_platform_gpu)
