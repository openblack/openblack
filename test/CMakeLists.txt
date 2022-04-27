include(CTest)

macro(OPENBLACK_SETUP_AND_ADD_TEST TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    add_dependencies(${TEST_NAME} generate-mock-game-data)
    target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main openblack_lib)
    target_compile_definitions(${TEST_NAME} PRIVATE TEST_PWD="${CMAKE_CURRENT_LIST_DIR}")
    target_compile_definitions(${TEST_NAME} PRIVATE TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
    set_property(TARGET ${TEST_NAME} PROPERTY FOLDER "tests")
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

OPENBLACK_SETUP_AND_ADD_TEST(test_game_initialize test_game_initialize.cpp)
OPENBLACK_SETUP_AND_ADD_TEST(test_rng_manager rng_manager/test_rng_manager.cpp)
