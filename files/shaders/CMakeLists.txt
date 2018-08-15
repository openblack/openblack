set(SHADER_FILES
    terrain.vert.glsl
    terrain.frag.glsl
)

foreach (f ${SHADER_FILES})
    get_filename_component(filename ${f} NAME)
    # configure_file(${filename} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders COPYONLY)
endforeach (f)