cmake_minimum_required(VERSION 3.16)

project(RFID_TMReader_TEST_ONLY LANGUAGES CXX)
# --- C++17 설정 ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------
# nlohmann/json (system package)
# -----------------------------------------------
# Ubuntu:
#   sudo apt install nlohmann-json3-dev
find_package(nlohmann_json CONFIG REQUIRED)

# -----------------------------------------------
# RFID 테스트 실행 파일
# -----------------------------------------------
add_executable(rfid_cpp_test
        src/main.cpp
)

# --- RFID 프로젝트 루트 디렉토리 입력 유무 확인 ---
if(NOT DEFINED RFID_ROOT)
    set(RFID_ROOT "${CMAKE_SOURCE_DIR}" CACHE PATH "RFID project root")
endif()

# --- MercuryAPI C++ 바인딩 라이브러리 경로 설정 ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # 디버그 빌드 경로로 변경
    set(MERCURYAPI_PREFIX "${RFID_ROOT}/install-debug")
    set(MERCURYAPI_DEBUG_SUFFIX "_d")
else()
    # 릴리즈 빌드 경로로 변경
    set(MERCURYAPI_PREFIX "${RFID_ROOT}/install-release")
    set(MERCURYAPI_DEBUG_SUFFIX "")
endif()

# --- 실행 파일과 라이브러리 연결 ---
target_link_libraries(rfid_cpp_test
        PRIVATE
        mercuryapi_cpp
        nlohmann_json::nlohmann_json
)

# --- 런타임 라이브러리 경로 설정 (Linux 전용) ---
set_target_properties(rfid_cpp_test PROPERTIES
        BUILD_RPATH "${MERCURYAPI_PREFIX}/lib/rfid/mercuryapi_cpp"
)

# --- 설정 파일 복사 ---
configure_file(
        config/config.json
        ${CMAKE_CURRENT_BINARY_DIR}/config.json
        COPYONLY
)

message(STATUS "CPP_TEST_COMPLETE")
message(STATUS "RFID_ROOT: ${RFID_ROOT}")
message(STATUS "MERCURYAPI_PREFIX: ${MERCURYAPI_PREFIX}")
message(STATUS "MERCURYAPI_DEBUG_SUFFIX: ${MERCURYAPI_DEBUG_SUFFIX}")
message(STATUS "Build RPATH: ${CMAKE_CURRENT_BINARY_DIR}/lib/rfid/mercuryapi_cpp")
message(STATUS "Config file copied to: ${CMAKE_CURRENT_BINARY_DIR}/config.json")
