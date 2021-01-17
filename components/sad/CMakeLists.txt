file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_library(sad STATIC ${SOURCES} ${HEADERS})
target_include_directories(sad
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
if(MSVC)
	target_compile_options(sad PRIVATE /W4 /WX)
else()
	target_compile_options(sad PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

if(HAS_FILESYSTEM)
	message(STATUS "Got <filesystem>: add compiler definition HAS_FILESYSTEM")
	target_compile_definitions(sad PRIVATE HAS_FILESYSTEM)
else()
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		message(
				STATUS
				"No <filesystem>: linking stdc++fs on unknown compiler id: ${CMAKE_CXX_COMPILER_ID}"
		)
		target_link_libraries(sad PRIVATE stdc++fs)
	else()
		message(
				STATUS "No <filesystem> on unknown compiler id: ${CMAKE_CXX_COMPILER_ID}")
	endif()
endif()

set_property(TARGET sad PROPERTY FOLDER "components")
