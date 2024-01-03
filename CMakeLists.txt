cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

# VCPKG has an imperfect android implementation.
# NDK must be in env and vcpkg_android.cmake selects architecture.
if (VCPKG_TARGET_ANDROID)
  set(ENV{ANDROID_NDK_HOME} ${ANDROID_NDK_HOME})
  include(vcpkg_android)
endif ()

# On Visual Studio generators default to using vcpkg as the most newbie friendly option
if (CMAKE_GENERATOR MATCHES "^Visual Studio")
  message(
    STATUS
      "Defaulting to using vcpkg in a Windows build environment. You can disable with OPENBLACK_USE_VCPKG"
  )
  option(OPENBLACK_USE_VCPKG
         "Resolve dependencies using the vcpkg submodule toolchain" ON
  )
else ()
  option(OPENBLACK_USE_VCPKG
         "Resolve dependencies using the vcpkg submodule toolchain" OFF
  )
endif ()

# If using vcpkg and not manually specified the toolchain then set it for them
if (OPENBLACK_USE_VCPKG AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
      CACHE STRING "Default to vcpkg toolchain file"
  )
endif ()

project(openblack)

# Configure C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# don't allow in source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# do find_package with CONFIG before MODULE
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# Output binaries to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

option(
  OPENBLACK_CLANG_TIDY_CHECKS
  "Do extra checks with clang-tidy if it's available (takes longer to build, enable for CI)"
  OFF
)
option(OPENBLACK_WARNINGS_AS_ERRORS
       "Treat warnings as errors (disable easier prototyping, enable for CI)"
       OFF
)
option(OPENBLACK_TRACE_TIME
       "Compilation Time analysis (only available with clang)" OFF
)

find_program(
  CLANG_TIDY NAMES clang-tidy-7 clang-tidy-6.0 clang-tidy-5.0 clang-tidy-4.0
                   clang-tidy
)

# Special case for SDL2 dependency, goal is to find a config that exports SDL2::SDL2 target
# libsdl2-dev has a `sdl2-config.cmake` that doesn't export this, but vcpkg does..
find_package(SDL2 CONFIG QUIET)
if (NOT TARGET SDL2::SDL2)
  find_package(SDL2 MODULE REQUIRED)
endif ()

find_package(Stb QUIET)
if (NOT Stb_FOUND)
  find_path(
    Stb_INCLUDE_DIR
    NAMES stb_image_write.h stb_image.h stb_rect_pack.h
    PATHS /usr/include/stb REQUIRED
  )
endif ()

find_package(unofficial-minizip CONFIG QUIET)
if (TARGET unofficial::minizip::minizip)
  add_library(minizip::minizip ALIAS unofficial::minizip::minizip)
else ()
  find_package(PkgConfig)
  pkg_check_modules(minizip REQUIRED minizip IMPORTED_TARGET)
  add_library(minizip::minizip ALIAS PkgConfig::minizip)
  target_link_libraries(PkgConfig::minizip INTERFACE z ${minizip_LIBRARIES})
endif ()

# patch until next glm release
find_package(glm REQUIRED)
if (TARGET glm AND NOT TARGET glm::glm)
  add_library(glm::glm ALIAS glm)
endif ()

# vcpkg loses the dependency of bx on windows which fails to pull in all headers
find_package(bgfx REQUIRED)
if (TARGET bgfx::bgfx AND TARGET bgfx::bx)
  target_link_libraries(bgfx::bgfx INTERFACE bgfx::bx)
endif ()

find_path(
  DRLIBS_INCLUDE_DIRS REQUIRED
  NAMES dr_flac.h dr_mp3.h dr_wav.h
  PATHS /usr/include/dr_libs
)

find_package(OpenAL REQUIRED)
find_package(imgui REQUIRED)
find_package(Bullet REQUIRED)
find_package(spdlog 1.3.0 REQUIRED)
find_package(EnTT 3.7.0 CONFIG REQUIRED) # only available as a config
find_package(cxxopts REQUIRED)

include(ClangFormat)

# gets bundled dependencies (imgui, bgfx.cmake)
add_subdirectory(externals)
add_subdirectory(components/l3d)
add_subdirectory(components/pack)
add_subdirectory(components/lnd)
add_subdirectory(components/anm)
add_subdirectory(components/glw)
add_subdirectory(components/morph)
add_subdirectory(components/ScriptLibrary)
add_subdirectory(src)
add_subdirectory(apps/l3dtool)
add_subdirectory(apps/packtool)
add_subdirectory(apps/lndtool)
add_subdirectory(apps/anmtool)
add_subdirectory(apps/glwtool)
add_subdirectory(apps/morphtool)

# Map CMAKE_HOST_SYSTEM_PROCESSOR value
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64"
    OR ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64"
)
  set(OPENBLACK_HOST_ARCH "x64")
elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86"
        OR ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "i386"
)
  set(OPENBLACK_HOST_ARCH "Win32")
elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "ARM64")
  set(OPENBLACK_HOST_ARCH "ARM64")
else ()
  set(OPENBLACK_HOST_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif ()

# Cross compile check
set(OPENBLACK_CROSSCOMPILING ${CMAKE_CROSSCOMPILING})
if (MSVC)
  if (NOT "${OPENBLACK_HOST_ARCH}" STREQUAL "${CMAKE_VS_PLATFORM_NAME}"
      AND VCPKG_TARGET_TRIPLET
      AND NOT VCPKG_TARGET_TRIPLET MATCHES "${OPENBLACK_HOST_ARCH}*"
  )
    set(OPENBLACK_CROSSCOMPILING ON)
  endif ()
endif ()

if (NOT OPENBLACK_CROSSCOMPILING) # Requires python and running compiled apps
  add_subdirectory(test/mock)
endif ()

find_package(GTest CONFIG)
if (GTest_FOUND AND NOT OPENBLACK_CROSSCOMPILING)
  enable_testing()
  add_subdirectory(test)
endif ()

# Set openblack project as default startup project in Visual Studio
set_property(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT openblack
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
