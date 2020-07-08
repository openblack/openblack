file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_library(morph STATIC ${SOURCES} ${HEADERS})
target_include_directories(morph
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
if(MSVC)
	target_compile_options(morph PRIVATE /W4 /WX)
else()
	target_compile_options(morph PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET morph PROPERTY FOLDER "components")
