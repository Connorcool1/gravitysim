cmake_minimum_required(VERSION 3.28.3)
project(gravitysim VERSION 0.1.0)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(gravitysim 
    src/main.cpp 
    src/glad.c
)

target_link_libraries(gravitysim glfw OpenGL::GL)
target_include_directories(gravitysim PRIVATE include)

# Copy shader files to build directory
configure_file(${CMAKE_SOURCE_DIR}/src/shader.vs ${CMAKE_BINARY_DIR}/shader.vs COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/shader.fs ${CMAKE_BINARY_DIR}/shader.fs COPYONLY)