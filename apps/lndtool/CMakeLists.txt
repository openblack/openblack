set(LNDTOOL lndtool.cpp)

source_group(apps\\lndtool FILES ${LNDTOOL})

add_executable(lndtool ${LNDTOOL})

target_compile_definitions(lndtool PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_link_libraries(lndtool PRIVATE lnd)
target_include_directories(lndtool PRIVATE ${CXXOPTS_INCLUDE_DIRS})

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
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
