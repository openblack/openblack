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

# FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
if (NOT MSVC AND CLANG_TIDY)
	set_target_properties(packtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

if(MSVC)
	target_compile_options(packtool PRIVATE /W4 /WX)
else()
	target_compile_options(packtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET packtool PROPERTY FOLDER "tools")
