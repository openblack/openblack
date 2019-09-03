cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(HUNTER_ENABLED "Enable Hunter package manager support" OFF)
include(cmake/HunterGate.cmake)

HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.23.214.tar.gz"
  SHA1 "e14bc153a7f16d6a5eeec845fb0283c8fad8c358"
  LOCAL # cmake/Hunter/config.cmake
)

project(openblack)

# Set C++17 Standard
# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
string(COMPARE EQUAL "${CMAKE_CXX_STANDARD}" "" no_cmake_cxx_standard_set)
if(no_cmake_cxx_standard_set)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF) # disable gnu extensions like gnu++17
  message(STATUS "Using default C++ standard ${CMAKE_CXX_STANDARD}")
else()
  message(STATUS "Using user specified C++ standard ${CMAKE_CXX_STANDARD}")
endif()

# Output binaries to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# disable in-source builds
# set(CMAKE_DISABLE_SOURCE_CHANGES ON)
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
check_submodules_present()

# check compiler for various features
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(filesystem HAS_FILESYSTEM)
if(HAS_FILESYSTEM)
	message("compiler has CXX header <filesystem>")
else()
	message("compiler has no CXX header <filesystem>, need to use <experimental/filesystem>")
endif()

# Dependencies
hunter_add_package(SDL2)
hunter_add_package(glew)
hunter_add_package(glm)
find_package(OpenGL REQUIRED)
if(HUNTER_ENABLED)
	find_package(SDL2 CONFIG REQUIRED)
	find_package(glew CONFIG REQUIRED)
	find_package(glm REQUIRED)
else()
	find_package(SDL2 REQUIRED)
	find_package(GLEW2 REQUIRED)
	find_package(GLM REQUIRED)
endif()

# Add dependencies from the external directory
set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
add_subdirectory("${EXTERNAL_DIR}/spdlog")
add_subdirectory("${EXTERNAL_DIR}/entt")

# Include git hash in source
include(cmake/GetGitRevisionDescription.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
message(STATUS "Building ${CMAKE_PROJECT_NAME} GIT SHA1: ${GIT_SHA1}")

set(GLEW_USE_STATIC_LIBS true)

if(OPENGL_INCLUDE_DIR)
	include_directories("${OPENGL_INCLUDE_DIR}")
endif()
if(NOT HUNTER_ENABLED)
	include_directories("${SDL2_INCLUDE_DIR}")
	include_directories("${GLEW_INCLUDE_DIR}")
	include_directories("${GLM_INCLUDE_DIRS}")
endif()

add_subdirectory(components/ScriptLibrary)
include(src/CMakeLists.txt)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT openblack)

# Installation configuration
if (UNIX)
  install(TARGETS openblack RUNTIME DESTINATION bin)
  install(FILES src/imgui/LICENSE.txt
          DESTINATION "share/licenses/openblack"
          RENAME "IMGUI_LICENSE.txt")
endif()
