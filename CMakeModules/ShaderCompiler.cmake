# Shader compilation runs on the build host, including when the game is cross-compiled.
if (OPENBLACK_USE_VCPKG AND NOT VCPKG_HOST_TRIPLET)
  if (DEFINED ENV{VCPKG_DEFAULT_HOST_TRIPLET}
      AND NOT "$ENV{VCPKG_DEFAULT_HOST_TRIPLET}" STREQUAL ""
  )
    set(VCPKG_HOST_TRIPLET "$ENV{VCPKG_DEFAULT_HOST_TRIPLET}")
  else ()
    if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
      set(host_arch arm64)
    elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64|x64)$")
      set(host_arch x64)
    else ()
      set(host_arch x86)
    endif ()
    string(TOLOWER "${CMAKE_HOST_SYSTEM_NAME}" host_system)
    if (host_system STREQUAL "darwin")
      set(host_system osx)
    endif ()
    set(VCPKG_HOST_TRIPLET "${host_arch}-${host_system}")
  endif ()
endif ()

# A build tree has one toolchain. When that toolchain cannot produce a binary
# the host can run against a host slang, the compiler must come from a separate
# native build of apps/slang-sdl3-gpu-compiler instead.
set(build_shader_compiler_default ON)
if (CMAKE_CROSSCOMPILING
    OR (OPENBLACK_USE_VCPKG AND NOT VCPKG_TARGET_TRIPLET STREQUAL
                                VCPKG_HOST_TRIPLET)
)
  set(build_shader_compiler_default OFF)
endif ()
option(
  OPENBLACK_BUILD_SHADER_COMPILER
  "Build slang-sdl3-gpu-compiler in this tree instead of importing a host build"
  ${build_shader_compiler_default}
)

if (OPENBLACK_BUILD_SHADER_COMPILER)
  add_subdirectory(apps/slang-sdl3-gpu-compiler)
else ()
  # Host tools never live in the target sysroot, so skip find root re-rooting.
  find_package(slang-sdl3-gpu-compiler CONFIG NO_CMAKE_FIND_ROOT_PATH)
  if (NOT slang-sdl3-gpu-compiler_FOUND)
    message(
      FATAL_ERROR
        "slang-sdl3-gpu-compiler must be built for the host first:\n"
        "  cmake -S apps/slang-sdl3-gpu-compiler -B build-host -DCMAKE_BUILD_TYPE=Release"
        " [-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake]\n"
        "  cmake --build build-host\n"
        "Then configure this project with -Dslang-sdl3-gpu-compiler_DIR=<build-host>"
    )
  endif ()
endif ()
