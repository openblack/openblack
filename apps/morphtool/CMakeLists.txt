set(MORPHTOOL
		morphtool.cpp
        )

source_group(apps\\morphtool FILES ${MORPHTOOL})

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_executable(morphtool ${MORPHTOOL})

target_link_libraries(morphtool
	PRIVATE
		cxxopts::cxxopts
		morph
)

if(MSVC)
	target_compile_options(morphtool PRIVATE /W4 /WX)
else()
	target_compile_options(morphtool
			PRIVATE
			-Wall
			-Wextra
			-pedantic
			-Werror)
endif()

set_property(TARGET morphtool PROPERTY FOLDER "tools")
