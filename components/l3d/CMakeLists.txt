file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_library(l3d STATIC ${SOURCES} ${HEADERS})
target_include_directories(l3d
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
if(MSVC)
	target_compile_options(l3d PRIVATE /W4 /WX)
else()
	target_compile_options(l3d PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
