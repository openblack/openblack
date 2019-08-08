cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(openblack)

# Set C++17 Standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

# Output binaries to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# disable in-source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Project Options
if (WIN32)
    option(USE_DEBUG_CONSOLE "whether a debug console should be enabled for debug builds, if false debug output is redirected to Visual Studio output" ON)
endif()

# Default build type to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release" FORCE)
endif()

# Check that all Git submodules are present
function(check_submodules_present)
    file(READ "${PROJECT_SOURCE_DIR}/.gitmodules" gitmodules)
    string(REGEX MATCHALL "path *= *[^ \t\r\n]*" gitmodules ${gitmodules})
    foreach(module ${gitmodules})
        string(REGEX REPLACE "path *= *" "" module ${module})
        if (NOT EXISTS "${PROJECT_SOURCE_DIR}/${module}/.git")
            message(FATAL_ERROR "Git submodule ${module} not found. "
                    "Please run: git submodule update --init --recursive")
        endif()
    endforeach()
endfunction()
# check_submodules_present()

# Dependencies
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW2 REQUIRED)
find_package(GLM REQUIRED)

# Include git hash in source
include(cmake/GetGitRevisionDescription.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
message(STATUS "Building ${CMAKE_PROJECT_NAME} GIT SHA1: ${GIT_SHA1}")

set(GLEW_USE_STATIC_LIBS true)

if(OPENGL_INCLUDE_DIR)
	include_directories("${OPENGL_INCLUDE_DIR}")
endif()
include_directories("${SDL2_INCLUDE_DIR}")
include_directories("${GLEW_INCLUDE_DIR}")
include_directories("${GLM_INCLUDE_DIRS}")

add_subdirectory(components/ScriptLibrary)
add_subdirectory(src)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT openblack)