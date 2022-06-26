file(GLOB_RECURSE SCRIPTLIBRARY_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB_RECURSE SCRIPTLIBRARY_HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

set(SCRIPTLIBRARY_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PARENT_SCOPE
)
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(ScriptLibrary ${SCRIPTLIBRARY_SOURCES} ${SCRIPTLIBRARY_HEADERS})

target_include_directories(
  ScriptLibrary
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

source_group(
  TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SCRIPTLIBRARY_SOURCES}
                                       ${SCRIPTLIBRARY_HEADERS}
)

set_property(TARGET ScriptLibrary PROPERTY FOLDER "components")
