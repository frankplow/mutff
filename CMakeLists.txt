cmake_minimum_required(VERSION 3.13)

project(MuTFF
    LANGUAGES C
    VERSION 0.1
    DESCRIPTION "A small QuickTime file format (QTFF) library"
)

option(BUILD_TESTS "Build ${PROJECT_NAME} tests" ON)
option(BUILD_COVERAGE "Build ${PROJECT_NAME} coverage report" OFF)
option(BUILD_DOCS "Build ${PROJECT_NAME} documentation" OFF)

add_library(${PROJECT_NAME}
    src/mutff.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/mutff.h)


if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_COVERAGE)
    if(CMAKE_C_COMPILER_ID STREQUAL GNU)
        target_compile_options(${PROJECT_NAME} PUBLIC --coverage)
        target_link_options(${PROJECT_NAME} PUBLIC --coverage)
    elseif(CMAKE_C_COMPILER_ID MATCHES "(Apple)?Clang")
        target_compile_options(${PROJECT_NAME} PUBLIC --coverage)
        target_link_options(${PROJECT_NAME} PUBLIC --coverage)
    else()
        message(WARNING "Unrecognised compiler. Unable to set flags for coverage generation")
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
        message(WARNING "Optimisation may cause inaccuracies in coverage reports. Build with `Debug` configuration for best results.")
    endif()

    set(object_dir ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir)
    get_property(lib_sources TARGET ${PROJECT_NAME} PROPERTY SOURCES)
    add_custom_target(coverage)
    add_custom_command(TARGET coverage PRE_BUILD
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/coverage
    )
    foreach(source ${lib_sources})
        message("source=${source}")
        add_custom_command(TARGET coverage PRE_BUILD
            COMMAND gcov ${object_dir}/${source}.gcno
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/coverage
        )
    endforeach()
endif()

if(BUILD_DOCS)
    find_package(Doxygen)
    configure_file(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    add_custom_target(docs
        ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMENT "Generating documentation"
    )
endif()
