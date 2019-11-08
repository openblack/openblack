set(L3DTOOL
	l3dtool.cpp
)

source_group(apps\\l3dtool FILES ${L3DTOOL})

set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_executable(l3dtool ${L3DTOOL})

target_link_libraries(l3dtool
	PRIVATE
		cxxopts::cxxopts
		l3d
)

if(MSVC)
	target_compile_options(l3dtool PRIVATE /W4 /WX)
else()
	target_compile_options(l3dtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
