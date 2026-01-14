# ----------------------------
# 조건부 project 호출 (단독 호출 일 경우)
# ----------------------------
if (NOT DEFINED TOP_LEVEL_BUILD)
    cmake_minimum_required(VERSION 3.16)
    project(RFID_TMReader_CPP_ONLY LANGUAGES C CXX)
endif ()

# ----------------------------
# Build type (single-config generators: Ninja/Makefiles)
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
# CPP 표준 지정 & 적용
# ----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
set(MERCURY_C_PATH "${TOP_ROOT}/third_party/mercuryapi/c/")

# ----------------------------
# MercuryAPI(C core) API 래퍼 위치
# ----------------------------
set(MERCURY_C_WRAPPER_PATH "${TOP_ROOT}/c_lib/api/")
set(MERCURY_CPP_WRAPPER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/api/")

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

# ----------------------------
# mercuryapi (완전체: C코어+rfid_api+mercuryapi 포함)
# ----------------------------
add_library(mercuryapi_cpp SHARED
        ${MERCURY_C_SOURCES}
        "${MERCURY_C_WRAPPER_PATH}/rfid_api.c"
        "${MERCURY_CPP_WRAPPER_PATH}/mercuryapi.cpp"
)

# 링커 옵션: 심볼 누락 방지
set_target_properties(mercuryapi_cpp PROPERTIES LINK_FLAGS "-Wl,--no-undefined -Wl,-z,defs")

target_include_directories(mercuryapi_cpp
        PUBLIC
        "${MERCURY_C_PATH}/include"
        "${MERCURY_C_WRAPPER_PATH}"
        "${MERCURY_CPP_WRAPPER_PATH}"
)

# ----------------------------
# 컴파일 옵션 (Debug / Release 분기)
# ----------------------------
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # <DEBUG_BUILD> - 디버그 전용 매크로 정의
    # <OSDEP_POSIX> - POSIX OS 종속 코드 사용 매크로 정의
    target_compile_definitions(mercuryapi_cpp PRIVATE DEBUG_BUILD OSDEP_POSIX)
    # <-g> - gdb 디버깅 가능
    # <-O0> - 최적화 비활성화
    # <-Wall> - 모든 경고 활성화
    # <-Wextra> - 추가 경고 활성화
    target_compile_options(mercuryapi_cpp PRIVATE -g -O0 -Wall -Wextra)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    # <OSDEP_POSIX> - POSIX OS 종속 코드 사용 매크로 정의
    target_compile_definitions(mercuryapi_cpp PRIVATE OSDEP_POSIX)
    # <-03> - 최고 수준의 최적화
    target_compile_options(mercuryapi_cpp PRIVATE -O3)
    # <-s> - 스트립 옵션 (디버그 심볼 제거)
    target_link_options(mercuryapi_cpp PRIVATE -s)
endif ()

# ----------------------------
# 출력 파일 이름 및 버전 설정
# ----------------------------
set_target_properties(mercuryapi_cpp PROPERTIES
        OUTPUT_NAME "mercuryapi_cpp"

        VERSION 1.0.0      # 실제 파일 버전
        SOVERSION 1          # SONAME (ABI 버전)

        DEBUG_POSTFIX "_d"
)

# ----------------------------
# install 출력 파일 생성
# ----------------------------
install(TARGETS mercuryapi_cpp
        LIBRARY
        DESTINATION lib/rfid/mercuryapi_cpp
        COMPONENT mercury_cpp
        NAMELINK_COMPONENT mercury_cpp

        ARCHIVE
        DESTINATION lib/rfid/mercuryapi_cpp
        COMPONENT mercury_cpp

        RUNTIME
        DESTINATION bin
        COMPONENT mercury_cpp
)

install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/api/mercuryapi.hpp"
        DESTINATION include/rfid/mercuryapi_cpp
        COMPONENT mercury_cpp
)

add_custom_command(TARGET mercuryapi_cpp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E echo "POST_BUILD fired (mercuryapi_cpp)"
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config $<CONFIG> --component mercury_cpp
        VERBATIM
)

# ----------------------------
# 상태 메시지 출력
# ----------------------------
message(STATUS "========================================")
message(STATUS "RFID TMReader C++ ONLY Library Build Info")
message(STATUS "========================================")
message(STATUS "Target            : mercuryapi_cpp only")
message(STATUS "RFID root         : ${TOP_ROOT}")
message(STATUS "C++ header        : ${CMAKE_CURRENT_SOURCE_DIR}/api/mercuryapi.hpp")
message(STATUS "C++ source        : ${CMAKE_CURRENT_SOURCE_DIR}/api/mercuryapi.cpp")
message(STATUS "Build type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "Output names      : mercuryapi_cpp.so / mercuryapi_cpp_d.so")
message(STATUS "========================================")
