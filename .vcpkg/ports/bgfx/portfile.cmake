# Common Ambient Variables:
#   CURRENT_BUILDTREES_DIR    = ${VCPKG_ROOT_DIR}\buildtrees\${PORT}
#   CURRENT_PACKAGES_DIR      = ${VCPKG_ROOT_DIR}\packages\${PORT}_${TARGET_TRIPLET}
#   CURRENT_PORT_DIR          = ${VCPKG_ROOT_DIR}\ports\${PORT}
#   CURRENT_INSTALLED_DIR     = ${VCPKG_ROOT_DIR}\installed\${TRIPLET}
#   DOWNLOADS                 = ${VCPKG_ROOT_DIR}\downloads
#   PORT                      = current port name (zlib, etc)
#   TARGET_TRIPLET            = current triplet (x86-windows, x64-windows-static, etc)
#   VCPKG_CRT_LINKAGE         = C runtime linkage type (static, dynamic)
#   VCPKG_LIBRARY_LINKAGE     = target library linkage type (static, dynamic)
#   VCPKG_ROOT_DIR            = <C:\path\to\current\vcpkg>
#   VCPKG_TARGET_ARCHITECTURE = target architecture (x64, x86, arm)
#   VCPKG_TOOLCHAIN           = ON OFF
#   TRIPLET_SYSTEM_ARCH       = arm x86 x64
#   BUILD_ARCH                = "Win32" "x64" "ARM"
#   MSBUILD_PLATFORM          = "Win32"/"x64"/${TRIPLET_SYSTEM_ARCH}
#   DEBUG_CONFIG              = "Debug Static" "Debug Dll"
#   RELEASE_CONFIG            = "Release Static"" "Release DLL"
#   VCPKG_TARGET_IS_WINDOWS
#   VCPKG_TARGET_IS_UWP
#   VCPKG_TARGET_IS_LINUX
#   VCPKG_TARGET_IS_OSX
#   VCPKG_TARGET_IS_FREEBSD
#   VCPKG_TARGET_IS_ANDROID
#   VCPKG_TARGET_IS_MINGW
#   VCPKG_TARGET_EXECUTABLE_SUFFIX
#   VCPKG_TARGET_STATIC_LIBRARY_SUFFIX
#   VCPKG_TARGET_SHARED_LIBRARY_SUFFIX
#
# 	See additional helpful variables in /docs/maintainers/vcpkg_common_definitions.md

# Issues with glsl-optimizer force static only builds https://github.com/bkaradzic/bgfx.cmake/pull/130
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO "bkaradzic/bgfx.cmake"
  HEAD_REF master
  REF f5dc53db855a73de82d48d7ebdc029940094580d
  SHA512
    1568cc465fbd881d73b119a30e4466bae18ae3b547508182a9d2e6216582a4fe20cf7237f7b695b97e43b815367d6fd685bc3fd0b50dde42b65ee0a8fc9eef44
)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH_BX
  REPO "bkaradzic/bx"
  HEAD_REF master
  REF d4403099f916425d4bae3252c77396b51d8fe734
  SHA512
    cade0e6017f65b7d080fd0f23d77bc9c4b4b57ff5587c898df3b11ecfe827644aabd222dbb2b5602765a26e2eb4684001b59f621c0a4d4ee9c4158d384a123a7
)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH_BIMG
  REPO "bkaradzic/bimg"
  HEAD_REF 16-bit-bgr
  REF 90ac47dfb0ac21e602ed46a91fcc34016e565b98
  SHA512
    593a0689981bc7343fa45d4a5de8fd139d5f49d9d83edeea162992e8c887671d045e7c79cef3f1029d8d236810adef25e3a17098acd8d73c376f94e201f92eb2
)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH_BGFX
  REPO "bkaradzic/bgfx"
  HEAD_REF 16-bit-bgr
  REF 72cbe8327509738612afa336195ef40d71a34048
  SHA512
    ad890b8eba8141157284b1ec77815cb117ec3ccc8ad0a5270b0d2ad81194dcc9348c7e0e6e788f1bc7e9dfc74c5b1a59bd7c52979c492f50df34ac7d9a5e5f30
)

vcpkg_check_features(
  OUT_FEATURE_OPTIONS FEATURE_OPTIONS
  FEATURES tools BGFX_BUILD_TOOLS multithreaded BGFX_CONFIG_MULTITHREADED
)

if (VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
  set(BGFX_LIBRARY_TYPE "SHARED")
else ()
  set(BGFX_LIBRARY_TYPE "STATIC")
endif ()

vcpkg_cmake_configure(
  SOURCE_PATH "${SOURCE_PATH}"
  OPTIONS -DBX_DIR=${SOURCE_PATH_BX}
          -DBIMG_DIR=${SOURCE_PATH_BIMG}
          -DBGFX_DIR=${SOURCE_PATH_BGFX}
          -DBGFX_LIBRARY_TYPE=${BGFX_LIBRARY_TYPE}
          -DBGFX_BUILD_EXAMPLES="OFF"
          -DBGFX_OPENGLES_VERSION=30
          ${FEATURE_OPTIONS}
  # OPTIONS_RELEASE -DOPTIMIZE=1
  # OPTIONS_DEBUG -DDEBUGGABLE=1
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/${PORT}")
vcpkg_copy_pdbs()

if (BGFX_BUILD_TOOLS)
  vcpkg_copy_tools(
    TOOL_NAMES shaderc geometryc geometryv texturec texturev AUTO_CLEAN
  )
endif ()

# Handle copyright
file(
  INSTALL "${CURRENT_PACKAGES_DIR}/share/licences/${PORT}/LICENSE"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/licences"
     "${CURRENT_PACKAGES_DIR}/debug/include"
     "${CURRENT_PACKAGES_DIR}/debug/share" "${CURRENT_PACKAGES_DIR}/debug/bin"
)
