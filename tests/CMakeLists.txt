include(FetchContent)

enable_language(CXX)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)
include(CTest)


configure_file(test.mov ${CMAKE_CURRENT_BINARY_DIR}/test.mov COPYONLY)

set(test_executable_name ${PROJECT_NAME}_test)
add_executable(${test_executable_name} mutff_test.cpp)
target_link_libraries(${test_executable_name} ${PROJECT_NAME} GTest::gtest_main)
gtest_discover_tests(${test_executable_name})
add_custom_command(
    TARGET ${test_executable_name} POST_BUILD
    COMMAND ${CMAKE_CTEST_COMMAND}
)
