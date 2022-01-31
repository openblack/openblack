# For a list of common variables see https://github.com/microsoft/vcpkg/blob/master/docs/maintainers/vcpkg_common_definitions.md

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH
    REPO "bkaradzic/bgfx.cmake"
    HEAD_REF master
    REF a7ac0aa3ae685505a1760e64b98e6c1ccc0b76a5
    SHA512 f7946c32e0e1caad71199f48a5bcec7e7a9c70f3c3f60fdcd508b2f3d165317a64232dfd591a8bd9e611e1afd817240172eb8bcb78756f093cceaf931d35009c
    PATCHES
        # https://github.com/bkaradzic/bgfx.cmake/pull/129
        0001-bx-Link-with-android-log-library.patch
        0002-examples-Don-t-build-on-android-by-default.patch
)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH_BX
    REPO "bkaradzic/bx"
    HEAD_REF master
    REF ad018d47c6c107e2fe2f3ba0716f9e728ed59a39
    SHA512 fe8b644d9b57159808d56e34b2251c09d934efa57b25378f8c136c0bcb5446acde88fb696c7328da9cf20fffc73cef8a3faa02fa8b3870ec3060ac7fbd47b5ca
)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH_BIMG
    REPO "bkaradzic/bimg"
    HEAD_REF master
    REF 663f724186e26caf46494e389ed82409106205fb
    SHA512 6fcd6d06d0d55a8f573a882bb57b729ee22101314a201bd8f440bdcadbfeb0dca6834302ef7dcfdee785d87b5456d24d2cb630d1d39836b4cd7652a6dc92d5a8
)

vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH_BGFX
    REPO "bkaradzic/bgfx"
    HEAD_REF master
    REF ae7aeba72c0663d006e99d51015ff6fcb7ba4d1d
    SHA512 1116d03cbd983f16b7595aa9b22d005f2d2e772d87c247f7915357905af4d794c828387fbc42b8fb1d76f506086b631abd1bd7172509e6fab47c5da7b32ac564
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
