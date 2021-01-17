file(GLOB_RECURSE OPENBLACK_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE OPENBLACK_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")

set(OPENBLACK_RC ${CMAKE_SOURCE_DIR}/assets/resource/icon/openblack.rc)

# force glm left handedness
add_definitions(-DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_RADIANS
                -DGLM_ENABLE_EXPERIMENTAL)

configure_file("${CMAKE_CURRENT_LIST_DIR}/GitSHA1.cpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)

file(GLOB OPENBLACK_IMGUI_VS_SHADERS
     "${CMAKE_SOURCE_DIR}/assets/shaders/imgui/vs_*.sc")
file(GLOB OPENBLACK_IMGUI_FS_SHADERS
     "${CMAKE_SOURCE_DIR}/assets/shaders/imgui/fs_*.sc")
mark_shaders_for_compilation(
  TYPE
  VERTEX
  SHADERS
  ${OPENBLACK_IMGUI_VS_SHADERS}
  VARYING_DEF
  ${CMAKE_SOURCE_DIR}/assets/shaders/imgui/varying.def.sc
  OUTPUT_DIR
  ${CMAKE_BINARY_DIR}/include/generated/shaders/imgui)
mark_shaders_for_compilation(
  TYPE
  FRAGMENT
  SHADERS
  ${OPENBLACK_IMGUI_FS_SHADERS}
  VARYING_DEF
  ${CMAKE_SOURCE_DIR}/assets/shaders/imgui/varying.def.sc
  OUTPUT_DIR
  ${CMAKE_BINARY_DIR}/include/generated/shaders/imgui)

file(GLOB OPENBLACK_VERTEX_SHADERS "${CMAKE_SOURCE_DIR}/assets/shaders/vs_*.sc")
file(GLOB OPENBLACK_FRAGMENT_SHADERS
     "${CMAKE_SOURCE_DIR}/assets/shaders/fs_*.sc")
file(GLOB OPENBLACK_VARYING_DEF_FILE
     "${CMAKE_SOURCE_DIR}/assets/shaders/varying.def.sc")

mark_shaders_for_compilation(
  TYPE
  VERTEX
  SHADERS
  ${OPENBLACK_VERTEX_SHADERS}
  VARYING_DEF
  ${OPENBLACK_VARYING_DEF_FILE}
  OUTPUT_DIR
  ${CMAKE_BINARY_DIR}/include/generated/shaders)
mark_shaders_for_compilation(
  TYPE
  FRAGMENT
  SHADERS
  ${OPENBLACK_FRAGMENT_SHADERS}
  VARYING_DEF
  ${OPENBLACK_VARYING_DEF_FILE}
  OUTPUT_DIR
  ${CMAKE_BINARY_DIR}/include/generated/shaders)

list(APPEND OPENBLACK_SHADERS ${OPENBLACK_IMGUI_VS_SHADERS})
list(APPEND OPENBLACK_SHADERS ${OPENBLACK_IMGUI_FS_SHADERS})
list(APPEND OPENBLACK_SHADERS
            ${CMAKE_SOURCE_DIR}/assets/shaders/imgui/varying.def.sc)
list(APPEND OPENBLACK_SHADERS ${OPENBLACK_VERTEX_SHADERS})
list(APPEND OPENBLACK_SHADERS ${OPENBLACK_FRAGMENT_SHADERS})
list(APPEND OPENBLACK_SHADERS ${OPENBLACK_VARYING_DEF_FILE})

add_executable(
  openblack
  ${OPENBLACK_SOURCES}
  ${OPENBLACK_HEADERS}
  "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp"
  ${OPENBLACK_RC}
  ${OPENBLACK_SHADERS}
  ${OPENBLACK_VERTEX_SHADERS}
  ${OPENBLACK_FRAGMENT_SHADERS}
  ${OPENBLACK_VARYING_DEF_FILE})
set_target_properties(openblack PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  openblack PRIVATE ${CMAKE_CURRENT_LIST_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}/../include)
target_link_libraries(
  openblack
  PRIVATE
          "$<$<CXX_COMPILER_ID:MSVC>:-SAFESEH:NO>"
          l3d
          pack
          lnd
          anm
          morph
          sad
          imgui
          imgui-flame-graph
          ScriptLibrary
          SDL2::SDL2
          # SDL2::SDL2-static
          glm
          spdlog::spdlog
          EnTT::EnTT
          bgfx::bgfx
          OpenAL::OpenAL
          cxxopts::cxxopts
          ${FFmpeg_LIBRARIES}
)
if(UNIX)
  find_library(WAYLAND wayland-egl)
  if(WAYLAND)
    target_link_libraries(openblack PRIVATE ${WAYLAND})
  endif()
endif()

if(HAS_FILESYSTEM)
  message(STATUS "Got <filesystem>: add compiler definition HAS_FILESYSTEM")
  target_compile_definitions(openblack PRIVATE HAS_FILESYSTEM)
else()
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(
      STATUS
        "No <filesystem>: linking stdc++fs on unknown compiler id: ${CMAKE_CXX_COMPILER_ID}"
    )
    target_link_libraries(openblack PRIVATE stdc++fs)
  else()
    message(
      STATUS "No <filesystem> on unknown compiler id: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endif()

target_compile_definitions(openblack PRIVATE "$<$<CONFIG:DEBUG>:OPENBLACK_DEBUG>")

if(MSVC)
  set(CMAKE_CONFIGURATION_TYPES
    Debug Release
    CACHE STRING "" FORCE)

  target_compile_definitions(openblack
    PRIVATE
    # Suppress WinMain(), provided by SDL
    SDL_MAIN_HANDLED
    # Get rid of useless crud from windows.h
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    # disable CRT warnings on windows cause they're annoying as shit and we use C
    # functions everywhere
    _CRT_SECURE_NO_WARNINGS
    )

  target_compile_options(openblack PRIVATE
    # Level 4 warnings
    /W4
    # Multi-threaded
    /MP
    # Enables stricter C++ standards conformance checks
    /permissive-
    )

  # Debug builds create a debug console
  set_target_properties(openblack PROPERTIES LINK_FLAGS_DEBUG
                                             "/SUBSYSTEM:CONSOLE")
  set_target_properties(openblack PROPERTIES LINK_FLAGS_RELWITHDEBINFO
                                             "/SUBSYSTEM:CONSOLE")
  set_target_properties(openblack PROPERTIES COMPILE_DEFINITIONS_DEBUG
                                             "_CONSOLE")

  # Release builds also have a console, for now..
  set_target_properties(openblack PROPERTIES LINK_FLAGS_RELEASE
                                             "/SUBSYSTEM:CONSOLE")
  set_target_properties(openblack PROPERTIES COMPILE_DEFINITIONS_RELEASE
                                             "CONSOLE")
  set_target_properties(openblack PROPERTIES LINK_FLAGS_MINSIZEREL
                                             "/SUBSYSTEM:CONSOLE")

  if (OPENBLACK_WARNINGS_AS_ERRORS)
    target_compile_options(openblack PRIVATE /WX)
  endif()
else()
  target_compile_options(openblack PRIVATE
    -Wall
    -Wextra
    -pedantic
    -Wno-unused-private-field
    -Wno-unused-variable
    -Wno-unused-parameter)
  if (OPENBLACK_WARNINGS_AS_ERRORS)
    target_compile_options(openblack PRIVATE -Werror)
  endif()
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT
                                                            openblack)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${OPENBLACK_SOURCES}
             ${OPENBLACK_HEADERS})

foreach(SHADER ${OPENBLACK_SHADERS})
  source_group("Shaders" FILES "${SHADER}")
endforeach()

# Installation configuration
install(TARGETS openblack RUNTIME DESTINATION bin)
install(
  FILES ../external/imgui/LICENSE.txt
  DESTINATION "share/licenses/openblack"
  RENAME "IMGUI_LICENSE.txt")
if(UNIX)
  install(FILES "${CMAKE_SOURCE_DIR}/assets/resource/shortcut/openblack.desktop"
          DESTINATION "share/applications")
  foreach(RES 64 128 256)
    install(
      FILES
        "${CMAKE_SOURCE_DIR}/assets/resource/icon/openblack-${RES}x${RES}.png"
      DESTINATION "share/icons/hicolor/${RES}x${RES}/apps"
      RENAME "openblack.png")
  endforeach()
endif()
