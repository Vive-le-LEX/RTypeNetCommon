cmake_minimum_required(VERSION 3.10)
project(RTypeNetCommon
    VERSION 0.0.1
    DESCRIPTION "The RType Net Common"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)
SET(IS_TESTING FALSE CACHE BOOL "Some user-specified option")


if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif(WIN32)

if(UNIX AND NOT APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif(UNIX AND NOT APPLE)

add_subdirectory(external)

set(SRC_MAIN
    src/main.cpp
)

set(SRC
)

add_executable(${PROJECT_NAME} ${SRC} ${SRC_MAIN})

target_link_libraries(${PROJECT_NAME} PUBLIC asio)

if(IS_TESTING)
    include(CTest)

    set(TEST_PROJECT_NAME ${PROJECT_NAME}_test)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(SRC_TEST
        tests/main.cpp
    )
    message(STATUS "Building tests")
    add_executable(${TEST_PROJECT_NAME}
        ${SRC_TEST} ${SRC}
    )

    target_link_libraries(${TEST_PROJECT_NAME} PUBLIC asio)

    target_link_libraries(${TEST_PROJECT_NAME}
        PRIVATE
        GTest::gtest_main
    )

    target_include_directories(${TEST_PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
    )

    include(GoogleTest)
    gtest_discover_tests(${TEST_PROJECT_NAME})
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

add_custom_target(fclean
    COMMAND make clean
    COMMAND rm -f ${PROJECT_NAME}
    COMMAND rm -rf CMakeFiles
    COMMAND rm -f CMakeCache.txt
    COMMAND rm -f cmake_install.cmake
    COMMAND rm -f Makefile
)
