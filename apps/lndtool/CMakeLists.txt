set(LNDTOOL lndtool.cpp)

source_group(apps\\lndtool FILES ${LNDTOOL})

add_executable(lndtool ${LNDTOOL})

target_compile_definitions(lndtool PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_link_libraries(lndtool PRIVATE cxxopts::cxxopts lnd)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
    set_target_properties(lndtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(lndtool PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(lndtool PRIVATE /W4 /WX /EHs-c-)
else ()
  target_compile_options(
    lndtool PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

set_property(TARGET lndtool PROPERTY FOLDER "tools")
