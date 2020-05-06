set(PACKTOOL
  packtool.cpp
)

source_group(apps\\packtool FILES ${PACKTOOL})

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_executable(packtool ${PACKTOOL})

target_link_libraries(packtool
	PRIVATE
		cxxopts::cxxopts
		pack
)

if(MSVC)
	target_compile_options(packtool PRIVATE /W4 /WX)
else()
	target_compile_options(packtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET packtool PROPERTY FOLDER "tools")
