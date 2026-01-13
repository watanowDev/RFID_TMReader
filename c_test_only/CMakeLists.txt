cmake_minimum_required(VERSION 3.16)

project(RFID_TMReader_C_TEST_ONLY LANGUAGES C)

# --- C99 설정 ---
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -----------------------------------------------
# RFID C 테스트 실행 파일
# -----------------------------------------------
add_executable(rfid_c_test
        main.c
)

# --- RFID 프로젝트 루트 디렉토리 입력 유무 확인 ---
if(NOT DEFINED RFID_ROOT)
    set(RFID_ROOT "${CMAKE_SOURCE_DIR}" CACHE PATH "RFID project root")
endif()

# --- install 경로 선택(Debug/Release) ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MERCURYAPI_PREFIX "${RFID_ROOT}/install-debug")
    set(MERCURYAPI_DEBUG_SUFFIX "_d")
else()
    set(MERCURYAPI_PREFIX "${RFID_ROOT}/install-release")
    set(MERCURYAPI_DEBUG_SUFFIX "")
endif()

# --- MercuryAPI C 라이브러리 불러오기 (IMPORTED) ---
add_library(mercuryapi SHARED IMPORTED)

set_target_properties(mercuryapi PROPERTIES
        IMPORTED_LOCATION "${MERCURYAPI_PREFIX}/lib/libmercuryapi${MERCURYAPI_DEBUG_SUFFIX}.so"
        INTERFACE_INCLUDE_DIRECTORIES "${MERCURYAPI_PREFIX}/include"
)

# --- (선택) C-wrapper가 별도 SO인 경우를 대비한 imported 타겟 ---
#     install/lib에 librfid_api(_d).so 같은 형태로 존재하면 자동으로 링크됨.
set(RFID_WRAPPER_SO "${MERCURYAPI_PREFIX}/lib/librfid_api${MERCURYAPI_DEBUG_SUFFIX}.so")
if(EXISTS "${RFID_WRAPPER_SO}")
    add_library(rfid_api_wrapper SHARED IMPORTED)
    set_target_properties(rfid_api_wrapper PROPERTIES
            IMPORTED_LOCATION "${RFID_WRAPPER_SO}"
            INTERFACE_INCLUDE_DIRECTORIES "${MERCURYAPI_PREFIX}/include"
    )
    target_link_libraries(rfid_c_test PRIVATE rfid_api_wrapper)
endif()

# --- 실행 파일과 라이브러리 연결 ---
target_link_libraries(rfid_c_test
        PRIVATE
        mercuryapi
)

# --- 인클루드 디렉토리 설정 ---
# install/include 에 rfid_api.h, rfid_types.h 가 있으므로 보통 이것만으로 충분
target_include_directories(rfid_c_test
        PRIVATE
        "${MERCURYAPI_PREFIX}/include"
)

# --- 런타임 라이브러리 경로 설정 (Linux 전용) ---
set_target_properties(rfid_c_test PROPERTIES
        BUILD_RPATH "${MERCURYAPI_PREFIX}/lib"
)