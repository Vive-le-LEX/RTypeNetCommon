find_library(STDUUID_LIB stduuid)

if(NOT STDUUID_LIB)
    FetchContent_Declare(
        stduuid
        GIT_REPOSITORY git@github.com:mariusbancila/stduuid.git
        GIT_TAG master
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
    )

    FetchContent_GetProperties(stduuid)

    if(NOT stduuid_POPULATED)
        FetchContent_Populate(stduuid)
    endif()

    add_library(stduuid INTERFACE)

    target_include_directories(${PROJECT_NAME} INTERFACE ${stduuid_SOURCE_DIR}/include)
    target_include_directories(${PROJECT_NAME} INTERFACE ${stduuid_SOURCE_DIR}/gsl)
else()
    message(STATUS "Stduuid library found at ${STDUUID_LIB}")
endif()
