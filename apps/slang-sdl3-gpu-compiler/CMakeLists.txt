cmake_minimum_required(VERSION 3.18)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(slang-sdl3-gpu-compiler LANGUAGES CXX)
  find_path(CXXOPTS_INCLUDE_DIRS "cxxopts.hpp" REQUIRED)
  # Keep the executable path consistent for single- and multi-config generators.
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/bin>")
endif ()

find_package(slang CONFIG REQUIRED)

set(SLANG_TO_SDL3_GPU_COMPILER slang-sdl3-gpu-compiler.cpp)

source_group(apps\\slang-sdl3-gpu-compiler FILES ${SLANG_TO_SDL3_GPU_COMPILER})

add_executable(slang-sdl3-gpu-compiler ${SLANG_TO_SDL3_GPU_COMPILER})
set_target_properties(
  slang-sdl3-gpu-compiler PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON
)

target_compile_definitions(
  slang-sdl3-gpu-compiler PRIVATE CXXOPTS_NO_EXCEPTIONS
)
target_link_libraries(slang-sdl3-gpu-compiler PRIVATE slang::slang)
target_include_directories(
  slang-sdl3-gpu-compiler PRIVATE ${CXXOPTS_INCLUDE_DIRS}
)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
    set_target_properties(
      slang-sdl3-gpu-compiler PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY}
    )
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(slang-sdl3-gpu-compiler PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(slang-sdl3-gpu-compiler PRIVATE /W4 /WX /EHs-c-)
else ()
  target_compile_options(
    slang-sdl3-gpu-compiler PRIVATE -Wall -Wextra -pedantic -Werror
                                    -fno-exceptions
  )
endif ()

set_property(TARGET slang-sdl3-gpu-compiler PROPERTY FOLDER "tools")

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # Cross builds import this tree with -Dslang-sdl3-gpu-compiler_DIR=<binary dir>
  export(TARGETS slang-sdl3-gpu-compiler
         FILE "${CMAKE_BINARY_DIR}/slang-sdl3-gpu-compiler-config.cmake"
  )
endif ()
