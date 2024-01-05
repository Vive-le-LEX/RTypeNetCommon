find_library(STDUUID_LIB stduuid)

if(NOT STDUUID_LIB)
    FetchContent_Declare(
        stduuid
        GIT_REPOSITORY git@github.com:mariusbancila/stduuid.git
        GIT_TAG master
    )

    FetchContent_MakeAvailable(stduuid)

else()
    message(STATUS "Stduuid library found at ${STDUUID_LIB}")
endif()
