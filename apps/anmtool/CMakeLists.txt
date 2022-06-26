set(ANMTOOL anmtool.cpp json.hpp tiny_gltf.h)

source_group(apps\\anmtool FILES ${ANMTOOL})

add_executable(anmtool ${ANMTOOL})

target_link_libraries(anmtool PRIVATE cxxopts::cxxopts anm)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
    set_target_properties(anmtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_options(anmtool PRIVATE /W4 /WX)
else ()
  target_compile_options(
    anmtool
    PRIVATE -Wall -Wextra -pedantic -Werror
            # Disabled due to tiny-gltf
            -Wno-switch
  )
endif ()

set_property(TARGET anmtool PROPERTY FOLDER "tools")
