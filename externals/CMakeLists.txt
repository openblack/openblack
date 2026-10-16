# Definitions for all external bundled libraries

add_library(
  imgui_user STATIC
  # user additions to imgui
  imgui_user/imgui_user.cpp
  imgui_user/imgui_user.h
  imgui_user/imgui_bitfield.cpp
  imgui_user/imgui_bitfield.h
  imgui_user/imgui_memory_editor.h
  # https://github.com/ocornut/imgui/tree/v1.92.9b/backends
  imgui_user/imgui_impl_sdl3.cpp
  imgui_user/imgui_impl_sdl3.h
  imgui_user/imgui_impl_sdlgpu3.cpp
  imgui_user/imgui_impl_sdlgpu3.h
  imgui_user/imgui_impl_sdlgpu3_shaders.h
  # https://github.com/bwrsandman/imgui-flame-graph
  imgui_user/imgui_widget_flamegraph.cpp
  imgui_user/imgui_widget_flamegraph.h
)

target_include_directories(
  imgui_user PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imgui_user
)
target_link_libraries(imgui_user PUBLIC imgui::imgui SDL3::SDL3)
target_compile_definitions(imgui_user PUBLIC IMGUI_DEFINE_MATH_OPERATORS)

set_property(TARGET imgui_user PROPERTY FOLDER "externals")
