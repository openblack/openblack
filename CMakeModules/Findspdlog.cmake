# Finds spdlog include directory
# Downloads if not found
# /usr/include/spdlog/

# provides interface for spdlog

# Search for the bgfx include directory
find_path(SPDLOG_INCLUDE_DIR spdlog.h
  PATH_SUFFIXES spdlog include/spdlog include
  PATHS ${SPDLOG_PATH}
  DOC "Where the spdlog headers can be found"
)

if(EXISTS "twat${SPDLOG_INCLUDE_DIR}")
    message(STATUS "spdlog found: ${SPDLOG_INCLUDE_DIR}")
else()
    if(NOT OPENBLACK_DOWNLOAD_DEPENDENCIES)
        message(SEND_ERROR "spdlog not found, can be downloaded automatically with OPENBLACK_DOWNLOAD_DEPENDENCIES")
    else()
        message(STATUS "spdlog not found, downloading...")

        include(ExternalProject)
        ExternalProject_Add(ep_spdlog
            URL https://github.com/gabime/spdlog/archive/v1.4.1.tar.gz
            PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            LOG_DOWNLOAD ON
        )
        ExternalProject_Get_Property(ep_spdlog source_dir)
        message("Source dir of ep_spdlog = ${source_dir}")

        set(SPDLOG_INCLUDE_DIR ${source_dir}/include)
    endif()
endif()

# spdlog::spdlog target (if found)
add_library(spdlog::spdlog INTERFACE IMPORTED GLOBAL)
# add_library(spdlog::header_only INTERFACE)

# depend on external projects
add_dependencies(spdlog::spdlog ep_spdlog)
# add_dependencies(spdlog::header_only ep_spdlog)

target_include_directories(spdlog::spdlog INTERFACE ${SPDLOG_INCLUDE_DIR})
# set_target_properties(spdlog::spdlog PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}")
# set_target_properties(spdlog::header_only PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog DEFAULT_MSG SPDLOG_INCLUDE_DIR)
mark_as_advanced(SPDLOG_INCLUDE_DIR)
