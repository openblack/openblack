file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_library(pack STATIC ${SOURCES} ${HEADERS})
target_include_directories(pack
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
if(MSVC)
	target_compile_options(pack PRIVATE /W4 /WX)
else()
	target_compile_options(pack PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_property(TARGET pack PROPERTY FOLDER "components")
