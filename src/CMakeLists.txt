file(GLOB_RECURSE OPENBLACK_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE OPENBLACK_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")

set(OPENBLACK_RC ${CMAKE_SOURCE_DIR}/assets/resource/icon/openblack.rc)

# force glm left handedness
add_definitions(-DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_RADIANS -DGLM_ENABLE_EXPERIMENTAL)

configure_file("${CMAKE_CURRENT_LIST_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)

file(GLOB OPENBLACK_IMGUI_VS_SHADERS "${CMAKE_SOURCE_DIR}/assets/shaders/imgui/vs_*.sc")
file(GLOB OPENBLACK_IMGUI_FS_SHADERS "${CMAKE_SOURCE_DIR}/assets/shaders/imgui/fs_*.sc")
mark_shaders_for_compilation(
	HEADER
	TYPE VERTEX
	SHADERS ${OPENBLACK_IMGUI_VS_SHADERS}
	VARYING_DEF ${CMAKE_SOURCE_DIR}/assets/shaders/imgui/varying.def.sc
	OUTPUT_DIR ${CMAKE_BINARY_DIR}/include/generated/imgui/shaders)
mark_shaders_for_compilation(
	HEADER
	TYPE FRAGMENT
	SHADERS ${OPENBLACK_IMGUI_FS_SHADERS}
	VARYING_DEF ${CMAKE_SOURCE_DIR}/assets/shaders/imgui/varying.def.sc
	OUTPUT_DIR ${CMAKE_BINARY_DIR}/include/generated/imgui/shaders)

file(GLOB OPENBLACK_VERTEX_SHADERS "${CMAKE_SOURCE_DIR}/assets/shaders/vs_*.sc")
file(GLOB OPENBLACK_FRAGMENT_SHADERS "${CMAKE_SOURCE_DIR}/assets/shaders/fs_*.sc")
file(GLOB OPENBLACK_VARYING_DEF_FILE "${CMAKE_SOURCE_DIR}/assets/shaders/varying.def.sc")
mark_shaders_for_compilation(
	TYPE VERTEX
	SHADERS ${OPENBLACK_VERTEX_SHADERS}
	VARYING_DEF ${OPENBLACK_VARYING_DEF_FILE}
	OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/shaders)
mark_shaders_for_compilation(
	TYPE FRAGMENT
	SHADERS ${OPENBLACK_FRAGMENT_SHADERS}
	VARYING_DEF ${OPENBLACK_VARYING_DEF_FILE}
	OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/shaders)

list(APPEND OPENBLACK_SHADERS ${OPENBLACK_IMGUI_VS_SHADERS})
list(APPEND OPENBLACK_SHADERS ${OPENBLACK_IMGUI_FS_SHADERS})
list(APPEND OPENBLACK_SHADERS ${CMAKE_SOURCE_DIR}/assets/shaders/imgui/varying.def.sc)
list(APPEND OPENBLACK_SHADERS ${OPENBLACK_VERTEX_SHADERS})
list(APPEND OPENBLACK_SHADERS ${OPENBLACK_FRAGMENT_SHADERS})
list(APPEND OPENBLACK_SHADERS ${OPENBLACK_VARYING_DEF_FILE})

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
		add_compile_options(/fp:fast)

		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")
	endif()
endif()

add_executable(openblack
	${OPENBLACK_SOURCES}
	${OPENBLACK_HEADERS}
	"${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp"
	${OPENBLACK_RC}
	${OPENBLACK_SHADERS}
	${OPENBLACK_VERTEX_SHADERS}
	${OPENBLACK_FRAGMENT_SHADERS}
	${OPENBLACK_VARYING_DEF_FILE})
set_target_properties(openblack PROPERTIES LINKER_LANGUAGE CXX)

if(MSVC)
	target_compile_options(
		openblack
		PRIVATE
		/W4
#		/WX
	)
else(MSVC)
	target_compile_options(
		openblack
		PRIVATE
#		-Werror
		-Wall
		-Wextra
		-pedantic
		# disable acceptable errors
		-Wno-unused-private-field
		-Wno-unused-variable
		-Wno-unused-parameter
	)
endif(MSVC)

target_include_directories(openblack PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../include)
target_link_libraries(openblack PRIVATE
	imgui
	ScriptLibrary
	SDL2::SDL2main
	# SDL2::SDL2-static
	${OPENGL_LIBRARIES}
	glm
	spdlog::spdlog
	EnTT
	# bgfx::bgfx
)
if (UNIX)
	find_library(WAYLAND wayland-egl)
	if (WAYLAND)
		target_link_libraries(openblack PRIVATE ${WAYLAND})
	endif()
endif()

if (HAS_FILESYSTEM)
	message(STATUS "Got <filesystem>: add compiler definition HAS_FILESYSTEM")
	target_compile_definitions(openblack PRIVATE HAS_FILESYSTEM)
else()
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		message(STATUS "No <filesystem>: linking stdc++fs on unknown compiler id: ${CMAKE_CXX_COMPILER_ID}")
		target_link_libraries(openblack PRIVATE stdc++fs)
	else()
		message(STATUS "No <filesystem> on unknown compiler id: ${CMAKE_CXX_COMPILER_ID}")
	endif()
endif()

if (MSVC)
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
endif ()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT openblack)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${OPENBLACK_SOURCES} ${OPENBLACK_HEADERS})

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders")

foreach(SHADER ${OPENBLACK_SHADERS})
	source_group("Shaders" FILES "${SHADER}")
	if(WIN32)
		add_custom_command(TARGET openblack PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER} "${CMAKE_BINARY_DIR}/bin/shaders/")
	elseif(UNIX)
		file(COPY ${SHADER} DESTINATION ${CMAKE_BINARY_DIR}/bin/shaders)
	endif(WIN32)
endforeach()

# Installation configuration
if (UNIX)
  install(TARGETS openblack RUNTIME DESTINATION bin)
  install(FILES ${OPENBLACK_SHADERS}
          DESTINATION bin/shaders)
  install(FILES ../external/imgui/imgui/LICENSE.txt
          DESTINATION "share/licenses/openblack"
          RENAME "IMGUI_LICENSE.txt")
  install(FILES "${CMAKE_SOURCE_DIR}/assets/resource/shortcut/openblack.desktop"
          DESTINATION "share/applications")
  foreach(RES 64 128 256)
      install(FILES "${CMAKE_SOURCE_DIR}/assets/resource/icon/openblack-${RES}x${RES}.png"
              DESTINATION "share/icons/hicolor/${RES}x${RES}/apps"
              RENAME "openblack.png")
  endforeach()
endif()
