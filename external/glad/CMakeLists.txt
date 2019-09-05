# OpenGL 3.3 Core with following extensions
# - EXT_texture_compression_s3tc
# - ARB_texture_storage // for https://www.khronos.org/opengl/wiki/GLAPI/glTexStorage3D
# - GL_ARB_debug_output
# - GL_KHR_debug

add_library(glad STATIC
    include/glad/glad.h
    include/KHR/khrplatform.h
    src/glad.c
)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT WIN32)
	target_link_libraries(glad PUBLIC dl)
endif()

