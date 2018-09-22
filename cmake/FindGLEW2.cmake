#
# Try to find GLEW library and include path.
# Once done this will define
#
# GLEW_FOUND
# GLEW_INCLUDE_DIR
# GLEW_LIBRARY
# 

IF (WIN32)
	FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
		$ENV{PROGRAMFILES}/GLEW/include
		${GLEW_ROOT_DIR}/include
		DOC "The directory where GL/glew.h resides")

	if (GLEW_USE_STATIC_LIBS)
		set(GLEW_LIBRARY_NAME glew32s)
		set(GLEW_DEFINITIONS -DGLEW_STATIC)
	else()
		set(GLEW_LIBRARY_NAME glew32)
	endif()

	FIND_LIBRARY( GLEW_LIBRARY
		NAMES ${GLEW_LIBRARY_NAME}
		PATHS
		$ENV{PROGRAMFILES}/GLEW/lib
		${GLEW_ROOT_DIR}/lib/Release/Win32
		DOC "The GLEW library")

		unset(GLEW_LIBRARY_NAME)
ELSE (WIN32)
	FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		DOC "The directory where GL/glew.h resides")
	FIND_LIBRARY( GLEW_LIBRARY
		NAMES GLEW glew
		PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		DOC "The GLEW library")
ENDIF (WIN32)

# Handle REQUIRD argument, define *_FOUND variable
find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_INCLUDE_DIR GLEW_LIBRARY)

# Define GLEW_LIBRARIES and GLEW_INCLUDE_DIRS
if (GLEW_FOUND)
	set(GLEW_LIBRARIES ${OPENGL_LIBRARIES} ${GLEW_LIBRARY})
	set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
endif()

# Hide some variables
mark_as_advanced(GLEW_INCLUDE_DIR GLEW_LIBRARY)
