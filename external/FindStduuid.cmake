find_library(STDUUID_LIB stduuid)

if(NOT STDUUID_LIB)
    FetchContent_Declare(
        stduuid
        GIT_REPOSITORY https://github.com/mariusbancila/stduuid
        GIT_TAG master
    )

    FetchContent_MakeAvailable(stduuid)

else()
    message(STATUS "Stduuid library found at ${STDUUID_LIB}")
endif()
