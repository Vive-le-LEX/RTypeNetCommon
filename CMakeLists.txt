cmake_minimum_required(VERSION 3.10)
project(rtype-netcommon
    VERSION 0.0.1
    DESCRIPTION "The RType Net Common"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)
SET(IS_TESTING FALSE CACHE BOOL "Some user-specified option")


include(external/FindDependencies.cmake)

add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME} INTERFACE asio stduuid)

target_include_directories(${PROJECT_NAME} INTERFACE ./includes)
