include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE) # Needed to print downloading progress

message(STATUS "Downloading and extracting googletest library sources. This will take some time...")
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

message(STATUS "Downloading and extracting asio library sources. This will take some time...")
FetchContent_Declare(
    asio
    GIT_REPOSITORY git@github.com:chriskohlhoff/asio.git
    GIT_TAG master
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)

FetchContent_GetProperties(asio)

if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
endif()

add_library(asio INTERFACE)

target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
find_package(Threads)
target_link_libraries(asio INTERFACE Threads::Threads)
