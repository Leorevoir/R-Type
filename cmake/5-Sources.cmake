#######################################

file(GLOB_RECURSE SRC_R_TYPE "src/*.cpp")
if(NOT SRC_R_TYPE)
    message(FATAL_ERROR "No source files found under src/ — check path or globs.")
endif()

#######################################

set(INCLUDE_R_TYPE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/R-Engine/include"
)

#######################################

message(STATUS "INFO: found ${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp sources")

#######################################
