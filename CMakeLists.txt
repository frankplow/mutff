cmake_minimum_required(VERSION 3.13)

project(MuTFF
    LANGUAGES C
    VERSION 0.1
    DESCRIPTION "A small QuickTime file format (QTFF) library"
)

string(TOLOWER ${PROJECT_NAME} library_name)
string(TOUPPER ${PROJECT_NAME} project_name_uppercase)

option(${project_name_uppercase}_BUILD_TESTS "Build ${PROJECT_NAME} tests" ON)
option(${project_name_uppercase}_BUILD_COVERAGE "Build ${PROJECT_NAME} coverage report" OFF)
option(${project_name_uppercase}_BUILD_DOCS "Build ${PROJECT_NAME} documentation" OFF)

add_library(${library_name}
    src/mutff.c
)

target_include_directories(${library_name} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(${library_name} PROPERTIES
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/mutff.h)

if(CMAKE_C_COMPILER_ID STREQUAL GNU)
    target_compile_options(${library_name} PRIVATE
        -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter)
elseif(CMAKE_C_COMPILER_ID MATCHES "(Apple)?Clang")
    target_compile_options(${library_name} PRIVATE
        -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

if(${project_name_uppercase}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(${project_name_uppercase}_BUILD_COVERAGE)
    if(CMAKE_C_COMPILER_ID STREQUAL GNU)
        target_compile_options(${library_name} PUBLIC --coverage)
        target_link_options(${library_name} PUBLIC --coverage)
    elseif(CMAKE_C_COMPILER_ID MATCHES "(Apple)?Clang")
        target_compile_options(${library_name} PUBLIC --coverage)
        target_link_options(${library_name} PUBLIC --coverage)
    else()
        message(WARNING "Unrecognised compiler. Unable to set flags for coverage generation")
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
        message(WARNING "Optimisation may cause inaccuracies in coverage reports. Build with `Debug` configuration for best results.")
    endif()

    set(object_dir ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${library_name}.dir)
    set(coverage_dir ${CMAKE_CURRENT_BINARY_DIR}/coverage)
    get_property(lib_sources TARGET ${library_name} PROPERTY SOURCES)
    add_custom_target(coverage)
    add_custom_command(TARGET coverage PRE_BUILD
        COMMAND mkdir -p ${coverage_dir}
    )
    foreach(source ${lib_sources})
        add_custom_command(TARGET coverage PRE_BUILD
            COMMAND cp ${object_dir}/${source}.gcno ${coverage_dir}
            COMMAND cp ${object_dir}/${source}.gcda ${coverage_dir}
        )
    endforeach()
endif()

if(${project_name_uppercase}_BUILD_DOCS)
    find_package(Doxygen)
    configure_file(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMENT "Generating documentation"
    )
endif()
