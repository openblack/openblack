set(L3DTOOL
	l3dtool.cpp
	json.hpp
	tiny_gltf.h
)

source_group(apps\\l3dtool FILES ${L3DTOOL})

add_executable(l3dtool ${L3DTOOL})

target_link_libraries(l3dtool
	PRIVATE
		cxxopts::cxxopts
		l3d
)

# FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
if (NOT MSVC AND CLANG_TIDY)
	set_target_properties(l3dtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

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
