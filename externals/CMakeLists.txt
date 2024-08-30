# Definitions for all external bundled libraries

add_library(
  imgui_user STATIC
  # user additions to imgui
  imgui_user/imgui_user.cpp
  imgui_user/imgui_user.h
  imgui_user/imgui_bitfield.cpp
  imgui_user/imgui_bitfield.h
  imgui_user/imgui_memory_editor.h
  # https://raw.githubusercontent.com/ocornut/imgui/master/backends/
  imgui_user/imgui_impl_sdl.cpp
  imgui_user/imgui_impl_sdl.h
  # https://github.com/bwrsandman/imgui-flame-graph
  imgui_user/imgui_widget_flamegraph.cpp
  imgui_user/imgui_widget_flamegraph.h
)

target_include_directories(
  imgui_user PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imgui_user
)
target_link_libraries(imgui_user PUBLIC imgui::imgui SDL2::SDL2)
target_compile_definitions(imgui_user PUBLIC IMGUI_DEFINE_MATH_OPERATORS)

set_property(TARGET imgui_user PROPERTY FOLDER "externals")

add_library(external::nlohmann_json INTERFACE IMPORTED GLOBAL)
set_target_properties(
  external::nlohmann_json
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
             "${CMAKE_CURRENT_LIST_DIR}/nlohmann/include/"
             FOLDER "externals"
)
