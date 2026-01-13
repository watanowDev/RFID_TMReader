#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mercuryapi {
    /**
     * @brief C++ wrapper에서 사용하는 Result 코드
     * @note C 레이어 결과를 래핑한 값입니다.
     */
    enum class Result {
        Ok = 0, Disabled, InvalidArg, NotInitialized, ConnectFail, RegionFail, PlanFail, ReadFail, InternalError
    };

    /**
     * @brief Region 설정(오탈자 방지를 위해 enum 사용)
     */
    enum class Region {
        Auto = 0, KR2, US, EU
    };

    /**
     * @brief RFID 태그 결과 모델
     */
    struct Tag {
        std::string epc; ///< @brief RFID tag 값(EPC 문자열)
        int rssi = 0; ///< @brief 수신 강도(RSSI)
        std::uint32_t readcnt = 0; ///< @brief read count
        int antenna = 0; ///< @brief 태그가 읽힌 안테나 번호
        std::uint64_t ts = 0; ///< @brief timestamp(ms)
    };

    /**
     * @brief 초기화 파라미터 모델
     */
    struct Config {
        ///< @brief 0이면 RFID 기능 비활성
        bool enable = true;
        ///< @brief 예: "tmr:///dev/ttyUSB0"
        std::string uri;
        ///< @brief region 설정(Auto 포함)
        Region region = Region::Auto;
        ///< @brief 사용할 안테나 목록(예: {1,2})
        std::vector<int> antennas{1, 2};
        ///< @brief read plan timeout(ms), 필요 시 0 허용
        int plan_timeout_ms = 0;

        ///< @brief Write power(cdBm). 0 이하면 "기본값 사용"(설정하지 않음)
        int write_power_cdbm = 0;

        ///< @brief 내부 read 버퍼 용량(태그 최대 수)
        std::size_t capacity = 64;
    };

    /**
     * @brief JSON 문자열에서 Config를 파싱한다.
     *
     * @note
     * - 헤더에서 JSON 타입을 노출하지 않기 위해 문자열 기반 API로 제공한다.
     * - nlohmann/json.hpp 가 빌드에 포함되어 있어야 구현이 활성화된다.
     *
     * @param[in] json_text JSON 문자열
     * @param[out] out_cfg 파싱 결과
     * @param[out] out_err 실패 시 에러 메시지(NULL 허용)
     * @return 성공 시 true
     */
    bool ParseConfigJson(const std::string &json_text, Config &out_cfg, std::string *out_err = nullptr);

    /**
     * @brief RFID 예외
     */
    class RfidException : public std::runtime_error {
    public:
        /**
         * @brief 예외 생성
         * @param r Result 코드
         * @param msg 에러 메시지
         */
        explicit RfidException(Result r, const std::string &msg) : std::runtime_error(msg), result_(r) {}

        /**
         * @brief 실패 원인 코드
         */
        Result result() const noexcept {
            return result_;
        }

    private:
        Result result_;
    };

    /**
     * @brief C++ RFID Wrapper (Pimpl)
     *
     * @note
     * - 생성자는 init을 수행하지 않습니다(사용자가 Init 시점 제어).
     * - Destroy를 호출하지 않아도 소멸자에서 자동 해제(RAII).
     * - 실패 시 throw + GetLastError 스타일 병행 지원.
     */
    class Reader {
    public:
        /**
         * @brief 생성자 (연결/초기화 수행하지 않음)
         */
        Reader();

        /**
         * @brief 소멸자 (항상 Destroy 수행, throw 하지 않음)
         */
        ~Reader();

        Reader(const Reader &) = delete;
        Reader& operator=(const Reader &) = delete;

        Reader(Reader &&);
        Reader& operator=(Reader &&);

        /**
         * @brief 초기화/연결
         * @param cfg 설정 값
         * @return 결과 코드
         */
        Result Init(const Config &cfg);

        /**
         * @brief 해제 (예외를 던지지 않음)
         * @return 결과 코드
         */
        Result Destroy();

        /**
         * @brief 초기화 완료 여부
         */
        bool IsInitialized() const;

        /**
         * @brief 태그 읽기
         * @param read_timeout_ms read timeout(ms)
         * @param[out] out_tags 결과 태그 리스트(태그 없으면 empty)
         * @return 결과 코드
         */
        Result Read(const int read_timeout_ms, std::vector<Tag> &out_tags);

        /**
         * @brief Write power(cdBm)를 변경한다.
         *
         * @param write_power_cdbm write power(cdBm). 0 이하면 "기본값 사용"(설정하지 않음)
         * @return 결과 코드
         */
        Result SetWritePowerCdbm(const int write_power_cdbm);

        /**
         * @brief 마지막 에러 코드(GetLastError 스타일)
         */
        Result GetLastError() const;

        /**
         * @brief 마지막 에러 문자열(GetLastErrorString 스타일)
         */
        std::string GetLastErrorString() const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace mercuryapi
