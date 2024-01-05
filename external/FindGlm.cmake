find_library(GLM_LIB glm)

if(NOT GLM_LIB)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
    )

    FetchContent_MakeAvailable(glm)

else()
    message(STATUS "Stduuid library found at ${GLM_LIB}")
endif()
