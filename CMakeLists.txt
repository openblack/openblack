cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(${CMAKE_VERSION} VERSION_LESS 3.14)
	include(${CMAKE_SOURCE_DIR}/cmake/backport/FetchContent.cmake)
else()
	include(FetchContent)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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
    option(OPENBLACK_USE_DEBUG_CONSOLE "whether a debug console should be enabled for debug builds, if false debug output is redirected to Visual Studio output" ON)
endif()

# Default build type to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release" FORCE)
endif()

# check compiler for various features
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(filesystem HAS_FILESYSTEM)
if(HAS_FILESYSTEM)
	message("compiler has CXX header <filesystem>")
else()
	message("compiler has no CXX header <filesystem>, need to use <experimental/filesystem>")
endif()

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

# Dependencies

find_package(OpenGL REQUIRED)
find_package(spdlog REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLM REQUIRED)
find_package(EnTT REQUIRED)
find_package(bgfx REQUIRED COMPONENTS bgfx)

# unify includes as targets
if(NOT TARGET SDL2::SDL2)
	add_library(SDL2::SDL2 IMPORTED INTERFACE)
	target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIR})
	set_property(TARGET SDL2::SDL2 PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES})
endif()
if(NOT TARGET SDL2::SDL2main)
	add_library(SDL2::SDL2main IMPORTED INTERFACE)
	target_link_libraries(SDL2::SDL2main INTERFACE SDL2::Main)
endif()
if(NOT TARGET glm::glm)
	add_library(glm::glm IMPORTED INTERFACE)
	if(CMAKE_VERSION VERSION_LESS 3.11)
		# for 3.10 support we need to do this
		set_property(TARGET glm::glm PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIRS}")
		set_property(TARGET glm::glm PROPERTY INTERFACE_LINK_LIBRARIES ${GLM_LIBRARY})
	else()
		# starting with cmake 3.11 we can do this
		target_include_directories(glm::glm INTERFACE ${GLM_INCLUDE_DIRS})
		target_link_libraries(glm::glm INTERFACE ${GLM_LIBRARY})
	endif()
endif()

# Include git hash in source
include(cmake/GetGitRevisionDescription.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
message(STATUS "Building ${CMAKE_PROJECT_NAME} GIT SHA1: ${GIT_SHA1}")

if(OPENGL_INCLUDE_DIR)
	include_directories("${OPENGL_INCLUDE_DIR}")
endif()

include(cmake/Shaders.cmake)
add_subdirectory(components/ScriptLibrary)
add_subdirectory(external/imgui)
add_subdirectory(src)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT openblack)
