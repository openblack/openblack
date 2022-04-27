set(LNDTOOL
  lndtool.cpp
)

source_group(apps\\lndtool FILES ${LNDTOOL})

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_executable(lndtool ${LNDTOOL})

target_link_libraries(lndtool
	PRIVATE
		cxxopts::cxxopts
		lnd
)

if(MSVC)
	target_compile_options(lndtool PRIVATE /W4 /WX)
else()
	target_compile_options(lndtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET lndtool PROPERTY FOLDER "tools")
