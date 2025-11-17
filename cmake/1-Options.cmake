#######################################

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

#######################################

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

#######################################

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#######################################

message(STATUS "INFO: C++ standard set to ${CMAKE_CXX_STANDARD}")

#######################################

option(ENABLE_DEBUG "Enable R_TYPE_DEBUG macro" OFF)
if(ENABLE_DEBUG)
    add_compile_definitions(R_TYPE_DEBUG=1)
    message(STATUS "INFO: R_TYPE_DEBUG enabled")
endif()

#######################################

option(ENABLE_TESTS "Enable building tests" OFF)

#######################################

if (MSVC)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _USE_MATH_DEFINES)
    message(STATUS "INFO: MSVC compile definitions added: WIN32_LEAN_AND_MEAN NOMINMAX _USE_MATH_DEFINES")
endif()
