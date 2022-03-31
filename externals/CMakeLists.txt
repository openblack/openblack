# Definitions for all external bundled libraries

add_library(imgui_user STATIC
    # user additions to imgui
    imgui_user/imgui_user.cpp imgui_user/imgui_user.h
    imgui_user/imgui_bitfield.cpp imgui_user/imgui_bitfield.h
    imgui_user/imgui_memory_editor.h
)

target_include_directories(imgui_user PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imgui_user)
target_link_libraries(imgui_user PUBLIC imgui::imgui)

set_property(TARGET imgui_user PROPERTY FOLDER "externals")
