set(MORPHTOOL morphtool.cpp)

source_group(apps\\morphtool FILES ${MORPHTOOL})

add_executable(morphtool ${MORPHTOOL})

target_compile_definitions(morphtool PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_link_libraries(morphtool PRIVATE cxxopts::cxxopts morph)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
    set_target_properties(morphtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(morphtool PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(morphtool PRIVATE /W4 /WX /EHs- /EHc-)
else ()
  target_compile_options(
    morphtool PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

set_property(TARGET morphtool PROPERTY FOLDER "tools")
