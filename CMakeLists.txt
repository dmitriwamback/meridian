cmake_minimum_required(VERSION 4.1)
project(meridian)

set(CMAKE_CXX_STANDARD 20)

add_executable(meridian main.cpp)
target_link_libraries(meridian "/opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.dylib")
target_link_libraries(meridian "/opt/homebrew/Cellar/glfw/3.3.8/lib/libglfw.dylib")
target_link_libraries(meridian "/opt/homebrew/Cellar/assimp/5.4.3/lib/libassimp.dylib")
target_link_libraries(meridian "/opt/local/lib/libvulkan.1.dylib")
include_directories("/opt/homebrew/Cellar/assimp/5.4.3/include")
find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
target_link_libraries(meridian ${OPENGL_FRAMEWORK})