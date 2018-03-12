cmake_minimum_required(VERSION 2.6)

project(vx2)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

add_subdirectory(src)
add_subdirectory(lt)
add_subdirectory(thirdparty)

include_directories(src thirdparty lt/src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DLT_DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

if(CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif(CMAKE_BUILD_TYPE MATCHES Release)

add_executable(dr
  src/main.cpp
  src/application.cpp
  src/camera.cpp
  src/renderer.cpp
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
  src/stb_rect_pack.cpp
  src/stb_truetype.cpp
  src/stb_image.cpp
  src/vertex_buffer.cpp
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
