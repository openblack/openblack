# This module tries to find the bgfx library and sets the following variables:
#   BGFX_INCLUDE_DIR
#   BX_INCLUDE_DIR
#   BGFX_LIBRARY
#   BGFX_FOUND

# Search for the bgfx include directory
find_path(BGFX_INCLUDE_DIR bgfx.h
  PATH_SUFFIXES bgfx include/bgfx include
  PATHS ${BGFX_PATH}
  DOC "Where the bgfx headers can be found"
)

find_path(BX_INCLUDE_DIR bx.h
  PATH_SUFFIXES bx include/bx include
  PATHS ${BGFX_PATH}
  DOC "Where the bx headers can be found"
)

find_library(BGFX_LIBRARY
  NAMES bgfx
  PATH_SUFFIXES lib
  PATHS ${BGFX_PATH}
  DOC "Where the BGFX Library can be found"
)

# add_library(bgfx::bgfx INTERFACE IMPORTED)
