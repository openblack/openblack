file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

add_library(file_utils STATIC ${SOURCES} ${HEADERS})

target_include_directories(file_utils
        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        )

target_link_libraries(file_utils
        PUBLIC
        spdlog::spdlog
        )

if (OPENBLACK_CLANG_TIDY_CHECKS)
    # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
    if (NOT MSVC AND CLANG_TIDY)
        set_target_properties(file_utils PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
    else ()
        message("Clang-tidy checks requested but unavailable")
    endif ()
endif ()

target_compile_definitions(file_utils
        PUBLIC
        # Compile-time optimize out log messages (debug level if debug, else info)
        "SPDLOG_ACTIVE_LEVEL=$<IF:$<CONFIG:DEBUG>,SPDLOG_LEVEL_DEBUG,SPDLOG_LEVEL_INFO>"
        )

if (MSVC)
    target_compile_options(file_utils PRIVATE /W4 /WX)
else ()
    target_compile_options(file_utils PRIVATE -Wall -Wextra -pedantic -Werror)
endif ()

set_property(TARGET file_utils PROPERTY FOLDER "components")
