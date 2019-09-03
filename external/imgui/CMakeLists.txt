# add imgui library to use for debugging
add_library(imgui STATIC
	imgui/imconfig.h
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui.h
	imgui/imgui_internal.h
	imgui/imgui_widgets.cpp
	imgui/imstb_rectpack.h
	imgui/imstb_textedit.h
	imgui/imstb_truetype.h
	# opengl helper
	imgui/examples/imgui_impl_sdl.h
	imgui/examples/imgui_impl_sdl.cpp
	imgui/examples/imgui_impl_opengl3.h
	imgui/examples/imgui_impl_opengl3.cpp
	# memory_editor
	imgui_club/imgui_memory_editor/imgui_memory_editor.h
	)
target_include_directories(imgui PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/imgui/example)

# tell imgui we want to use GLEW as OpenGL loader
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW=1)

if(HUNTER_ENABLED)
	target_link_libraries(imgui PRIVATE
		SDL2::SDL2
		glew::glew )
else()
	target_include_directories(imgui PRIVATE
		${SDL2_INCLUDE_DIR}
		${GLEW_INCLUDE_DIR}
	)
	target_link_libraries(imgui PRIVATE
		${GLEW_LIBRARY}
	)
endif()
