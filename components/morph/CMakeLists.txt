file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

add_library(morph STATIC ${SOURCES} ${HEADERS})

target_include_directories(morph
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
	set_target_properties(openblack_lib PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message(WARNING "Clang-tidy checks requested but unavailable")
  endif()
endif()

if(MSVC)
	target_compile_options(morph PRIVATE /W4 /WX)
else()
	target_compile_options(morph PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET morph PROPERTY FOLDER "components")
