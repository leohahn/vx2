cmake_minimum_required(VERSION 2.6)

project(deferred_renderer)

add_subdirectory(src)
add_subdirectory(lt)
add_subdirectory(thirdparty)

add_subdirectory(thirdparty/freetype-2.9)
include_directories(thirdparty/freetype-2.9/include)

include_directories(src thirdparty lt/src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_definitions(-DLT_DEBUG)

add_executable(dr
  src/main.cpp
  src/application.cpp
  src/camera.cpp
  src/renderer.cpp
  src/resources.cpp
  src/resource_manager.cpp
  src/io_task_manager.cpp
  src/io_task.cpp
  src/shader.cpp
  src/world.cpp
  src/skybox.cpp
  src/input.cpp
  lt/src/lt_math.cpp
  lt/src/lt_fs.cpp
  thirdparty/glad/glad.cpp
  thirdparty/stb_image.cpp)

target_link_libraries(dr -ldl -lm ${CMAKE_BINARY_DIR}/thirdparty/freetype-2.9/libfreetyped.a)

# GLFW 3
find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIR})
target_link_libraries(dr glfw)

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
target_link_libraries(dr ${OPENGL_gl_LIBRARY})
target_link_libraries(dr -lGL)

# Assimp
find_package(assimp REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIR})
target_link_libraries(dr ${ASSIMP_LIBRARY_DIR})
target_link_libraries(dr assimp)

if (UNIX)
  target_link_libraries(dr -pthread)
endif (UNIX)
