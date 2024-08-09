set(LHVMTOOL lhvmtool.cpp)

source_group(apps\\lhvmtool FILES ${LHVMTOOL})

add_executable(lhvmtool ${LHVMTOOL})

target_link_libraries(lhvmtool PRIVATE cxxopts::cxxopts ScriptLibrary)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
    set_target_properties(lhvmtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_options(lhvmtool PRIVATE /W4 /WX)
else ()
  target_compile_options(lhvmtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif ()

set_property(TARGET lhvmtool PROPERTY FOLDER "tools")
