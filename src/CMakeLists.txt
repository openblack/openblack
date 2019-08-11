file(GLOB_RECURSE OPENBLACK_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE OPENBLACK_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")

set(OPENBLACK_RC ${CMAKE_SOURCE_DIR}/assets/resource/icon/openblack.rc)

# force glm left handedness
add_definitions(-DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_RADIANS -DGLM_ENABLE_EXPERIMENTAL)

configure_file("${CMAKE_CURRENT_LIST_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)

file(GLOB OPENBLACK_SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/**.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/**.frag"
)

if (WIN32)
    # Suppress WinMain(), provided by SDL
    add_definitions(-DSDL_MAIN_HANDLED)
    # Get rid of useless crud from windows.h
	add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
	# disable CRT warnings on windows cause they're annoying as shit and we use C functions everywhere
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)

	if (MSVC)
		# build on all cores
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /nologo")

		# fast FP
		add_definitions(/fp:fast)

		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")
	endif()
endif()

add_executable(openblack ${OPENBLACK_SOURCES} ${OPENBLACK_HEADERS} "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" ${OPENBLACK_RC} ${OPENBLACK_SHADERS})
set_target_properties(openblack PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(openblack PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(openblack PRIVATE ScriptLibrary SDL2::Main ${OPENGL_LIBRARIES} ${GLEW_LIBRARY})

if (WIN32)
	if (USE_DEBUG_CONSOLE)
		set_target_properties(openblack PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(openblack PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(openblack PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
	else (USE_DEBUG_CONSOLE)
		# Turn off debug console, debug output will be written to visual studio output instead
		set_target_properties(openblack PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
		set_target_properties(openblack PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
	endif()

	# Release builds don't use the debug console
    set_target_properties(openblack PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    set_target_properties(openblack PROPERTIES COMPILE_DEFINITIONS_RELEASE "_WINDOWS")
	set_target_properties(openblack PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif (WIN32)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT openblack)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${OPENBLACK_SOURCES} ${OPENBLACK_HEADERS})

# copy SDL2.dll & soft_oal.dll
if (MSVC)
	function(windows_copy_files TARGET SOURCE_DIR DEST_DIR)
		string(REPLACE "/" "\\\\" SOURCE_DIR ${SOURCE_DIR})
		string(REPLACE "/" "\\\\" DEST_DIR ${DEST_DIR})
		add_custom_command(TARGET ${TARGET} POST_BUILD
			COMMAND if not exist ${DEST_DIR} mkdir ${DEST_DIR} 2> nul
			COMMAND robocopy ${SOURCE_DIR} ${DEST_DIR} ${ARGN} /NJH /NJS /NDL /NFL /NC /NS /NP || cmd /c "exit /b 0"
		)
	endfunction()

    # windows_copy_files(openblack ${SDL2_DLL_DIR} "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/" SDL2.dll)
    # windows_copy_files(openblack ${OPENAL_DLL_DIR} "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/" soft_oal.dll)
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders")

foreach(SHADER ${OPENBLACK_SHADERS})
  source_group("Shaders" FILES "${SHADER}")
	if(WIN32)
		add_custom_command(TARGET openblack PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER} "${CMAKE_BINARY_DIR}/bin/shaders/")
	elseif(UNIX)
		file(COPY ${SHADER} DESTINATION ${CMAKE_BINARY_DIR}/bin/shaders)
	endif(WIN32)
endforeach()
