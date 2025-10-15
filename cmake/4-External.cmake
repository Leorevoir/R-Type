#######################################

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/R-Engine/CMakeLists.txt")
    add_subdirectory(external/R-Engine)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/R-Engine")
    message(WARNING "external/R-Engine directory exists but no CMakeLists.txt found. Skipping add_subdirectory.")
else()
    message(STATUS "external/R-Engine not found; skipping add_subdirectory. If r-engine is required, add it as a submodule or install a prebuilt r-engine library.")
endif()

#######################################
