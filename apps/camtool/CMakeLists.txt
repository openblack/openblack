set(CAMTOOL camtool.cpp)

source_group(apps\\camtool FILES ${CAMTOOL})

add_executable(camtool ${CAMTOOL})

target_compile_definitions(camtool PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_link_libraries(camtool PRIVATE cam)
target_include_directories(camtool PRIVATE ${CXXOPTS_INCLUDE_DIRS})

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
    set_target_properties(camtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(camtool PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(camtool PRIVATE /W4 /WX /EHs-c-)
else ()
  target_compile_options(
    camtool PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

set_property(TARGET camtool PROPERTY FOLDER "tools")
