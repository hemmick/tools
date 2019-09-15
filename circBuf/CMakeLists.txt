cmake_minimum_required(VERSION 3.0.0)
project(test VERSION 0.1.0)

get_filename_component(ProjectId ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId} C CXX)

include(CTest)
enable_testing()

list(INSERT SOURCES 0
"test.c"
"circular_buffer.c"
)

add_executable(${ProjectId}.out ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
