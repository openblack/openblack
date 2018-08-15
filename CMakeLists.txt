cmake_minimum_required(VERSION 3.2)

project(OpenBlack)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

if(MSVC OR MINGW)
	add_definitions(-D_WINDOWS)
	if(CMAKE_CL_64)
		add_definitions(-DWIN64)
	ELSE(CMAKE_CL_64)
		add_definitions(-DWIN32)
	ENDIF(CMAKE_CL_64)
endif(MSVC OR MINGW)

option(SDL2_STATIC "Link static build of SDL into the binaries" FALSE)

if (WIN32)
    option(USE_DEBUG_CONSOLE "whether a debug console should be enabled for debug builds, if false debug output is redirected to Visual Studio output" ON)
endif()

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLM REQUIRED)
#find_package(FFmpeg REQUIRED)
#find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE SWRESAMPLE)

if (WIN32)
    # Suppress WinMain(), provided by SDL
    add_definitions(-DSDL_MAIN_HANDLED)
    # Get rid of useless crud from windows.h
	add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
	# disable CRT warnings on windows cause they're annoying as shit and we use C functions everywhere
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	# fast FP
	add_definitions(/fp:fast)

    #foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    #    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    #    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "$(SolutionDir)$(Configuration)" )
    #    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "$(ProjectDir)$(Configuration)" )
	#endforeach( OUTPUTCONFIG )

	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")
endif()

include_directories("."
	SYSTEM
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${SDL2_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIRS}
	${GLM_INCLUDE_DIRS}
	${FFMPEG_INCLUDE_DIR}
)

file(GLOB_RECURSE OPENBLACK_SRCS RELATIVE
	"${CMAKE_CURRENT_SOURCE_DIR}"
    "src/**.cpp"
    "src/**.h"
)

add_subdirectory(files)

set(OPENBLACK_ICON "resource/icon/openblack.rc")

add_executable(openblack ${OPENBLACK_SRCS} ${OPENBLACK_ICON})
set_target_properties(openblack PROPERTIES C_STANDARD 99 CXX_STANDARD 11)

foreach(source IN LISTS OPENBLACK_SRCS)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

target_include_directories(
	openblack PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${SDL2_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIRS}
	${GLM_INCLUDE_DIRS}
	${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(
	openblack
	${SDL2_LIBRARY}
	${OPENGL_LIBRARY}
	${GLEW_LIBRARIES}
	${FFMPEG_LIBAVCODEC_LIBRARIES}
	${FFMPEG_LIBAVFORMAT_LIBRARIES}
	${FFMPEG_LIBAVUTIL_LIBRARIES}
)

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

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT openblack)
endif (MSVC)

# execute_process(COMMAND cmake -E copy ${CMAKE_CURRENT_SOURCE_DIR}/external/sdl-1.3.0-5206/binary/win32-vc2008/SDL.dll ${CMAKE_CURRENT_BINARY_DIR}/Debug/SDL.dll)

# Copy the license to the build directory (for CI)
#file(COPY COPYING
#    DESTINATION "${PROJECT_BINARY_DIR}"
#)