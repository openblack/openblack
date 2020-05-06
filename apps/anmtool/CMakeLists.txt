set(ANMTOOL
	anmtool.cpp
	json.hpp
	tiny_gltf.h
)

source_group(apps\\anmtool FILES ${ANMTOOL})

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_executable(anmtool ${ANMTOOL})

target_link_libraries(anmtool
	PRIVATE
		cxxopts::cxxopts
		anm
)

if(MSVC)
	target_compile_options(anmtool PRIVATE /W4 /WX)
else()
	target_compile_options(anmtool
			PRIVATE
			-Wall
			-Wextra
			-pedantic
			-Werror
			# Disabled due to tiny-gltf
			-Wno-ignored-qualifiers)
endif()

set_property(TARGET anmtool PROPERTY FOLDER "tools")
