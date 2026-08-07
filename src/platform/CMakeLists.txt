if(APPLE AND NOT IOS)
  add_subdirectory(apple/metal)
elseif(WIN32)
  add_subdirectory(windows/d3d12)
else()
  message(FATAL_ERROR
    "REFUSION_BUILD_PLATFORM_GPU is enabled, but this platform has no admitted GPU implementation")
endif()
