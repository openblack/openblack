#!/bin/bash
set -e
set -x

uname -a
cmake --version

cmake -H. -B_build_${TOOLCHAIN} -DCMAKE_INSTALL_PREFIX=${PWD}/_install_${TOOLCHAIN} -DCMAKE_TOOLCHAIN_FILE="${PWD}/ci/toolchains/${TOOLCHAIN}.cmake" -DHUNTER_ENABLED=${HUNTER_ENABLED} -DUSE_SUBMODULES=${USE_SUBMODULES}
cmake --build _build_${TOOLCHAIN} -- -j2

if [ "$RUN_TESTS" = true ]; then
	case "$TOOLCHAIN" in linux-mingw*)
		echo "copy runtime libraries needed for tests into build directory"
		cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll /usr/lib/gcc/x86_64-w64-mingw32/7.3-win32/{libstdc++-6.dll,libgcc_s_seh-1.dll} _build_${TOOLCHAIN}
	esac
	CTEST_OUTPUT_ON_FAILURE=1 cmake --build _build_${TOOLCHAIN} --target test
fi

