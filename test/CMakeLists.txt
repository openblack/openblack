include(CTest)

# Macro for setting up and adding a simple test.
# TEST_NAME is the name of the test.
# TEST_SOURCE is the source file of the test.
macro (OPENBLACK_SETUP_AND_ADD_TEST TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${TEST_SOURCE})
  add_dependencies(${TEST_NAME} generate_mock_game_data)
  target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main openblack_lib)
  target_compile_definitions(
    ${TEST_NAME} PRIVATE TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
                         GLM_ENABLE_EXPERIMENTAL
  )
  set_property(TARGET ${TEST_NAME} PROPERTY FOLDER "tests")
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro ()

# Macro for setting up a test that uses JSON scenario files for validation.
# This macro does everything the standard test setup does, but also copies a
# directory of JSON scenarios into the build directory and adds a dependency on
# these scenarios.
# The TEST_NAME and TEST_SOURCE arguments have the same purpose as in the standard setup.
# The test should be in its own directory
macro (OPENBLACK_SETUP_AND_ADD_JSON_TEST TEST_NAME TEST_SOURCE)
  get_filename_component(TEST_DIR_NAME ${TEST_SOURCE} DIRECTORY)
  add_custom_target(
    ${TEST_NAME}_scenarios
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR_NAME}/scenarios
      ${CMAKE_CURRENT_BINARY_DIR}/${TEST_DIR_NAME}/scenarios
  )
  set_property(TARGET ${TEST_NAME}_scenarios PROPERTY FOLDER "tests")
  openblack_setup_and_add_test(${TEST_NAME} ${TEST_SOURCE})
  target_include_directories(
    ${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party
                         vector_compare.h
  )
  add_dependencies(${TEST_NAME} ${TEST_NAME}_scenarios)
endmacro ()

openblack_setup_and_add_test(test_game_initialize test_game_initialize.cpp)
openblack_setup_and_add_test(test_load_scene test_load_scene.cpp)
openblack_setup_and_add_test(test_fixed test_fixed.cpp)
openblack_setup_and_add_test(test_interpolator test_interpolator.cpp)
openblack_setup_and_add_test(test_set_camera_pos camera/test_set_camera_pos.cpp)
openblack_setup_and_add_json_test(
  test_mobile_wall_hug mobile_wall_hug/test_mobile_wall_hug.cpp
)
openblack_setup_and_add_json_test(test_camera camera/test_camera.cpp)
openblack_setup_and_add_test(test_sound_decoding audio/test_sound_decoding.cpp)
openblack_setup_and_add_test(test_music audio/test_music.cpp)
openblack_setup_and_add_test(test_music_decoding audio/test_music_decoding.cpp)
openblack_setup_and_add_test(test_anim_effects audio/test_anim_effects.cpp)
openblack_setup_and_add_test(test_screen_fade gui/test_screen_fade.cpp)
target_link_libraries(test_sound_decoding PRIVATE pack)
openblack_setup_and_add_test(test_hand_shadow hand/test_hand_shadow.cpp)
openblack_setup_and_add_test(
  test_object_shadows graphics/test_object_shadows.cpp
)
openblack_setup_and_add_test(test_hand_light graphics/test_hand_light.cpp)
openblack_setup_and_add_test(test_hand_water_glow graphics/test_hand_water_glow.cpp)
openblack_setup_and_add_test(test_model_light graphics/test_model_light.cpp)
openblack_setup_and_add_test(test_sun graphics/test_sun.cpp)
openblack_setup_and_add_test(test_moon graphics/test_moon.cpp)
openblack_setup_and_add_test(test_shader_samplers graphics/test_shader_samplers.cpp)
openblack_setup_and_add_test(test_pack_bank_info pack/test_pack_bank_info.cpp)
target_link_libraries(test_pack_bank_info PRIVATE pack)
openblack_setup_and_add_test(test_game_random test_game_random.cpp)
openblack_setup_and_add_test(test_crash_report test_crash_report.cpp)
openblack_setup_and_add_test(test_key_bindings input/test_key_bindings.cpp)
openblack_setup_and_add_test(test_particle_file test_particle_file.cpp)
target_link_libraries(test_particle_file PRIVATE psys)
openblack_setup_and_add_test(test_particle_maths test_particle_maths.cpp)
target_link_libraries(test_particle_maths PRIVATE psys)
openblack_setup_and_add_test(test_map_coords test_map_coords.cpp)
openblack_setup_and_add_test(test_land_normal test_land_normal.cpp)
openblack_setup_and_add_test(test_tree_brightness test_tree_brightness.cpp)
openblack_setup_and_add_test(test_block_texture test_block_texture.cpp)
target_link_libraries(test_block_texture PRIVATE lnd)
openblack_setup_and_add_test(test_sea_rows test_sea_rows.cpp)
openblack_setup_and_add_test(test_day_night_clock test_day_night_clock.cpp)
openblack_setup_and_add_test(test_mists test_mists.cpp)
openblack_setup_and_add_test(test_clouds test_clouds.cpp)
openblack_setup_and_add_test(test_village_lights test_village_lights.cpp)
openblack_setup_and_add_test(test_land_light_table test_land_light_table.cpp)
openblack_setup_and_add_test(test_lightning test_lightning.cpp)
openblack_setup_and_add_test(test_land_colour_stamps test_land_colour_stamps.cpp)
openblack_setup_and_add_test(test_sky_dome test_sky_dome.cpp)
openblack_setup_and_add_test(test_near_clipping test_near_clipping.cpp)
openblack_setup_and_add_test(test_ground_blobs test_ground_blobs.cpp)
openblack_setup_and_add_test(test_zsort test_zsort.cpp)
openblack_setup_and_add_test(test_rain test_rain.cpp)
openblack_setup_and_add_test(test_chimney_smoke test_chimney_smoke.cpp)
openblack_setup_and_add_test(test_influence_circle test_influence_circle.cpp)
openblack_setup_and_add_test(test_field_crop test_field_crop.cpp)
openblack_setup_and_add_test(test_snowfall test_snowfall.cpp)
openblack_setup_and_add_test(test_file_dialog test_file_dialog.cpp)
openblack_setup_and_add_test(test_vertex_blend test_vertex_blend.cpp)
openblack_setup_and_add_test(test_creature_shadow graphics/test_creature_shadow.cpp)
openblack_setup_and_add_test(test_gutils_angle test_gutils_angle.cpp)
openblack_setup_and_add_test(test_gutils_distance test_gutils_distance.cpp)
openblack_setup_and_add_test(test_l3d_lightmaps l3d/test_l3d_lightmaps.cpp)
target_link_libraries(test_l3d_lightmaps PRIVATE l3d)
openblack_setup_and_add_test(test_l3d_copy l3d/test_l3d_copy.cpp)
openblack_setup_and_add_test(test_l3d_ray_cast l3d/test_l3d_ray_cast.cpp)
target_link_libraries(test_l3d_ray_cast PRIVATE l3d)
target_link_libraries(test_l3d_copy PRIVATE l3d)
openblack_setup_and_add_test(
  test_l3d_submesh_names l3d/test_l3d_submesh_names.cpp
)
target_link_libraries(test_l3d_submesh_names PRIVATE l3d)
openblack_setup_and_add_test(test_light_beams graphics/test_light_beams.cpp)
target_link_libraries(test_light_beams PRIVATE l3d glw)
openblack_setup_and_add_test(test_frame_stats test_frame_stats.cpp)
openblack_setup_and_add_test(test_uniform_table graphics/test_uniform_table.cpp)
openblack_setup_and_add_test(test_editor_scripts test_editor_scripts.cpp)
target_link_libraries(test_editor_scripts PRIVATE ScriptLibrary)
openblack_setup_and_add_test(test_chl_language lhvm/test_chl_language.cpp)
target_link_libraries(test_chl_language PRIVATE lhvmlang)
openblack_setup_and_add_test(test_lhvm_decompiler lhvm/test_lhvm_decompiler.cpp)
target_link_libraries(test_lhvm_decompiler PRIVATE lhvmdecompiler)
openblack_setup_and_add_test(test_particle_miracles test_particle_miracles.cpp)
target_link_libraries(test_particle_miracles PRIVATE psys)
openblack_setup_and_add_test(test_chl_compiler lhvm/test_chl_compiler.cpp)
target_link_libraries(test_chl_compiler PRIVATE lhvmcompiler)
openblack_setup_and_add_test(test_chl_roundtrip lhvm/test_chl_roundtrip.cpp)
target_link_libraries(test_chl_roundtrip PRIVATE lhvmcompiler lhvmdecompiler)
openblack_setup_and_add_test(test_chl_forms lhvm/test_chl_forms.cpp)
target_link_libraries(test_chl_forms PRIVATE lhvmcompiler lhvmdecompiler)
openblack_setup_and_add_test(test_chl_constructs lhvm/test_chl_constructs.cpp)
target_link_libraries(test_chl_constructs PRIVATE lhvmcompiler lhvmdecompiler)
openblack_setup_and_add_test(test_chl_random lhvm/test_chl_random.cpp)
target_link_libraries(test_chl_random PRIVATE lhvmcompiler lhvmdecompiler)
