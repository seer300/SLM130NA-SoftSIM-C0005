string(TIMESTAMP BUILD_TIMESTAMP)
string(TIMESTAMP TEMP_TIMESTAMP %Y%m%d-%H%M%S)
string(SUBSTRING ${TEMP_TIMESTAMP} 0 15 BUILD_BRIEF_TIMESTAMP)
message(STATUS "Timestamp is ${BUILD_TIMESTAMP}")

string(REPLACE ".build" "" TEMP_PATH ${CMAKE_SOURCE_DIR})

configure_file(
    ${CMAKE_SOURCE_DIR}/../TOOLS/cmake/timestamp.h.in
    ${CMAKE_SOURCE_DIR}/../SYSAPP/system/inc/timestamp.h
    @ONLY)
