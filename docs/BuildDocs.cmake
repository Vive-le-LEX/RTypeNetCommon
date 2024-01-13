find_package(Doxygen)
if (DOXYGEN_FOUND)
    FetchContent_Declare(
        doxygen-awesome-css
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG        main
        SOURCE_DIR ""
        BINARY_DIR ""
    )
    FetchContent_GetProperties(doxygen-awesome-css)
    if(NOT doxygen-awesome-css_POPULATED)
        FetchContent_Populate(doxygen-awesome-css)
    endif()

    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_AWESOME_CSS_DIR ${doxygen-awesome-css_SOURCE_DIR})
    file(GLOB ALL_MARKDOWNS ${CMAKE_CURRENT_SOURCE_DIR}/docs/*.md)
    string(REPLACE ";" " " ALL_MARKDOWNS "${ALL_MARKDOWNS}")

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)

    add_custom_target( doc_doxygen ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/docs/html/.nojekyll
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
