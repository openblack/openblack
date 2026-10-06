file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/src/*.h"
)
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

add_library(lhvmcompiler STATIC ${SOURCES} ${HEADERS})

target_include_directories(
  lhvmcompiler
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(
  lhvmcompiler
  PUBLIC lhvmlang ScriptLibrary
  PRIVATE fmt::fmt
)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
    set_target_properties(lhvmcompiler PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(lhvmcompiler PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(lhvmcompiler PRIVATE /W4 /WX /EHs-c-)
else ()
  target_compile_options(
    lhvmcompiler PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SOURCES} ${HEADERS})

set_property(TARGET lhvmcompiler PROPERTY FOLDER "components")
