include(CTest)

macro (OPENBLACK_SETUP_AND_ADD_TEST TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${TEST_SOURCE})
  add_dependencies(${TEST_NAME} generate-mock-game-data)
  target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main openblack_lib)
  target_compile_definitions(
    ${TEST_NAME} PRIVATE TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  )
  set_property(TARGET ${TEST_NAME} PROPERTY FOLDER "tests")
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro ()

openblack_setup_and_add_test(test_game_initialize test_game_initialize.cpp)
openblack_setup_and_add_test(test_load_scene test_load_scene.cpp)
openblack_setup_and_add_test(test_fixed test_fixed.cpp)

add_custom_target(
  test_mobile_wall_hug_scenarios
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/mobile_wall_hug/scenarios
    ${CMAKE_CURRENT_BINARY_DIR}/mobile_wall_hug/scenarios
)
openblack_setup_and_add_test(
  test_mobile_wall_hug mobile_wall_hug/test_mobile_wall_hug.cpp
)
add_dependencies(test_mobile_wall_hug test_mobile_wall_hug_scenarios)
