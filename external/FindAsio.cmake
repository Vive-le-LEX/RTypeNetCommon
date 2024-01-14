find_library(ASIO_LIB asio)

if(NOT ASIO_LIB)
    message(STATUS "Asio library not found, downloading from github")
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio
        GIT_TAG master
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
    )

    FetchContent_GetProperties(asio)

    if(NOT asio_POPULATED)
        FetchContent_Populate(asio)
    endif()

    add_library(asio INTERFACE)

    set(asio_SOURCE_DIR "${asio_SOURCE_DIR}" CACHE PATH "Path to ASIO source directory" FORCE)

    find_package(Threads)
    target_link_libraries(asio INTERFACE Threads::Threads)
else()
    message(STATUS "Asio library found at ${ASIO_LIB}")
endif()
