project (matf_rg)
cmake_minimum_required (VERSION 2.8.11)
add_executable (matf_rg main_linux.cpp gl.cpp global.cpp)
target_include_directories (matf_rg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (matf_rg LINK_PUBLIC GL GLEW glfw)

