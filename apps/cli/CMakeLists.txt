add_executable(refusion-cli main.cpp)
target_link_libraries(refusion-cli PRIVATE ReFusion::Core)
refusion_enable_warnings(refusion-cli)

