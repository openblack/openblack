set(GLWTOOL glwtool.cpp json.hpp)

source_group(apps\\glwtool FILES ${GLWTOOL})

add_executable(glwtool ${GLWTOOL})

target_compile_definitions(glwtool PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_link_libraries(glwtool PRIVATE glw)
target_include_directories(glwtool PRIVATE ${CXXOPTS_INCLUDE_DIRS})

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
    set_target_properties(glwtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(glwtool PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(glwtool PRIVATE /W4 /WX /EHs-c-)
else ()
  target_compile_options(
    glwtool PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

set_property(TARGET glwtool PROPERTY FOLDER "tools")
