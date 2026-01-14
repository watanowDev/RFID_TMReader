/**
 * @file main.c
 * @brief RFID 읽기 예제 프로그램
 *
 * C 및 C++ 래퍼 라이브러리를 이용하여 RFID 태그를 읽고 출력하는 예제입니다.
 * 단발 읽기와 루프 모드를 지원하며, 설치된 장치와 포트를 기준으로 URI를 설정합니다.
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>  /**< POSIX용 usleep/nanosleep */

#include "rfid_api.h"

#ifndef RFID_TAG_CAPACITY
#define RFID_TAG_CAPACITY (64) /**< 태그 버퍼 최대 개수 */
#endif

/**
 * @brief 밀리초 단위로 현재 스레드를 대기
 * @param[in] ms 대기할 시간 (밀리초 단위). 0 이하이면 즉시 반환.
 */
static void SleepMs_(IN_ const int ms) {
    if (ms <= 0)
        return;

    struct timespec ts;
    ts.tv_sec = (time_t) (ms / 1000);
    ts.tv_nsec = (long) ((ms % 1000) * 1000000L);
    (void) nanosleep(&ts, NULL);
}

/**
 * @brief 읽은 RFID 태그 정보를 출력
 *
 * 태그가 없으면 "no tags" 메시지 출력.
 *
 * @param[in] tags 읽은 태그 배열 포인터
 * @param[in] count 태그 개수
 */
static void PrintTags_(IN_ const rfid_tag_t *tags, IN_ const int count) {
    if ((NULL == tags) || (count <= 0)) {
        printf("[OK] Read: no tags\n");
        return;
    }

    printf("[OK] Read: %d tag(s)\n", count);
    for (int i = 0; i < count; ++i) {
        const rfid_tag_t *t = &tags[i];
        printf("  [%d] ant=%d rssi=%d readcnt=%u ts=%llu epc=%s\n"
               , i
               , t->antenna
               , t->rssi
               , (unsigned) t->readcnt
               , (unsigned long long) t->ts
               , t->epc);
    }
}

/**
 * @brief RFID API 호출 실패 시 상태 메시지 출력
 *
 * @param[in] op 수행한 연산 이름 (예: "Init", "Read")
 * @param[in] ret RFID_RESULT 반환값
 * @param[in] st tmr_status 값
 * @param[in] errstr 장치 오류 문자열 (NULL 가능)
 */
static void PrintResultWithStatus_(IN_ const char *op
                                   , IN_ const RFID_RESULT ret
                                   , IN_ const uint32_t st
                                   , IN_ const char *errstr) {
    printf("[ERR] %s failed: %s (tmr_status=%u errstr=%s)\n"
           , op
           , rfid_result_to_string(ret)
           , (unsigned) st
           , (errstr ? errstr : "(null)"));
}

/**
 * @brief 프로그램 진입점
 *
 * @details
 * URI, 안테나, 지역, 읽기 시간 등을 상수 기반으로 초기화하고 RFID 장치를 초기화합니다.
 * 단발 또는 루프 모드로 태그를 읽고 결과를 출력합니다.
 *
 * @param[in] argc 명령행 인자 개수
 * @param[in] argv 명령행 인자 배열 (argv[1]로 URI 덮어쓰기 가능)
 * @return int 종료 코드
 * - 0: 성공
 * - 2: RFID 초기화 실패
 * - 3: 태그 버퍼 할당 실패
 * - 4: 단발 읽기 실패
 */
int main(int argc, char **argv) {
    /**< URI 설정 (기본값: /dev/ttyUSB0, argv[1]로 덮어쓰기 가능) */
    const char *uri = "tmr:///dev/ttyUSB0";
    if (argc >= 2 && argv[1] && argv[1][0] != '\0')
        uri = argv[1];

    /**< 사용 안테나 설정 */
    const int antennas[] = {1, 2};
    const int antenna_count = (int) (sizeof(antennas) / sizeof(antennas[0]));

    /**< 지역, 읽기 시간, 전력 설정 */
    const RFID_REGION region = RFID_REGION_AUTO;
    const int plan_timeout_ms = 300;
    const int read_power_cdbm = 3000; /* 30.00 dBm */

    const int read_timeout_ms = 500;
    const int loop = 1; /**< 0: 단발, 1: 루프 */
    const int loop_interval_ms = 1000;
    const int loop_count = 30; /**< 루프 반복 횟수 */

    /**< RFID 초기화 구조체 설정 */
    rfid_init_params_t params;
    memset(&params, 0, sizeof(params));
    params.rfid_enable = 1;
    params.uri = uri;
    params.region = region;
    params.antennas = antennas;
    params.antenna_count = antenna_count;
    params.plan_timeout_ms = plan_timeout_ms;
    params.write_power_cdbm = read_power_cdbm;

    printf("[INFO] uri=%s region=%s antennas=%d\n", uri, rfid_region_to_string(region), antenna_count);

    uint32_t tmr_status = 0;
    const char *tmr_errstr = NULL;
    rfid_ctx_t *ctx = NULL;

    /**< RFID 장치 초기화 */
    RFID_RESULT ret = rfid_init(&ctx, &params, &tmr_status, &tmr_errstr);
    if (RFID_RESULT_OK != ret) {
        PrintResultWithStatus_("Init", ret, tmr_status, tmr_errstr);
        return 2;
    }
    printf("[OK] Init success (tmr_status=%u errstr=%s)\n"
           , (unsigned) tmr_status
           , (tmr_errstr ? tmr_errstr : "(null)"));

    /**< 태그 버퍼 할당 */
    rfid_tag_t *tags = (rfid_tag_t *) calloc((size_t) RFID_TAG_CAPACITY, sizeof(rfid_tag_t));
    if (NULL == tags) {
        printf("[ERR] OOM: tags buffer\n");
        (void) rfid_deinit(&ctx, &tmr_status, &tmr_errstr);
        return 3;
    }

    /**< 단발 읽기 모드 */
    if (!loop) {
        int out_count = 0;
        ret = rfid_read(ctx
                        , antennas
                        , antenna_count
                        , read_timeout_ms
                        , tags
                        , RFID_TAG_CAPACITY
                        , &out_count
                        , &tmr_status
                        , &tmr_errstr);
        if (RFID_RESULT_OK != ret) {
            PrintResultWithStatus_("Read", ret, tmr_status, tmr_errstr);
        }
        else {
            printf("[OK] Read success (tmr_status=%u errstr=%s)\n"
                   , (unsigned) tmr_status
                   , (tmr_errstr ? tmr_errstr : "(null)"));
            PrintTags_(tags, out_count);
        }

        free(tags);
        (void) rfid_deinit(&ctx, &tmr_status, &tmr_errstr);
        return (RFID_RESULT_OK == ret) ? 0 : 4;
    }

    /**< 루프 읽기 모드 */
    printf("[OK] Loop mode start interval_ms=%d count=%d\n", loop_interval_ms, loop_count);

    int iter = 0;
    while (1) {
        if (loop_count > 0 && iter >= loop_count)
            break;

        printf("---- iteration %d ----\n", iter);

        int out_count = 0;
        ret = rfid_read(ctx
                        , antennas
                        , antenna_count
                        , read_timeout_ms
                        , tags
                        , RFID_TAG_CAPACITY
                        , &out_count
                        , &tmr_status
                        , &tmr_errstr);
        if (RFID_RESULT_OK != ret) {
            PrintResultWithStatus_("Read", ret, tmr_status, tmr_errstr);
        }
        else {
            printf("[OK] Read success (tmr_status=%u errstr=%s)\n"
                   , (unsigned) tmr_status
                   , (tmr_errstr ? tmr_errstr : "(null)"));
            PrintTags_(tags, out_count);
        }

        ++iter;
        SleepMs_(loop_interval_ms);
    }

    free(tags);
    (void) rfid_deinit(&ctx, &tmr_status, &tmr_errstr);
    printf("[OK] Done\n");
    return 0;
}
