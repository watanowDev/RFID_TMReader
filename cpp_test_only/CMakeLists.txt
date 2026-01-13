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
add_executable(rfid_test
        main.cpp
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


# --- MercuryAPI C++ 바인딩 라이브러리 불러오기 ---
add_library(mercuryapi_cpp SHARED IMPORTED)

# --- MercuryAPI C++ 바인딩 라이브러리 속성 설정 ---
set_target_properties(mercuryapi_cpp PROPERTIES
        IMPORTED_LOCATION "${MERCURYAPI_PREFIX}/lib/libmercuryapi_cpp${MERCURYAPI_DEBUG_SUFFIX}.so"
        INTERFACE_INCLUDE_DIRECTORIES "${MERCURYAPI_PREFIX}/include"
)

# --- 실행 파일과 라이브러리 연결 ---
target_link_libraries(rfid_test
        PRIVATE
        mercuryapi_cpp
        nlohmann_json::nlohmann_json
)

# --- 인클루드 디렉토리 설정 ---
target_include_directories(rfid_test
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# --- 런타임 라이브러리 경로 설정 (Linux 전용) ---
set_target_properties(rfid_test PROPERTIES
        BUILD_RPATH "${MERCURYAPI_PREFIX}/lib"
)

# --- 설정 파일 복사 ---
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config.json
        ${CMAKE_CURRENT_BINARY_DIR}/config.json
        COPYONLY
)