set(SADTOOL
  sadtool.cpp
)

source_group(apps\\sadtool FILES ${SADTOOL})

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_executable(sadtool ${SADTOOL})

target_link_libraries(sadtool
	PRIVATE
		cxxopts::cxxopts
		sad
)

if(MSVC)
	target_compile_options(sadtool PRIVATE /W4 /WX)
else()
	target_compile_options(sadtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET sadtool PROPERTY FOLDER "tools")
