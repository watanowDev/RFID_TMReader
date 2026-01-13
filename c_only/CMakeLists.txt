cmake_minimum_required(VERSION 3.16)

project(RFID_TMReader_C_ONLY LANGUAGES C)

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
# Standard
# ----------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================
# Position Independent Code
# ============================================================
# shared library(.so)는 반드시 PIC가 필요
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

set(RFID_C_WRAPPER_SOURCES
        "${MERCURY_API_ROOT}/rfid_api.c"
)

# ----------------------------
# mercuryapi (완전체: C코어+rfid_api 포함)
# ----------------------------
add_library(mercuryapi SHARED
        ${MERCURY_C_SOURCES}
        ${RFID_C_WRAPPER_SOURCES}
)

# 링커 옵션: 심볼 누락 방지
target_link_options(mercuryapi PRIVATE
        "LINKER:--no-undefined"
        "LINKER:-z,defs"
)

target_include_directories(mercuryapi
        PUBLIC
        "${MERCURY_API_ROOT}"
        "${MERCURY_C_ROOT}/include"
)

target_compile_definitions(mercuryapi
        PRIVATE
        OSDEP_POSIX
)

set_target_properties(mercuryapi PROPERTIES
        OUTPUT_NAME "mercuryapi"
        DEBUG_POSTFIX "_d"
)

# ----------------------------
# Install
# <prefix>/include, <prefix>/lib, <prefix>/bin
# ----------------------------
install(TARGETS mercuryapi
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

install(FILES
        "${MERCURY_API_ROOT}/rfid_api.h"
        "${MERCURY_API_ROOT}/rfid_types.h"
        DESTINATION include
)

# ----------------------------
# Summary
# ----------------------------
message(STATUS "Target            : mercuryapi only")
message(STATUS "Build type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "Output names      : libmercuryapi.so / libmercuryapi_d.so")
