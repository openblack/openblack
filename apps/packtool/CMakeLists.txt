set(PACKTOOL packtool.cpp)

source_group(apps\\packtool FILES ${PACKTOOL})

add_executable(packtool ${PACKTOOL})

target_compile_definitions(packtool PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_link_libraries(packtool PRIVATE pack)
target_include_directories(packtool PRIVATE ${CXXOPTS_INCLUDE_DIRS})

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
    set_target_properties(packtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(packtool PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(packtool PRIVATE /W4 /WX /EHs-c-)
else ()
  target_compile_options(
    packtool PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

set_property(TARGET packtool PROPERTY FOLDER "tools")
