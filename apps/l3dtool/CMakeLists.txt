set(L3DTOOL
	l3dtool.cpp
	json.hpp
	tiny_gltf.h
)

source_group(apps\\l3dtool FILES ${L3DTOOL})

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_executable(l3dtool ${L3DTOOL})

target_link_libraries(l3dtool
	PRIVATE
		cxxopts::cxxopts
		l3d
)

if(MSVC)
	target_compile_options(l3dtool PRIVATE /W4 /WX)
else()
	target_compile_options(l3dtool
			PRIVATE
			-Wall
			-Wextra
			-pedantic
			-Werror
			# Disabled due to tiny-gltf
			-Wno-switch)
endif()

set_property(TARGET l3dtool PROPERTY FOLDER "tools")
