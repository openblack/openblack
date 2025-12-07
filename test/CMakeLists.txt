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
