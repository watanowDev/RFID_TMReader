#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "rfid_api.h"

#ifndef RFID_TAG_CAPACITY
#define RFID_TAG_CAPACITY (64)
#endif

static void SleepMs_(IN_ const int ms) {
    if (ms <= 0)
        return;
#if defined(_WIN32)
    Sleep((DWORD) ms);
#else
    struct timespec ts;
    ts.tv_sec = (time_t) (ms / 1000);
    ts.tv_nsec = (long) ((ms % 1000) * 1000000L);
    (void) nanosleep(&ts, NULL);
#endif
}

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

int main(int argc, char **argv) {
    /*
     * 상수 기반 설정(요청사항): JSON 로딩 없이 고정 값으로 초기화
     * 필요하면 argv[1]로 URI만 덮어쓸 수 있게 해두었습니다.
     */
    const char *uri = "tmr:///dev/ttyUSB0";
    if (argc >= 2 && argv[1] && argv[1][0] != '\0')
        uri = argv[1];

    const int antennas[] = {1, 2};
    const int antenna_count = (int) (sizeof(antennas) / sizeof(antennas[0]));

    const RFID_REGION region = RFID_REGION_AUTO;
    const int plan_timeout_ms = 300;
    const int read_power_cdbm = 3000; /* 30.00 dBm */

    const int read_timeout_ms = 500;
    const int loop = 1; /* 0: 단발, 1: 루프 */
    const int loop_interval_ms = 1000;
    const int loop_count = 30; /* loop==1이고 0보다 크면 해당 횟수만큼 반복 */

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
    RFID_RESULT ret = rfid_init(&ctx, &params, &tmr_status, &tmr_errstr);
    if (RFID_RESULT_OK != ret) {
        PrintResultWithStatus_("Init", ret, tmr_status, tmr_errstr);
        return 2;
    }
    printf("[OK] Init success (tmr_status=%u errstr=%s)\n"
           , (unsigned) tmr_status
           , (tmr_errstr ? tmr_errstr : "(null)"));

    rfid_tag_t *tags = (rfid_tag_t *) calloc((size_t) RFID_TAG_CAPACITY, sizeof(rfid_tag_t));
    if (NULL == tags) {
        printf("[ERR] OOM: tags buffer\n");
        (void) rfid_deinit(&ctx, &tmr_status, &tmr_errstr);
        return 3;
    }

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

    printf("[OK] Loop mode start interval_ms=%d count=%d\n", loop_interval_ms, loop_count);

    int iter = 0;
    while (1) {
        if (loop_count > 0 && iter >= loop_count)
            break;

        printf("---- iteration %d ----\n", iter);

        int out_count = 0;
        ret = rfid_read(ctx,
                antennas,
                antenna_count,
                read_timeout_ms,
                tags,
                RFID_TAG_CAPACITY,
                &out_count,
                &tmr_status,
                &tmr_errstr);
        if (RFID_RESULT_OK != ret) {
            PrintResultWithStatus_("Read", ret, tmr_status, tmr_errstr);
            /* 실패해도 계속 돌릴지 여부는 정책에 따라 조정 */
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
