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

if (CMAKE_CROSSCOMPILING
    OR (OPENBLACK_USE_VCPKG AND NOT VCPKG_TARGET_TRIPLET STREQUAL
                                VCPKG_HOST_TRIPLET)
)
  include(ExternalProject)

  set(host_build_dir "${CMAKE_BINARY_DIR}/host-shader-compiler")
  set(host_compiler "${host_build_dir}/bin/slang-sdl3-gpu-compiler")
  if (CMAKE_HOST_WIN32)
    string(APPEND host_compiler ".exe")
  endif ()

  set(host_cmake_args "")
  if (OPENBLACK_USE_VCPKG)
    list(
      APPEND
      host_cmake_args
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
      "-DVCPKG_TARGET_TRIPLET:STRING=${VCPKG_HOST_TRIPLET}"
      "-DVCPKG_HOST_TRIPLET:STRING=${VCPKG_HOST_TRIPLET}"
      "-DVCPKG_INSTALLED_DIR:PATH=${VCPKG_INSTALLED_DIR}"
      "-DVCPKG_MANIFEST_MODE:BOOL=OFF"
    )
  endif ()

  ExternalProject_Add(
    slang-sdl3-gpu-compiler-host
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/apps/slang-sdl3-gpu-compiler"
    BINARY_DIR "${host_build_dir}"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE:STRING=Release" ${host_cmake_args}
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --config Release
    BUILD_ALWAYS ON
    BUILD_BYPRODUCTS "${host_compiler}"
    INSTALL_COMMAND ""
  )

  add_executable(slang-sdl3-gpu-compiler IMPORTED GLOBAL)
  set_target_properties(
    slang-sdl3-gpu-compiler PROPERTIES IMPORTED_LOCATION "${host_compiler}"
  )
  add_dependencies(slang-sdl3-gpu-compiler slang-sdl3-gpu-compiler-host)
else ()
  add_subdirectory(apps/slang-sdl3-gpu-compiler)
endif ()
