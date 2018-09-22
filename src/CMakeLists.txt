set(OS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE OPENBLACK_SRCS RELATIVE
	"${CMAKE_CURRENT_SOURCE_DIR}"
    "**.cpp"
    "**.h"
)

include_directories(BEFORE "${OS_SRC_DIR}")

if (WIN32)
    # Suppress WinMain(), provided by SDL
    add_definitions(-DSDL_MAIN_HANDLED)
    # Get rid of useless crud from windows.h
	add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
	# disable CRT warnings on windows cause they're annoying as shit and we use C functions everywhere
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	# fast FP
	add_definitions(/fp:fast)

	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")
endif()

add_executable(OpenBlack ${OPENBLACK_SRCS})
set_target_properties(OpenBlack PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(OpenBlack PROPERTIES OUTPUT_NAME openblack)
set_target_properties(OpenBlack PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(OpenBlack PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(OpenBlack PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_link_libraries(OpenBlack ${SDL2_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})

if(WIN32)
	foreach(LIB ${SDL2_LIBRARY})
		string(REGEX REPLACE "\\.lib$" ".dll" SDL_DLL ${LIB})
		if(EXISTS "${SDL_DLL}")
			add_custom_command(TARGET OpenBlack POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_DLL}" "${CMAKE_BINARY_DIR}/bin/")
			install(PROGRAMS ${SDL_DLL} DESTINATION bin)
		endif()
	endforeach()
endif()
