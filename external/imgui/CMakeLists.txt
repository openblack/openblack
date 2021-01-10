add_library(imgui STATIC
	imconfig.h
	imgui.cpp
	imgui_demo.cpp
	imgui_draw.cpp
	imgui.h
	imgui.cpp
	imgui_internal.h
	imgui_widgets.cpp
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
	# opengl helper
	imgui_impl_sdl.h
	imgui_impl_sdl.cpp
	# memory_editor
	imgui_memory_editor.h
	# imgui_bitfield
	imgui_bitfield.h
	imgui_bitfield.cpp
	# imgui_user
	imgui_user.h
	imgui_user.cpp
	)
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(imgui PUBLIC SDL2::SDL2)

set_property(TARGET imgui PROPERTY FOLDER "thirdparty")
