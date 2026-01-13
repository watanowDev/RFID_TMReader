cmake_minimum_required(VERSION 3.16)

project(RFID_TMReader LANGUAGES C CXX)

option(BUILD_C_ONLY     "Build mercuryapi only" OFF)
option(BUILD_CPP_ONLY   "Build mercuryapi_cpp only" OFF)
option(BUILD_C_TEST     "Build C test executable only" OFF)
option(BUILD_CPP_TEST   "Build C++ test executable only" ON)

# CLion에서 Build 시 install도 함께 실행할지
option(AUTO_INSTALL_AFTER_BUILD "Run 'install' target automatically on build" ON)

# --- Exactly one of four ---
set(_selected 0)
if(BUILD_C_ONLY)
    math(EXPR _selected "${_selected} + 1")
endif()
if(BUILD_CPP_ONLY)
    math(EXPR _selected "${_selected} + 1")
endif()
if(BUILD_C_TEST)
    math(EXPR _selected "${_selected} + 1")
endif()
if(BUILD_CPP_TEST)
    math(EXPR _selected "${_selected} + 1")
endif()

if(_selected EQUAL 0)
    message(FATAL_ERROR "Choose exactly one: BUILD_CPP_ONLY=ON or BUILD_C_ONLY=ON or BUILD_CPP_TEST=ON or BUILD_C_TEST=ON.")
elseif(_selected GREATER 1)
    message(FATAL_ERROR "Choose exactly one (not multiple): BUILD_CPP_ONLY / BUILD_C_ONLY / BUILD_CPP_TEST / BUILD_C_TEST.")
endif()

# --- Common Paths ---
set(RFID_ROOT
        "${CMAKE_SOURCE_DIR}"
        CACHE PATH "Root directory of the RFID_TMReader project")
set(MERCURY_C_ROOT
        "${CMAKE_SOURCE_DIR}/third_party/mercuryapi/c"
        CACHE PATH "Root directory of the MercuryAPI C core")
set(MERCURY_API_ROOT
        "${CMAKE_SOURCE_DIR}/third_party/mercuryapi/api"
        CACHE PATH "Root directory of the MercuryAPI API Wrapper")

# --- Default install prefix ---
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
            "${CMAKE_SOURCE_DIR}/install"
            CACHE PATH "Install prefix" FORCE)
endif()

# --- Build selection ---
if(BUILD_C_ONLY)
add_subdirectory(c_only)

elseif(BUILD_CPP_ONLY)
    add_subdirectory(cpp_only)

elseif(BUILD_C_TEST)
    # 기존 c_test_only 를 c_test 로 분리했다고 가정
    add_subdirectory(c_test_only)

elseif(BUILD_CPP_TEST)
    # 기존 test_only 를 cpp_test 로 분리했다고 가정
    add_subdirectory(cpp_test_only)

endif()
