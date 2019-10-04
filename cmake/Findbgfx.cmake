# This module tries to find the bgfx library and sets the following variables:
#   BGFX_INCLUDE_DIR
#   BX_INCLUDE_DIR
#   BGFX_LIBRARY
#   BGFX_FOUND

# Search for the header file
find_path(BGFX_INCLUDE_DIR bgfx.h
  PATH_SUFFIXES include)

find_path(BX_INCLUDE_DIR bx/bx.h
  PATH_SUFFIXES include)

# Search for the library
find_library(BGFX_LIBRARY NAMES bgfx
  PATH_SUFFIXES lib)

# Set to false if something isn't found
set(BGFX_FOUND TRUE)

if(${BGFX_INCLUDE_DIR-NOTFOUND})
	set(BGFX_FOUND FALSE)
	message(FATAL_ERROR "bgfx.h not found.")
endif()

if(${BX_INCLUDE_DIR-NOTFOUND})
	set(BGFX_FOUND FALSE)
	message(FATAL_ERROR "bx/bx.h not found.")
endif()

if(${BGFX_LIBRARY-NOTFOUND})
	set(BGFX_FOUND FALSE)
	message(FATAL_ERROR "bgfx lib not found.")
endif()

if(${BGFX_FOUND})
	message(STATUS "bgfx found in ${BGFX_INCLUDE_DIR}")
endif()
