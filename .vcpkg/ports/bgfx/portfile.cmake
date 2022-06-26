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
  REF dd2017c3326512fb8575f7c80afbda5819d50359
  SHA512
    622523a997009b20cb3ce45133058a2cb777e96daba7b68348217ff0a0684fdcb0a81de2cd551102eb0b0643c0bd14a033f6f492318ee5f7007b30bb1ad69400
)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH_BX
  REPO "bkaradzic/bx"
  HEAD_REF master
  REF ad018d47c6c107e2fe2f3ba0716f9e728ed59a39
  SHA512
    fe8b644d9b57159808d56e34b2251c09d934efa57b25378f8c136c0bcb5446acde88fb696c7328da9cf20fffc73cef8a3faa02fa8b3870ec3060ac7fbd47b5ca
)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH_BIMG
  REPO "bkaradzic/bimg"
  HEAD_REF master
  REF 663f724186e26caf46494e389ed82409106205fb
  SHA512
    6fcd6d06d0d55a8f573a882bb57b729ee22101314a201bd8f440bdcadbfeb0dca6834302ef7dcfdee785d87b5456d24d2cb630d1d39836b4cd7652a6dc92d5a8
)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH_BGFX
  REPO "bkaradzic/bgfx"
  HEAD_REF master
  REF 251e35ae09a78a80d35bb386dc8d6f5889d79ede
  SHA512
    d30639500cdef436289334a3f800a7d6a0997fb500ba2835277ec27c8c90bb446104a98a82f84f3137c079ce3cfa773e247998a0862fe008234ec1ea37e67529
)

vcpkg_check_features(
  OUT_FEATURE_OPTIONS FEATURE_OPTIONS FEATURES tools BGFX_BUILD_TOOLS
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
