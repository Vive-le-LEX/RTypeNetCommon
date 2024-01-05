
include(FetchContent)

Set(FETCHCONTENT_QUIET FALSE) # Needed to print downloading progress

set(IS_TESTING_TMP ${IS_TESTING})
set(IS_BUILDING_DOC_TMP ${IS_BUILDING_DOC})

set(IS_TESTING OFF)
set(IS_BUILDING_DOC OFF)

include(external/FindAsio.cmake)
include(external/FindStduuid.cmake)
include(external/FindGlm.cmake)
include(external/FindGTest.cmake)

set(IS_TESTING ${IS_TESTING_TMP})
set(IS_BUILDING_DOC ${IS_BUILDING_DOC_TMP})
