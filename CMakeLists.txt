cmake_minimum_required(VERSION 3.2)

project(OpenBlack)

set(OS_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(GLEW_USE_STATIC_LIBS true)

if (WIN32)
    option(USE_DEBUG_CONSOLE "whether a debug console should be enabled for debug builds, if false debug output is redirected to Visual Studio output" ON)
endif()

include(FindOpenGL)
if(NOT OPENGL_FOUND AND NOT OPENGL_XMESA_FOUND AND NOT OPENGL_GLU_FOUND)
	message(FATAL_ERROR "OpenGL not found, please install it")
endif()

include(cmake/FindSDL2.cmake)
if(NOT SDL2_FOUND)
	message(FATAL_ERROR "SDL 2.0 not found, set ENV{SDL2DIR} to point to SDL 2.0, and optionally set the cmake var SDL2_LIBRARY_TEMP to the lib dir")
endif()

include(cmake/FindGLEW2.cmake)
if(NOT GLEW_FOUND)
	message(FATAL_ERROR "GLEW not found, ${GLEW_ROOT_DIR}")
endif()

include(cmake/FindGLM.cmake)
if(NOT GLM_FOUND)
	message(FATAL_ERROR "GLM not found")
endif()

#find_package(FFmpeg REQUIRED)
#find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE SWRESAMPLE)

if(OPENGL_INCLUDE_DIR)
	include_directories("${OPENGL_INCLUDE_DIR}")
endif()
include_directories("${SDL2_INCLUDE_DIR}")
include_directories("${GLEW_INCLUDE_DIRS}")
include_directories("${GLM_INCLUDE_DIRS}")

add_subdirectory(src)
#add_subdirectory(files)