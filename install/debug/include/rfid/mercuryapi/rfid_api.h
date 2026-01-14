#ifndef RFID_API_H_
#define RFID_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rfid_types.h"

/**
 * @brief RFID 컨텍스트(Reader 핸들 포함). 구현부에서 정의하는 opaque 타입.
 */
typedef struct rfid_ctx rfid_ctx_t;

/**
 * @brief RFID_RESULT enum을 사람이 읽을 수 있는 문자열로 변환한다.
 * @param result RFID_RESULT 값
 * @return 결과 문자열(정적 문자열). NULL을 반환하지 않는다.
 */
const char* rfid_result_to_string(IN_ const RFID_RESULT result);

/**
 * @brief RFID_REGION enum을 사람이 읽을 수 있는 문자열로 변환한다.
 * @param region RFID_REGION 값
 * @return region 문자열(정적 문자열). NULL을 반환하지 않는다.
 */
const char* rfid_region_to_string(IN_ const RFID_REGION region);

/**
 * @brief RFID 초기화 및 리더 연결/설정을 수행한다.
 *
 * - params->rfid_enable == 0 인 경우: RFID_RESULT_DISABLED 반환
 * - 성공 시: *out_ctx 는 유효한 컨텍스트를 가리킨다.
 * - SDK 외부 오류(인자 오류 등)는 out_status = -1 로 설정된다.
 *
 * @param[out] out_ctx 생성된 컨텍스트(out). 성공 시 non-NULL.
 * @param[in]  params 초기화 파라미터(in). NULL이면 RFID_RESULT_INVALID_ARG.
 * @param[out] out_status TMR 상태 코드(out). NULL 허용. SDK 외부 오류 시 -1.
 * @param[out] out_errstr 상태 문자열(out). NULL 허용. TMR_ErrorCodeToString() 반환 문자열.
 *
 * @return RFID_RESULT 결과 코드
 */
RFID_RESULT rfid_init(OUT_ rfid_ctx_t **out_ctx
                      , IN_ const rfid_init_params_t *params
                      , OUT_ uint32_t *out_status
                      , OUT_ const char **out_errstr);

/**
 * @brief RFID 리더 리소스를 해제한다.
 *
 * - 성공 시 *inout_ctx 를 NULL로 설정한다.
 * - NULL 인자를 넣어도 안전하도록 구현한다.
 * - SDK 외부 오류(인자 오류 등)는 out_status = -1 로 설정된다.
 *
 * @param[in,out] inout_ctx 해제할 컨텍스트(in/out)
 * @param[out] out_status TMR 상태 코드(out). NULL 허용. SDK 외부 오류 시 -1.
 * @param[out] out_errstr 상태 문자열(out). NULL 허용. TMR_ErrorCodeToString() 반환 문자열.
 *
 * @return RFID_RESULT 결과 코드
 */
RFID_RESULT rfid_deinit(INOUT_ rfid_ctx_t **inout_ctx, OUT_ uint32_t *out_status, OUT_ const char **out_errstr);

/**
 * @brief Write Power(centi-dBm)를 변경한다.
 *
 * - read_power_cdbm <= 0 이면 "기본값 사용"으로 간주하며, 장치 파라미터를 변경하지 않고 성공을 반환한다.
 * - ctx가 NULL이거나 초기화되지 않은 경우, RFID_RESULT_INVALID_ARG / RFID_RESULT_NOT_INITIALIZED 를 반환한다.
 *
 * @param[in]  ctx RFID 컨텍스트(in)
 * @param[in]  read_power_cdbm 읽기 전력(centi-dBm). 예: 3000 == 30.00 dBm
 * @param[out] out_status TMR 상태 코드(out). NULL 허용. SDK 외부 오류 시 -1.
 * @param[out] out_errstr 상태 문자열(out). NULL 허용.
 *
 * @return RFID_RESULT 결과 코드
 */
RFID_RESULT rfid_set_write_power(IN_ rfid_ctx_t *ctx, IN_ int read_power_cdbm, OUT_ uint32_t *out_status, OUT_ const char **out_errstr);


/**
 * @brief 단발성 RFID 태그 읽기. 내부에서 RSSI desc, ReadCount desc로 정렬해 반환한다.
 *
 * - 태그가 없으면 RFID_RESULT_OK 를 반환하며 *out_count = 0 이 된다.
 * - out_tags는 호출자가 제공하는 버퍼이며, tag_capacity 만큼 채울 수 있다.
 * - SDK 외부 오류(인자 오류, 미초기화 등)는 out_status = -1 로 설정된다.
 *
 * @param[in]  ctx RFID 컨텍스트(in). NULL이면 RFID_RESULT_INVALID_ARG.
 * @param[in]  read_timeout_ms read 타임아웃(ms)
 * @param[out] out_tags 결과 태그 배열(out). NULL이면 RFID_RESULT_INVALID_ARG.
 * @param[in]  tag_capacity out_tags의 최대 원소 수(in). 0 이하이면 RFID_RESULT_INVALID_ARG.
 * @param[out] out_count 실제 반환된 태그 개수(out). NULL이면 RFID_RESULT_INVALID_ARG.
 * @param[out] out_status TMR 상태 코드(out). NULL 허용. SDK 외부 오류 시 -1.
 * @param[out] out_errstr 상태 문자열(out). NULL 허용. TMR_ErrorCodeToString() 반환 문자열.
 *
 * @return RFID_RESULT 결과 코드
 */
    RFID_RESULT rfid_read(IN_ rfid_ctx_t *ctx
                      , IN_ const int *antennas
                      , IN_ const int antenna_count
                      , IN_ const int read_timeout_ms
                      , OUT_ rfid_tag_t *out_tags
                      , IN_ const int tag_capacity
                      , OUT_ int *out_count
                      , OUT_ uint32_t *out_status
                      , OUT_ const char **out_errstr);

#ifdef __cplusplus
}
#endif

#endif  // RFID_API_H_
