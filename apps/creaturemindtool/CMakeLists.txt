set(CREATUREMINDTOOL creaturemindtool.cpp)

source_group(apps\\creaturemindtool FILES ${CREATUREMINDTOOL})

add_executable(creaturemindtool ${CREATUREMINDTOOL})

target_link_libraries(creaturemindtool PRIVATE creaturemind)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
    set_target_properties(creaturemindtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(creaturemindtool PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(creaturemindtool PRIVATE /W4 /WX /EHs- /EHc-)
else ()
  target_compile_options(
    creaturemindtool PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

set_property(TARGET creaturemindtool PROPERTY FOLDER "tools")
