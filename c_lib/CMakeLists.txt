# ----------------------------
# 조건부 project 호출 (단독 호출 일 경우)
# ----------------------------
if (NOT DEFINED TOP_LEVEL_BUILD)
    cmake_minimum_required(VERSION 3.16)
    project(RFID_TMReader_C_ONLY LANGUAGES C)
endif ()

# ----------------------------
# Build type 미정의 시 정의 주입 및 처리 가능한 build type 제한
# ----------------------------
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif ()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug
        Release
        RelWithDebInfo
        MinSizeRel
)

# ----------------------------
# C 표준 지정 & 적용
# ----------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ----------------------------
# Position Independent Code (shared library(.so)는 반드시 PIC가 필요)
# ----------------------------
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------------------------
# MercuryAPI(C core) 위치
# ----------------------------
if (NOT DEFINED TOP_LEVEL_BUILD)
    set(TOP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()
set(MERCURY_C_PATH "${TOP_ROOT}/third_party/mercuryapi/c")

# ----------------------------
# MercuryAPI(C core) API 래퍼 위치
# ----------------------------
set(MERCURY_API_PATH "${CMAKE_CURRENT_SOURCE_DIR}/api")

# ----------------------------
# MercuryAPI(C core) sources (Linux/POSIX)
# ----------------------------
set(MERCURY_C_SOURCES
        "${MERCURY_C_PATH}/src/hex_bytes.c"
        "${MERCURY_C_PATH}/src/osdep_posix.c"
        "${MERCURY_C_PATH}/src/serial_reader.c"
        "${MERCURY_C_PATH}/src/serial_reader_l3.c"
        "${MERCURY_C_PATH}/src/serial_transport_posix.c"
        "${MERCURY_C_PATH}/src/serial_transport_tcp_posix.c"
        "${MERCURY_C_PATH}/src/tm_reader.c"
        "${MERCURY_C_PATH}/src/tm_reader_async.c"
        "${MERCURY_C_PATH}/src/tmr_param.c"
        "${MERCURY_C_PATH}/src/tmr_strerror.c"
        "${MERCURY_C_PATH}/src/tmr_utils.c"
)

set(RFID_C_WRAPPER_SOURCES
        "${MERCURY_API_PATH}/rfid_api.c"
)

# ----------------------------
# mercuryapi (완전체: C코어+rfid_api 포함)
# ----------------------------
add_library(mercuryapi SHARED
        ${MERCURY_C_SOURCES}
        ${RFID_C_WRAPPER_SOURCES}
)

# 링커 옵션: 심볼 누락 방지
set_target_properties(mercuryapi PROPERTIES LINK_FLAGS "-Wl,--no-undefined -Wl,-z,defs")

target_include_directories(mercuryapi
        PUBLIC
        "${MERCURY_API_PATH}"
        "${MERCURY_C_PATH}/include"
)


# ----------------------------
# 컴파일 옵션 (Debug / Release 분기)
# ----------------------------
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # <DEBUG_BUILD> - 디버그 전용 매크로 정의
    # <OSDEP_POSIX> - POSIX OS 종속 코드 사용 매크로 정의
    target_compile_definitions(mercuryapi PRIVATE DEBUG_BUILD OSDEP_POSIX)
    # <-g> - gdb 디버깅 가능
    # <-O0> - 최적화 비활성화
    # <-Wall> - 모든 경고 활성화
    # <-Wextra> - 추가 경고 활성화
    target_compile_options(mercuryapi PRIVATE -g -O0 -Wall -Wextra)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    # <OSDEP_POSIX> - POSIX OS 종속 코드 사용 매크로 정의
    target_compile_definitions(mercuryapi PRIVATE OSDEP_POSIX)
    # <-03> - 최고 수준의 최적화
    target_compile_options(mercuryapi PRIVATE -O3)
    # <-s> - 스트립 옵션 (디버그 심볼 제거)
    target_link_options(mercuryapi PRIVATE -s)
endif ()

# ----------------------------
# 출력 파일 이름 및 버전 설정
# ----------------------------
set_target_properties(mercuryapi PROPERTIES
        OUTPUT_NAME "mercuryapi"

        VERSION 1.0.0      # 실제 파일 버전
        SOVERSION 1          # SONAME (ABI 버전)

        DEBUG_POSTFIX "_d"
)

# ----------------------------
# install 출력 파일 생성
# ----------------------------
install(TARGETS mercuryapi
        LIBRARY
        DESTINATION lib/rfid/mercuryapi
        COMPONENT mercury_c
        NAMELINK_COMPONENT mercury_c

        ARCHIVE
        DESTINATION lib/rfid/mercuryapi
        COMPONENT mercury_c

        RUNTIME
        DESTINATION bin
        COMPONENT mercury_c
)

install(FILES
        "${MERCURY_API_PATH}/rfid_api.h"
        "${MERCURY_API_PATH}/rfid_types.h"
        DESTINATION include/rfid/mercuryapi
        COMPONENT mercury_c
)

add_custom_command(TARGET mercuryapi POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E echo "POST_BUILD fired (mercuryapi)"
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config $<CONFIG> --component mercury_c
        VERBATIM
)

# ----------------------------
# Summary
# ----------------------------
message(STATUS "-----------------------------------")
message(STATUS "Target            : mercuryapi (C SDK wrapper)")
message(STATUS "Build type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "Output names      : libmercuryapi.so / libmercuryapi_d.so")
message(STATUS "-----------------------------------")
