set(G3DTOOL
	g3dtool.cpp
)

source_group(apps\\g3dtool FILES ${G3DTOOL})

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_executable(g3dtool ${G3DTOOL})

target_link_libraries(g3dtool
	PRIVATE
		cxxopts::cxxopts
		g3d
)

if(MSVC)
	target_compile_options(g3dtool PRIVATE /W4 /WX)
else()
	target_compile_options(g3dtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET g3dtool PROPERTY FOLDER "tools")