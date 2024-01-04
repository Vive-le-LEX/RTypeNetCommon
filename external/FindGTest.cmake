find_library(GTEST_LIB gtest)

if(IS_TESTING)
    if(NOT GTEST_LIB)
        message(STATUS "Testing mode enabled => Fetching GTest")
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
            DOWNLOAD_EXTRACT_TIMESTAMP true
        )

        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    else()
        message(STATUS "Testing mode enabled => GTest found")
    endif()
endif()
