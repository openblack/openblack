set(PACKTOOL
  packtool.cpp
)

source_group(apps\\packtool FILES ${PACKTOOL})

add_executable(packtool ${PACKTOOL})

target_link_libraries(packtool
	PRIVATE
		cxxopts::cxxopts
		pack
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
	target_compile_options(packtool PRIVATE /W4 /WX)
else()
	target_compile_options(packtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET packtool PROPERTY FOLDER "tools")
