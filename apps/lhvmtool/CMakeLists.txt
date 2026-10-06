# A desktop tool: Android's executables can't link the format library vcpkg builds for its newer API level
if (ANDROID)
  return()
endif ()

set(LHVMTOOL lhvmtool.cpp lhvmtool_compile.cpp lhvmtool_compile.h)

source_group(apps\\lhvmtool FILES ${LHVMTOOL})

add_executable(lhvmtool ${LHVMTOOL})

target_compile_definitions(lhvmtool PRIVATE CXXOPTS_NO_EXCEPTIONS)
find_package(fmt CONFIG REQUIRED)
target_link_libraries(
  lhvmtool PRIVATE ScriptLibrary lhvmdecompiler lhvmcompiler fmt::fmt
)
target_include_directories(lhvmtool PRIVATE ${CXXOPTS_INCLUDE_DIRS})

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
    set_target_properties(lhvmtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(lhvmtool PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(lhvmtool PRIVATE /W4 /WX /EHs-c-)
else ()
  target_compile_options(
    lhvmtool PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

set_property(TARGET lhvmtool PROPERTY FOLDER "tools")
