set(GLWTOOL glwtool.cpp json.hpp tiny_gltf.h)

source_group(apps\\glwtool FILES ${GLWTOOL})

add_executable(glwtool ${GLWTOOL})

target_link_libraries(glwtool PRIVATE cxxopts::cxxopts glw)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
    set_target_properties(glwtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_options(glwtool PRIVATE /W4 /WX)
else ()
  target_compile_options(
    glwtool
    PRIVATE -Wall -Wextra -pedantic -Werror
            # Disabled due to tiny-gltf
            -Wno-switch
  )
endif ()

set_property(TARGET glwtool PROPERTY FOLDER "tools")
