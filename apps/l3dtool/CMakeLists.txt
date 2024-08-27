set(L3DTOOL l3dtool.cpp json.hpp tiny_gltf.h)

source_group(apps\\l3dtool FILES ${L3DTOOL})

add_executable(l3dtool ${L3DTOOL})

target_compile_definitions(l3dtool PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_link_libraries(l3dtool PRIVATE cxxopts::cxxopts l3d)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  if (CLANG_TIDY)
    set_target_properties(l3dtool PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(l3dtool PRIVATE _HAS_EXCEPTIONS=0)
  target_compile_options(l3dtool PRIVATE /W4 /WX /EHs-c-)
else ()
  target_compile_options(
    l3dtool
    PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
            # Disabled due to tiny-gltf
            -Wno-switch
  )
endif ()

set_property(TARGET l3dtool PROPERTY FOLDER "tools")
