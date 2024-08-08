file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

add_library(cam STATIC ${SOURCES} ${HEADERS})

target_include_directories(
  cam PUBLIC $<INSTALL_INTERFACE:include>
             $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
    set_target_properties(cam PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_options(cam PRIVATE /W4 /WX)
else ()
  target_compile_options(cam PRIVATE -Wall -Wextra -pedantic -Werror)
endif ()

set_property(TARGET cam PROPERTY FOLDER "components")
