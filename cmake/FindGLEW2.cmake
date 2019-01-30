# Find GLEW
#
# Once done this will define
#  GLEW_FOUND - system has GLEW
#  GLEW_INCLUDE_DIR - the GLEW include directory
#  GLEW_LIBRARY - The library needed to use GLEW
#

include(FindPackageHandleStandardArgs)

# Set up architectures (for windows) and prefixes (for mingw builds)
if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(GLEW_LIB_PATH_SUFFIX Release/x64)
	else()
		set(GLEW_LIB_PATH_SUFFIX Release/Win32)
	endif()
endif()

find_path( GLEW_INCLUDE_DIR GL/glew.h
            /usr/include
            /usr/local/include
            /sw/include
            /opt/local/include
            $ENV{PROGRAMFILES}/GLEW/include
			${GLEW_ROOT_DIR}/include
            DOC "The directory where GL/glew.h resides")

find_library( GLEW_LIBRARY
            NAMES GLEW glew glew32s glew32
            PATHS
            /usr/lib64
            /usr/lib
            /usr/local/lib64
            /usr/local/lib
            /sw/lib
            /opt/local/lib
            $ENV{PROGRAMFILES}/GLEW/lib
			${GLEW_ROOT_DIR}/lib
			${GLEW_ROOT_DIR}/lib/${GLEW_LIB_PATH_SUFFIX}
            DOC "The GLEW library")

find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_LIBRARY GLEW_INCLUDE_DIR)
mark_as_advanced(GLEW_LIBRARY GLEW_INCLUDE_DIR)

