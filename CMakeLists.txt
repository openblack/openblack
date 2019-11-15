cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
if(${CMAKE_VERSION} VERSION_LESS 3.14)
  include(${CMAKE_SOURCE_DIR}/CMakeModules/backport/FetchContent.cmake)
else()
  include(FetchContent)
endif()

# automatically use vcpkg if VCPKG_ROOT is defined
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(STATUS "Using VCPKG_ROOT $ENV{VCPKG_ROOT}")
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

project(openblack)

# Configure C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# don't allow in source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Output binaries to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# check compiler for various features
include(CheckIncludeFileCXX)
check_include_file_cxx(filesystem HAS_FILESYSTEM)

# Dependencies: we search for config first, then module this helps package managers.
if(CMAKE_VERSION VERSION_GREATER 3.15 OR CMAKE_VERSION VERSION_EQUAL 3.15)
  set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

  # hacky because sdl2 cmake-config is terrible
  find_package(SDL2 MODULE QUIET)
  if(NOT SDL2_FOUND)
    find_package(SDL2 CONFIG REQUIRED)
  endif()

  find_package(spdlog 1.3.0 REQUIRED)
  find_package(glm REQUIRED)
  find_package(EnTT 3.1.0 CONFIG REQUIRED) # only available as a config
  find_package(cxxopts REQUIRED)
else()
  find_package(SDL2 MODULE QUIET)
  find_package(spdlog 1.3.0 CONFIG QUIET)
  find_package(glm CONFIG QUIET)
  find_package(EnTT 3.1.0 CONFIG REQUIRED) # only available as a config
  find_package(cxxopts CONFIG REQUIRED)

  if(NOT SDL2_FOUND)
    find_package(SDL2 CONFIG REQUIRED)
  endif()

  if(NOT DEFINED spdlog_DIR)
    find_package(spdlog 1.3.0 MODULE REQUIRED)
  endif()

  if(NOT DEFINED glm_DIR)
    find_package(glm MODULE REQUIRED)
  endif()

  if(NOT DEFINED cxxopts_DIR)
    find_package(cxxopts MODULE REQUIRED)
  endif()
endif()

# todo: support find_package(bgfx)
if(NOT bgfx_FOUND)
  FetchContent_Declare(
    bgfx
    GIT_REPOSITORY https://github.com/openblack/bgfx.cmake.git
    GIT_TAG materials
  )

  set(BGFX_BUILD_EXAMPLES
      OFF
      CACHE BOOL "Build bgfx examples.")
  set(BGFX_INSTALL
      OFF
      CACHE BOOL "Create installation target.")
  set(BGFX_CUSTOM_TARGETS
      OFF
      CACHE BOOL "Include convenience custom targets.")
  set(BGFX_USE_DEBUG_SUFFIX
      OFF
      CACHE BOOL "Add 'd' suffix to debug output targets")

  include(CMakeDependentOption)
  option(OPENBLACK_BGFX_USE_SYSTEM_DEPS
         "Search for system libraries for all 3rd parties of bgfx" OFF)
  cmake_dependent_option(
    OPENBLACK_BGFX_USE_SYSTEM_SPIRV_TOOLS
    "Search for system spirv tools instead of building it" ON
    "OPENBLACK_BGFX_USE_SYSTEM_DEPS" OFF)
  cmake_dependent_option(
    OPENBLACK_BGFX_USE_SYSTEM_SPIRV_CROSS
    "Search for system spirv cross instead of building it" ON
    "OPENBLACK_BGFX_USE_SYSTEM_DEPS" OFF)

  if(OPENBLACK_BGFX_USE_SYSTEM_SPIRV_TOOLS)
    find_library(SPIRVTOOLSOPT SPIRV-Tools-opt)
    find_package(spirv-tools QUIET)
    if(NOT spirv-tools_FOUND)
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(SPIRV_TOOLS REQUIRED IMPORTED_TARGET GLOBAL SPIRV-Tools)
      add_library(spirv-tools ALIAS PkgConfig::SPIRV_TOOLS)
    endif()
  endif()
  if(OPENBLACK_BGFX_USE_SYSTEM_SPIRV_CROSS)
    find_package(spirv_cross_core REQUIRED)
    find_package(spirv_cross_glsl REQUIRED)
    find_package(spirv_cross_msl REQUIRED)
    find_package(spirv_cross_reflect REQUIRED)
    add_library(spirv-cross INTERFACE)
    target_link_libraries(
      spirv-cross INTERFACE spirv-cross-core spirv-cross-glsl spirv-cross-msl
                            spirv-cross-reflect)
    if(WIN32)
      find_package(spirv_cross_hlsl REQUIRED)
      target_link_libraries(spirv-cross INTERFACE spirv-cross-hlsl)
    endif()
  endif()

  # declare this here so bgfx compiles in a reasonable time
  if(MSVC)
    add_compile_options(/MP)
  endif()

  FetchContent_MakeAvailable(bgfx)
  add_library(bgfx::bgfx ALIAS bgfx)
endif()

FetchContent_Declare(
  imgui-flame-graph
  GIT_REPOSITORY https://github.com/bwrsandman/imgui-flame-graph.git
  GIT_TAG master)
set(IMGUI_FLAME_GRAPH_BUILD_EXAMPLE
    OFF
    CACHE BOOL "imgui-flame bgfx examples.")
FetchContent_MakeAvailable(imgui-flame-graph)

# Setup a custom clang-format target (if clang-format can be found) that will run
# against all the src files. This should be used before making a pull request.
# =======================================================================

find_program(CLANG_FORMAT clang-format)
if (CLANG_FORMAT)
  set(SRCS ${PROJECT_SOURCE_DIR}/src)
  set(COMPS ${PROJECT_SOURCE_DIR}/components)
  set(APPS ${PROJECT_SOURCE_DIR}/apps)
  set(CCOMMENT "Running clang format against all the .h and .cpp files in src/")
  if (WIN32)
      add_custom_target(clang-format
          COMMAND powershell.exe -Command "Get-ChildItem '${SRCS}/*' -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
          COMMAND powershell.exe -Command "Get-ChildItem '${COMPS}/*' -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
          COMMAND powershell.exe -Command "Get-ChildItem '${APPS}/*' -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
          COMMENT ${CCOMMENT})
  elseif(MINGW)
      add_custom_target(clang-format
          COMMAND find `cygpath -u ${SRCS}` -iname *.h -o -iname *.cpp | xargs `cygpath -u ${CLANG_FORMAT}` -i
          COMMAND find `cygpath -u ${COMPS}` -iname *.h -o -iname *.cpp | xargs `cygpath -u ${CLANG_FORMAT}` -i
          COMMAND find `cygpath -u ${APPS}` -iname *.h -o -iname *.cpp | xargs `cygpath -u ${CLANG_FORMAT}` -i
          COMMENT ${CCOMMENT})
  else()
      add_custom_target(clang-format
          COMMAND find ${SRCS} -iname *.h -o -iname *.cpp | xargs ${CLANG_FORMAT} -i
          COMMAND find ${COMPS} -iname *.h -o -iname *.cpp | xargs ${CLANG_FORMAT} -i
          COMMAND find ${APPS} -iname *.h -o -iname *.cpp | xargs ${CLANG_FORMAT} -i
          COMMENT ${CCOMMENT})
  endif()
  unset(SRCS)
  unset(CCOMMENT)
endif()

# Include git hash in source
include(CMakeModules/GetGitRevisionDescription.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
message(STATUS "Building ${CMAKE_PROJECT_NAME} GIT SHA1: ${GIT_SHA1}")

add_subdirectory(external/imgui)

include(CMakeModules/Shaders.cmake)

add_subdirectory(components/l3d)
add_subdirectory(components/g3d)
add_subdirectory(components/lnd)
add_subdirectory(components/ScriptLibrary)
add_subdirectory(src)
add_subdirectory(apps/l3dtool)
add_subdirectory(apps/g3dtool)
add_subdirectory(apps/lndtool)

# Set openblack project as default startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT
                                                            openblack)
# set_property(GLOBAL PROPERTY USE_FOLDERS ON)
