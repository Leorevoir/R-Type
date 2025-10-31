########################################

set(R_TYPE_TARGET_NAME r-type)
if(TARGET ${R_TYPE_TARGET_NAME})
    set(R_TYPE_TARGET_NAME r-type-app)
    message(WARNING "Target 'r-type' already exists. Using '${R_TYPE_TARGET_NAME}' as the executable target name to avoid collision.")
endif()

########################################

add_executable(${R_TYPE_TARGET_NAME} ${SRC_R_TYPE})
target_include_directories(${R_TYPE_TARGET_NAME} PRIVATE ${INCLUDE_R_TYPE})

########################################

if(TARGET r-engine)
    target_link_libraries(${R_TYPE_TARGET_NAME} PRIVATE r-engine)
else()
    find_library(R_ENGINE_LIB NAMES r-engine r_engine 
        PATHS "${CMAKE_CURRENT_SOURCE_DIR}/lib" "${CMAKE_CURRENT_BINARY_DIR}/lib" 
        NO_DEFAULT_PATH)
    if(R_ENGINE_LIB)
        target_link_libraries(${R_TYPE_TARGET_NAME} PRIVATE ${R_ENGINE_LIB})
        message(STATUS "Linked ${R_TYPE_TARGET_NAME} against found library: ${R_ENGINE_LIB}")
    else()
        message(WARNING "r-engine target/library not found. If you expected external/R-Engine to be built, ensure it's present as a submodule or installed. Build may fail if r-engine symbols are required.")
    endif()
endif()

#######################################

apply_compiler_warnings(${R_TYPE_TARGET_NAME})
apply_linker_optimizations(${R_TYPE_TARGET_NAME})

#######################################
