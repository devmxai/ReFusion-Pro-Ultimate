if(NOT REFUSION_FFMPEG_ROOT)
  message(FATAL_ERROR
    "REFUSION_FFMPEG_ROOT must name the verified demux-only installation")
endif()

find_path(refusion_ffmpeg_include_dir libavformat/avformat.h
  PATHS "${REFUSION_FFMPEG_ROOT}/include"
  NO_DEFAULT_PATH REQUIRED)
find_library(refusion_ffmpeg_avformat NAMES avformat
  PATHS "${REFUSION_FFMPEG_ROOT}/lib"
  NO_DEFAULT_PATH REQUIRED)
find_library(refusion_ffmpeg_avcodec NAMES avcodec
  PATHS "${REFUSION_FFMPEG_ROOT}/lib"
  NO_DEFAULT_PATH REQUIRED)
find_library(refusion_ffmpeg_avutil NAMES avutil
  PATHS "${REFUSION_FFMPEG_ROOT}/lib"
  NO_DEFAULT_PATH REQUIRED)

add_library(refusion_ffmpeg_demux STATIC FfmpegMediaDemuxer.cpp)
add_library(ReFusion::FfmpegDemux ALIAS refusion_ffmpeg_demux)

target_include_directories(refusion_ffmpeg_demux
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    "${refusion_ffmpeg_include_dir}")
target_link_libraries(refusion_ffmpeg_demux
  PUBLIC ReFusion::Application
  PRIVATE
    "${refusion_ffmpeg_avformat}"
    "${refusion_ffmpeg_avcodec}"
    "${refusion_ffmpeg_avutil}")
target_compile_features(refusion_ffmpeg_demux PUBLIC cxx_std_20)
set_property(TARGET refusion_ffmpeg_demux PROPERTY BUILD_RPATH
  "${REFUSION_FFMPEG_ROOT}/lib")
refusion_enable_warnings(refusion_ffmpeg_demux)
