set(CAMTOOL camtool.cpp)

source_group(apps\\camtool FILES ${CAMTOOL})

add_executable(camtool ${CAMTOOL})

target_link_libraries(camtool PRIVATE cxxopts::cxxopts cam)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
    set_target_properties(camtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_options(camtool PRIVATE /W4 /WX)
else ()
  target_compile_options(camtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif ()

set_property(TARGET camtool PROPERTY FOLDER "tools")
