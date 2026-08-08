add_library(refusion_platform_gpu STATIC
  IosMetalContractCanary.mm
)

add_library(ReFusion::PlatformGpu ALIAS refusion_platform_gpu)

target_include_directories(refusion_platform_gpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
)

target_link_libraries(refusion_platform_gpu
  PUBLIC
    ReFusion::RuntimeGpu
    ReFusion::RuntimePresentation
  PRIVATE
    "-framework Foundation"
    "-framework Metal"
    "-framework QuartzCore"
    "-framework UIKit"
)

target_compile_features(refusion_platform_gpu PUBLIC cxx_std_20)
target_compile_options(refusion_platform_gpu PRIVATE -fobjc-arc)
target_compile_definitions(refusion_platform_gpu PRIVATE
  REFUSION_IOS_CONTRACT_CANARY=1)
refusion_enable_warnings(refusion_platform_gpu)
