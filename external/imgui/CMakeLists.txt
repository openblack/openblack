# add imgui library to use for debugging

if(${CMAKE_VERSION} VERSION_LESS 3.14)
	include(${CMAKE_SOURCE_DIR}/cmake/backport/FetchContent.cmake)
else()
	include(FetchContent)
endif()

FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG        master
)
FetchContent_Declare(
	imgui_club
	GIT_REPOSITORY https://github.com/ocornut/imgui_club.git
	GIT_TAG        master
)
FetchContent_MakeAvailable(imgui)
FetchContent_MakeAvailable(imgui_club)

add_library(imgui STATIC
	${imgui_SOURCE_DIR}/imconfig.h
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_demo.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui.h
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_internal.h
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/imstb_rectpack.h
	${imgui_SOURCE_DIR}/imstb_textedit.h
	${imgui_SOURCE_DIR}/imstb_truetype.h
	# opengl helper
	${imgui_SOURCE_DIR}/examples/imgui_impl_sdl.h
	${imgui_SOURCE_DIR}/examples/imgui_impl_sdl.cpp
	${imgui_SOURCE_DIR}/examples/imgui_impl_opengl3.h
	${imgui_SOURCE_DIR}/examples/imgui_impl_opengl3.cpp
	# memory_editor
	${imgui_club_SOURCE_DIR}/imgui_memory_editor/imgui_memory_editor.h
	)
target_include_directories(imgui PUBLIC
	${imgui_SOURCE_DIR}
	${imgui_SOURCE_DIR}/examples
	${imgui_club_SOURCE_DIR})
# tell imgui we want to use GLAD as OpenGL loader
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

target_link_libraries(imgui PUBLIC
	SDL2::SDL2
	glad
	${OPENGL_LIBRARIES})
