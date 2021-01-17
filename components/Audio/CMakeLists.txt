# Configure C++ standard

file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

# force glm left handedness
add_definitions(-DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_RADIANS
        -DGLM_ENABLE_EXPERIMENTAL)
add_library(Audio STATIC ${SOURCES} ${HEADERS})
find_path(DRLIBS_INCLUDE_DIRS "dr_wav.h")
find_path(DRLIBS_INCLUDE_DIRS "dr_mp3.h")
target_include_directories(Audio
        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE
        ${DRLIBS_INCLUDE_DIRS}
        )
target_link_libraries(
        Audio
        PRIVATE
        pack
        glm::glm
        spdlog::spdlog
        OpenAL::OpenAL
)

if (HAS_FILESYSTEM)
    message(STATUS "Got <filesystem>: add compiler definition HAS_FILESYSTEM")
    target_compile_definitions(Audio PRIVATE HAS_FILESYSTEM)
else ()
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        message(
                STATUS
                "No <filesystem>: linking stdc++fs on unknown compiler id: ${CMAKE_CXX_COMPILER_ID}"
        )
        target_link_libraries(Audio PRIVATE stdc++fs)
    else ()
        message(
                STATUS "No <filesystem> on unknown compiler id: ${CMAKE_CXX_COMPILER_ID}")
    endif ()
endif ()
