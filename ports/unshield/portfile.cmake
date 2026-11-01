vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO twogood/unshield
  REF "${VERSION}"
  SHA512
    fd3b604e8aa3347a1dc58396d6b9a74b167718908a7f3419dce0a57caaa5d67c3e816da74f05d6690707d09348f9898ea321a80e85a01717eedbf18e60abb3d4
  HEAD_REF main
  PATCHES 0001-Embed-md5-objects.patch
)

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" BUILD_STATIC)

vcpkg_cmake_configure(
  SOURCE_PATH "${SOURCE_PATH}"
  OPTIONS
    -DBUILD_STATIC=${BUILD_STATIC}
    # Avoid an optional OpenSSL dependency for md5
    -DUSE_OUR_OWN_MD5=ON
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=ON
    # The bundled md5 uses K&R prototypes, which C23 (GCC 15's default) reads as taking no arguments
    -DCMAKE_C_STANDARD=17
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/unshield)
# The exported target links ZLIB::ZLIB but the config never looks for it. zlib's own config package only
# provides ZLIB::ZLIBSTATIC for static builds, so stand in for ZLIB::ZLIB when it is missing.
file(READ "${CURRENT_PACKAGES_DIR}/share/unshield/unshieldConfig.cmake"
     UNSHIELD_CONFIG
)
file(
  WRITE "${CURRENT_PACKAGES_DIR}/share/unshield/unshieldConfig.cmake"
  [[
include(CMakeFindDependencyMacro)
find_dependency(ZLIB)
if(NOT TARGET ZLIB::ZLIB AND TARGET ZLIB::ZLIBSTATIC)
    add_library(ZLIB::ZLIB INTERFACE IMPORTED)
    target_link_libraries(ZLIB::ZLIB INTERFACE ZLIB::ZLIBSTATIC)
endif()
]]
  "${UNSHIELD_CONFIG}"
)
vcpkg_fixup_pkgconfig()

vcpkg_copy_tools(TOOL_NAMES unshield AUTO_CLEAN)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include"
     "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
