cmake_minimum_required(VERSION 3.16)
project(RFID_TMReader_C_TEST_ONLY LANGUAGES C)

# ----------------------------
# Build type (single-config generators: Ninja/Makefiles)
# ----------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# --- C99 설정 ---
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -----------------------------------------------
# RFID C 테스트 실행 파일
# -----------------------------------------------
add_executable(rfid_c_test
        src/main.c
)

# --- RFID 프로젝트 루트 디렉토리 입력 유무 확인 ---
if(NOT DEFINED TOP_ROOT)
    set(TOP_ROOT "${CMAKE_SOURCE_DIR}" CACHE PATH "RFID project root")
endif()

# --- install 경로 선택(Debug/Release) ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MERCURYAPI_PREFIX "${TOP_ROOT}/install/debug")
    set(MERCURYAPI_DEBUG_SUFFIX "_d")
else()
    set(MERCURYAPI_PREFIX "${TOP_ROOT}/install/release")
    set(MERCURYAPI_DEBUG_SUFFIX "")
endif()

# --- 실행 파일과 라이브러리 연결 ---
target_link_libraries(rfid_c_test PRIVATE mercuryapi)

# --- 런타임 라이브러리 경로 설정 (Linux 전용) ---
set_target_properties(rfid_c_test PROPERTIES
        BUILD_RPATH "${MERCURYAPI_PREFIX}/lib"
)

message(STATUS "-----------------------------------")
message(STATUS "C_TEST_COMPLETE")
message(STATUS "TOP_ROOT: ${TOP_ROOT}")
message(STATUS "MERCURYAPI_PREFIX: ${MERCURYAPI_PREFIX}")
message(STATUS "MERCURYAPI_DEBUG_SUFFIX: ${MERCURYAPI_DEBUG_SUFFIX}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "RPATH: ${CMAKE_BUILD_RPATH}")
message(STATUS "Executable: rfid_c_test")
message(STATUS "-----------------------------------")