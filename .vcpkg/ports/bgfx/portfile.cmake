# For a list of common variables see https://github.com/microsoft/vcpkg/blob/master/docs/maintainers/vcpkg_common_definitions.md

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH
    REPO "bkaradzic/bgfx.cmake"
    HEAD_REF master
    REF a01e8e043fe213a55b0bea9623f45fad80d067bc
    SHA512 9967046cea4ad9c00126cd112089ac8aa099ea2db08dc1691f6bcf2aa95e8e5c6e52c6b98a9542b5c4232d712041aca211b6fc8f98c97e69b308de8fa8a3237b
    PATCHES
        0001-Use-string-compare-for-library-type.patch
)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH_BX
    REPO "bkaradzic/bx"
    HEAD_REF master
    REF 51c326484623c719bf0d0de8ff7ca0511a01feb4
    SHA512 acba2375c5f17deff1a53121e1fee27f014fdac7748d7d933210120b5b6e21fc07eb5d2b463faaf2396358a3c70cc169ef8b68dbcb39708a94411fb22935b5fd
)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH_BIMG
    REPO "bkaradzic/bimg"
    HEAD_REF master
    REF 9e4d2b167ffb4ecf4dba625dee59f6be7cf2587a
    SHA512 dfb8b8e87282d520347f587eab6cb7074c71f4379d42b0e96aec8b30c57f5b320dd6f2ef2ba3b0a8807e3b022f6a9aaebf629f2629458868bcb09a9c1be24e65
)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH_BGFX
    REPO "bkaradzic/bgfx"
    HEAD_REF master
    REF 932d2f21e7b6c327ef536f9ce868222049a29078
    SHA512 a309e142fad01548d99d381d1372c44d1a5ce66f5b5324d143ee839713e20fd30e529a103800c5b0f25fe9355d671e473bc5def333f26db90bcfb3c11a82a23e
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        tools       BGFX_BUILD_TOOLS
)

vcpkg_cmake_configure(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        -DBX_DIR=${SOURCE_PATH_BX}
        -DBIMG_DIR=${SOURCE_PATH_BIMG}
        -DBGFX_DIR=${SOURCE_PATH_BGFX}
        -DBGFX_LIBRARY_TYPE="STATIC"
        -DBGFX_BUILD_EXAMPLES="OFF"
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/${PORT}")
vcpkg_copy_pdbs()

if (BGFX_BUILD_TOOLS)
    vcpkg_copy_tools(
        TOOL_NAMES shaderc geometryc geometryv texturec texturev
        AUTO_CLEAN)
endif()

# Handle copyright
file(INSTALL "${CURRENT_PACKAGES_DIR}/share/licences/${PORT}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/licences"
                    "${CURRENT_PACKAGES_DIR}/debug/include"
                    "${CURRENT_PACKAGES_DIR}/debug/share"
                    "${CURRENT_PACKAGES_DIR}/debug/bin")
