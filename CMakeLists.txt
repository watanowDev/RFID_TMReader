cmake_minimum_required(VERSION 3.16)
project(RFID_TMReader
        VERSION 1.0
        DESCRIPTION "RFID TM Reader Library"
        LANGUAGES NONE)

# --- Build Options ---
option(BUILD_C_LIB "Build mercuryapi C core & test logic" ON)
option(BUILD_CPP_LIB "Build mercuryapi_cpp & test logic" ON)
option(BUILD_C_TEST "Build C test logic" ON)
option(BUILD_CPP_TEST "Build C++ test logic" ON)
option(TOP_LEVEL_BUILD "Indicates if this is the top-level build" ON)

# --- Top Level Project Root Directory ---
set(TOP_ROOT "${CMAKE_SOURCE_DIR}" CACHE PATH "Top level project root directory")

if(BUILD_C_LIB OR BUILD_CPP_LIB)
    enable_language(C)
endif()
if(BUILD_CPP_LIB)
    enable_language(CXX)
endif()

# --- Libraries (products) ---
if(BUILD_C_LIB)
    add_subdirectory(c_lib)
endif ()
if(BUILD_CPP_LIB)
    add_subdirectory(cpp_lib)
endif ()

# --- Test Executables ---
if(BUILD_C_TEST)
    add_subdirectory(c_test)
endif ()
if(BUILD_CPP_TEST)
    add_subdirectory(cpp_test)
endif ()

message(STATUS "-----------------------------------")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build C Library: ${BUILD_C_LIB}")
message(STATUS "Build C++ Library: ${BUILD_CPP_LIB}")
message(STATUS "Build C Tests: ${BUILD_C_TEST}")
message(STATUS "Build C++ Tests: ${BUILD_CPP_TEST}")
message(STATUS "Top Level Build: ${TOP_LEVEL_BUILD}")
message(STATUS "Top Level Root: ${TOP_ROOT}")
message(STATUS "-----------------------------------")
