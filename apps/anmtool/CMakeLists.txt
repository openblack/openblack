set(ANMTOOL
	anmtool.cpp
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
			-Werror)
endif()

set_property(TARGET anmtool PROPERTY FOLDER "tools")
