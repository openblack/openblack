file(GLOB_RECURSE SCRIPTLIBRARY_SRCS RELATIVE
	"${CMAKE_CURRENT_SOURCE_DIR}"
    "src/**.cpp"
    "include/**.hpp"
)

foreach(FILE ${SCRIPTLIBRARY_SRCS})
  get_filename_component(PARENT_DIR "${FILE}" PATH)

  string(REPLACE "/" "\\" GROUP "${PARENT_DIR}")

  if ("${FILE}" MATCHES ".*\\.cpp")
    set(GROUP "Source Files\\${GROUP}")
  elseif("${FILE}" MATCHES ".*\\.hpp")
    set(GROUP "Header Files\\${GROUP}")
  endif()

  source_group("${GROUP}" FILES "${FILE}")
endforeach()

set(SCRIPTLIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(ScriptLibrary ${SCRIPTLIBRARY_SRCS})
set_target_properties(ScriptLibrary PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(ScriptLibrary PROPERTIES OUTPUT_NAME scriptlibrary)
set_target_properties(ScriptLibrary PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(ScriptLibrary PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(ScriptLibrary PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)