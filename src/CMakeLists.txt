configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)

set(OS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE OPENBLACK_SRCS RELATIVE
	"${CMAKE_CURRENT_SOURCE_DIR}"
    "**.cpp"
    "**.h"
)

foreach(FILE ${OPENBLACK_SRCS})
add_definitions(-DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_RADIANS -DGLM_ENABLE_EXPERIMENTAL)

  string(REPLACE "/" "\\" GROUP "${PARENT_DIR}")

  if ("${FILE}" MATCHES ".*\\.cpp")
    set(GROUP "Source Files\\${GROUP}")
  elseif("${FILE}" MATCHES ".*\\.h")
    set(GROUP "Header Files\\${GROUP}")
  endif()

  source_group("${GROUP}" FILES "${FILE}")
endforeach()

file(GLOB OPENBLACK_SHADERS
    "${OS_BASE_DIR}/assets/shaders/**.vert"
    "${OS_BASE_DIR}/assets/shaders/**.frag"
)

include_directories(BEFORE "${OS_SRC_DIR}")
include_directories("${SCRIPTLIBRARY_INCLUDE_DIR}")

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

set(OPENBLACK_ICON "../assets/resource/icon/openblack.rc")

add_executable(OpenBlack ${OPENBLACK_SRCS} "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" ${OPENBLACK_SHADERS} ${OPENBLACK_ICON})
set_target_properties(OpenBlack PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(OpenBlack PROPERTIES CXX_STANDARD 17)
set_target_properties(OpenBlack PROPERTIES OUTPUT_NAME openblack)
set_target_properties(OpenBlack PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(OpenBlack PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(OpenBlack PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_link_libraries(OpenBlack ScriptLibrary ${SDL2_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARY})

if (WIN32)
	if (USE_DEBUG_CONSOLE)
		set_target_properties(OpenBlack PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(OpenBlack PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(OpenBlack PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
	else (USE_DEBUG_CONSOLE)
		# Turn off debug console, debug output will be written to visual studio output instead
		set_target_properties(OpenBlack PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
		set_target_properties(OpenBlack PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
	endif()

	# Release builds don't use the debug console
    set_target_properties(OpenBlack PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    set_target_properties(OpenBlack PROPERTIES COMPILE_DEFINITIONS_RELEASE "_WINDOWS")
	set_target_properties(OpenBlack PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif (WIN32)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OpenBlack)
set_property(TARGET OpenBlack PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

if(WIN32)
	foreach(LIB ${SDL2_LIBRARY})
		string(REGEX REPLACE "\\.lib$" ".dll" SDL_DLL ${LIB})
		if(EXISTS "${SDL_DLL}")
			add_custom_command(TARGET OpenBlack POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_DLL}" "${CMAKE_BINARY_DIR}/bin/")
			install(PROGRAMS ${SDL_DLL} DESTINATION bin)
		endif()
	endforeach()
endif()

foreach(SHADER ${OPENBLACK_SHADERS})
  source_group("Shaders" FILES "${SHADER}")
	if(WIN32)
		add_custom_command(TARGET OpenBlack PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER} "${CMAKE_BINARY_DIR}/bin/shaders/")
	elseif(UNIX)
		file(COPY ${SHADER} DESTINATION ${CMAKE_BINARY_DIR}/bin/shaders)
	endif(WIN32)
endforeach()
