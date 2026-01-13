cmake_minimum_required(VERSION 3.16)

project(RFID_TMReader_CPP_ONLY LANGUAGES C CXX)

# ----------------------------
# Build type (single-config generators: Ninja/Makefiles)
# ----------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" )
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug
        Release
        RelWithDebInfo
        MinSizeRel
)

string(TOLOWER "${CMAKE_BUILD_TYPE}" _cfg_dir)

# ----------------------------
# Standards
# ----------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Position Independent Code
# ============================================================
# shared library(.so)는 반드시 PIC가 필요
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# C++ wrapper 위치(현재 구조)
set(CPP_INCLUDE_DIR   "${RFID_ROOT}/include")
set(CPP_SRC_DIR       "${RFID_ROOT}/src")

# ----------------------------
# MercuryAPI(C core) sources (Linux/POSIX)
# ----------------------------
set(MERCURY_C_SOURCES
        "${MERCURY_C_ROOT}/src/hex_bytes.c"
        "${MERCURY_C_ROOT}/src/osdep_posix.c"
        "${MERCURY_C_ROOT}/src/serial_reader.c"
        "${MERCURY_C_ROOT}/src/serial_reader_l3.c"
        "${MERCURY_C_ROOT}/src/serial_transport_posix.c"
        "${MERCURY_C_ROOT}/src/serial_transport_tcp_posix.c"
        "${MERCURY_C_ROOT}/src/tm_reader.c"
        "${MERCURY_C_ROOT}/src/tm_reader_async.c"
        "${MERCURY_C_ROOT}/src/tmr_param.c"
        "${MERCURY_C_ROOT}/src/tmr_strerror.c"
        "${MERCURY_C_ROOT}/src/tmr_utils.c"
)

# C 래퍼(rfid_api.c)
set(RFID_C_WRAPPER_SOURCES
        "${MERCURY_API_ROOT}/rfid_api.c"
)

# C++ wrapper(현재 파일명)
set(MERCURY_CPP_SOURCES
        "${CPP_SRC_DIR}/mercuryapi.cpp"
)

# ============================================================
# 라이브러리 타겟 생성
# ============================================================
# ----------------------------
# mercuryapi_cpp (완전체: C코어+rfid_api+cpp wrapper 포함)
# - mercuryapi.so 의존성 없음
# ----------------------------
add_library(mercuryapi_cpp SHARED
        ${MERCURY_C_SOURCES}
        ${RFID_C_WRAPPER_SOURCES}
        ${MERCURY_CPP_SOURCES}
)

# 링커 옵션: 심볼 누락 방지
target_link_options(mercuryapi_cpp PRIVATE
        "LINKER:--no-undefined"
        "LINKER:-z,defs"
)

# ============================================================
# 헤더 공개 범위 설정
# ============================================================
# PUBLIC:
# - 이 라이브러리를 사용하는 실행 파일에서도 include 가능
# - 배포 시 include/ 디렉터리만 외부 공개


target_include_directories(mercuryapi_cpp
        PUBLIC
        "${CPP_INCLUDE_DIR}"       # <-- include/mercuryapi.hpp 가 있는 곳
        "${MERCURY_API_ROOT}"      # rfid_api.h 등 쓰면 필요
        "${MERCURY_C_ROOT}/include"
)


# ============================================================
# 컴파일 옵션 (Debug / Release 분기)
# ============================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # --------------------------------------------------------
    # Debug 빌드
    # --------------------------------------------------------
    # - gdb 디버깅 가능
    # - 최적화 비활성화
    # - 디버그 전용 매크로 정의
    target_compile_definitions(mercuryapi_cpp
            PRIVATE DEBUG_BUILD
    )

    target_compile_options(mercuryapi_cpp
            PRIVATE
            -g # 디버그 심볼
            -O0 # 최적화 끔
            -Wall # 경고 활성화
            -Wextra
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    # --------------------------------------------------------
    # Release 빌드 (배포용)
    # --------------------------------------------------------
    # - 성능 최적화
    # - 디버그 심볼 제거


    target_compile_options(mercuryapi_cpp
            PRIVATE
            -O3 # 최대 최적화
    )


    # 링크 단계에서 strip 수행 (심볼 제거)
    target_link_options(mercuryapi_cpp
            PRIVATE
            -s
    )
endif()

# ============================================================
# 출력 파일 이름 제어
# ============================================================
# mercuryapi_cpp.so 형태로 생성되도록 명시
set_target_properties(mercuryapi_cpp PROPERTIES
        OUTPUT_NAME "mercuryapi_cpp"
        DEBUG_POSTFIX "_d"
)

# ============================================================
# install 출력 파일 생성
# ============================================================
install(TARGETS mercuryapi_cpp
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# ============================================================
# 상태 메시지 출력
# ============================================================
message(STATUS "========================================")
message(STATUS "RFID TMReader C++ ONLY Library Build Info")
message(STATUS "========================================")
message(STATUS "Target            : mercuryapi_cpp only")
message(STATUS "RFID root         : ${RFID_ROOT}")
message(STATUS "C++ header        : ${CPP_INCLUDE_DIR}/mercuryapi.hpp")
message(STATUS "C++ source        : ${CPP_SRC_DIR}/mercuryapi.cpp")
message(STATUS "Build type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "Install cfg dir   : ${_cfg_dir}")
message(STATUS "Output names      : mercuryapi_cpp.so / mercuryapi_cpp_d.so")
message(STATUS "========================================")

