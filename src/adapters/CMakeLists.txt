if(REFUSION_ENABLE_SKIA)
  add_subdirectory(skia)
endif()

if(REFUSION_ENABLE_FFMPEG_DEMUX)
  add_subdirectory(media/ffmpeg)
endif()
