# Definitions for all external bundled libraries

# bgfx.cmake
# add_subdirectory(bgfx.cmake)

if (NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.h)
	message(FATAL_ERROR "imgui submodule not checked out. Run:\n    git submodule update --init externals/imgui")
endif()

add_library(imgui STATIC
    # config (we can probably just use defines for this)
	imgui_user/imconfig.h

    # imgui core files
    imgui/imgui.h imgui/imgui_internal.h imgui/imstb_rectpack.h imgui/imstb_textedit.h imgui/imstb_truetype.h
	imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
    # imgui sdl2 backend
    imgui/backends/imgui_impl_sdl.cpp imgui/backends/imgui_impl_sdl.h

    # user additions to imgui
    imgui_user/imgui_user.cpp imgui_user/imgui_user.h
    imgui_user/imgui_bitfield.cpp imgui_user/imgui_bitfield.h
    imgui_user/imgui_memory_editor.h
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/imgui
    ${CMAKE_CURRENT_LIST_DIR}/imgui_user
)

target_link_libraries(imgui PUBLIC SDL2::SDL2)

set_property(TARGET imgui PROPERTY FOLDER "externals")
