cmake_minimum_required(VERSION 2.6)

project(deferred_renderer)

add_subdirectory(src)
add_subdirectory(lt)
add_subdirectory(thirdparty)

include_directories(src thirdparty lt/src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

add_definitions(-DLT_DEBUG)

add_executable(dr
  src/main.cpp
  src/application.cpp
  src/camera.cpp
  src/renderer.cpp
  src/resources.cpp
  src/landscape.cpp
  src/resource_manager.cpp
  src/pool_allocator.cpp
  src/io_task_manager.cpp
  src/io_task.cpp
  src/shader.cpp
  src/world.cpp
  src/font.cpp
  src/texture.cpp
  src/gl_resources.cpp
  src/resource_file.cpp
  src/skybox.cpp
  src/input.cpp
  src/stb_rect_pack.cpp
  src/stb_truetype.cpp
  src/stb_image.cpp
  lt/src/lt_math.cpp
  lt/src/lt_fs.cpp
  thirdparty/glad/glad.cpp
  thirdparty/open-simplex-noise.cpp
  )

target_link_libraries(dr -ldl -lm)

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
