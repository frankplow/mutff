cmake_minimum_required(VERSION 3.14)

include(FetchContent)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

enable_language(CXX)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)
include(CTest)

configure_file(test.mov ${CMAKE_CURRENT_BINARY_DIR}/test.mov COPYONLY)

set(test_executable_name ${library_name}_test)
add_executable(${test_executable_name} mutff_test.cpp)
target_link_libraries(${test_executable_name} ${library_name} GTest::gtest_main)
gtest_discover_tests(${test_executable_name})
