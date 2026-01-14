/**
 * @file mercuryapi.cpp
 * @brief MercuryAPI C++ 래퍼 구현부
 *
 * ThingMagic MercuryAPI C API를 기반으로 하는 C++ 래퍼 구현.
 * 내부적으로 C API를 호출하며, 상태 관리는 Result 열거형으로 수행.
 */

#include "mercuryapi.hpp"

extern "C" {
#include "rfid_api.h"
#include "rfid_types.h"
}

#include <algorithm>
#include <cstring>
#include <cctype>
#include <string_view>
#include <nlohmann/json.hpp>

namespace mercuryapi {
    /**
     * @brief Reader 클래스 내부 구현체 (PImpl 패턴)
     */
    class Reader::Impl {
    public:
        rfid_ctx_t *ctx = nullptr; /**< C API context 포인터 */

        std::vector<int> antennas{1, 2}; /**< Init 시 설정된 안테나 목록 (Read 시 사용) */

        std::vector<rfid_tag_t> cbuf; /**< C read 결과 버퍼 (내부용) */

    private:
        Result last_error = Result::Ok; /**< 마지막 오류 상태 */
        std::string last_error_string; /**< 마지막 오류 문자열 */

    public:
        Impl() = default;

        /**
         * @brief 소멸자
         *
         * 연결된 C API context가 존재하면 해제.
         */
        ~Impl() {
            if (nullptr != ctx) {
                (void) rfid_deinit(&ctx, nullptr, nullptr);
                ctx = nullptr;
            }
        }

        /**
         * @brief Result를 문자열로 변환
         * @param[in] r 변환할 Result
         * @return 변환된 문자열
         */
        static constexpr std::string_view ResultToString_(Result r) noexcept {
            using namespace std::literals;
            switch (r) {
                case Result::Ok:
                    return "Ok"sv;
                case Result::Disabled:
                    return "Disabled"sv;
                case Result::InvalidArg:
                    return "InvalidArg"sv;
                case Result::NotInitialized:
                    return "NotInitialized"sv;
                case Result::ConnectFail:
                    return "ConnectFail"sv;
                case Result::RegionFail:
                    return "RegionFail"sv;
                case Result::PlanFail:
                    return "PlanFail"sv;
                case Result::ReadFail:
                    return "ReadFail"sv;
                case Result::InternalError:
                    return "InternalError"sv;
            }
            return "UnknownResult"sv;
        }

        /**
         * @brief C API 결과를 C++ Result로 변환
         * @param[in] r C API RFID_RESULT 값
         * @return 대응되는 C++ Result
         */
        static Result ToCppResult_(const RFID_RESULT r) noexcept {
            switch (r) {
                case RFID_RESULT_OK:
                    return Result::Ok;
                case RFID_RESULT_DISABLED:
                    return Result::Disabled;
                case RFID_RESULT_INVALID_ARG:
                    return Result::InvalidArg;
                case RFID_RESULT_NOT_INITIALIZED:
                    return Result::NotInitialized;
                case RFID_RESULT_CONNECT_FAIL:
                    return Result::ConnectFail;
                case RFID_RESULT_REGION_FAIL:
                    return Result::RegionFail;
                case RFID_RESULT_PLAN_FAIL:
                    return Result::PlanFail;
                case RFID_RESULT_READ_FAIL:
                    return Result::ReadFail;
                case RFID_RESULT_INTERNAL_ERROR:
                    return Result::InternalError;
                default:
                    return Result::InternalError;
            }
        }

        /**
         * @brief C++ Result를 C API 결과로 변환
         * @param[in] r C++ Result
         * @return 대응되는 C API RFID_RESULT
         */
        static RFID_RESULT ToCResult_(const Result r) noexcept {
            switch (r) {
                case Result::Ok:
                    return RFID_RESULT_OK;
                case Result::Disabled:
                    return RFID_RESULT_DISABLED;
                case Result::InvalidArg:
                    return RFID_RESULT_INVALID_ARG;
                case Result::NotInitialized:
                    return RFID_RESULT_NOT_INITIALIZED;
                case Result::ConnectFail:
                    return RFID_RESULT_CONNECT_FAIL;
                case Result::RegionFail:
                    return RFID_RESULT_REGION_FAIL;
                case Result::PlanFail:
                    return RFID_RESULT_PLAN_FAIL;
                case Result::ReadFail:
                    return RFID_RESULT_READ_FAIL;
                case Result::InternalError:
                    return RFID_RESULT_INTERNAL_ERROR;
                default:
                    return RFID_RESULT_INTERNAL_ERROR;
            }
        }

        /**
         * @brief C++ Region을 C API Region으로 변환
         * @param[in] r C++ Region 값
         * @return 대응되는 C API RFID_REGION 값
         */
        static RFID_REGION ToCRegion_(const Region r) noexcept {
            switch (r) {
                case Region::Auto:
                    return RFID_REGION_AUTO;
                case Region::KR2:
                    return RFID_REGION_KR2;
                case Region::US:
                    return RFID_REGION_US;
                case Region::EU:
                    return RFID_REGION_EU;
                default:
                    return RFID_REGION_AUTO;
            }
        }

        /**
         * @brief 마지막 오류 상태 설정
         * @param[in] r Result 값
         * @param[in] prefix 오류 메시지 접두사 (선택)
         * @return 설정된 Result
         */
        Result SetLastError_(const Result r, const char *prefix = nullptr) {
            last_error = r;
            last_error_string = std::string(ResultToString_(r));
            if (prefix != nullptr && std::strlen(prefix) > 0)
                last_error_string += ": " + std::string(prefix);
            return r;
        }

        /**
         * @brief 마지막 오류 문자열 뒤에 상세 정보 추가
         * @param[in] detail 추가할 문자열
         */
        void AppendLastErrorDetail_(const char *detail) {
            if (nullptr == detail || detail[0] == '\0')
                return;
            if (!last_error_string.empty())
                last_error_string += " | ";
            last_error_string += detail;
        }

        /**
         * @brief 마지막 오류 반환
         * @return Result
         */
        Result GetLastError() const {
            return last_error;
        }

        /**
         * @brief 마지막 오류 문자열 반환
         * @return 오류 문자열
         */
        std::string GetLastError_string() const {
            return last_error_string;
        }

        /**
         * @brief 내부 C read 버퍼 확보
         * @param[in] cap 필요한 버퍼 용량
         * @return 확보된 버퍼 크기
         */
        int EnsureBuf_(std::size_t cap) {
            if (cap == 0)
                cap = 1;
            if (cbuf.size() != cap)
                cbuf.resize(cap);
            return static_cast<int>(cbuf.size());
        }
    };

    /**
     * @brief JSON 문자열을 Config 구조체로 파싱
     * @param[in] json_text JSON 문자열
     * @param[out] out_cfg 파싱된 Config
     * @param[out] out_err 오류 메시지 (NULL 가능)
     * @return true: 성공, false: 실패
     */
    bool ParseConfigJson(const std::string &json_text, Config &out_cfg, std::string *out_err) {
#if MERCURYAPI_HAS_NLOHMANN_JSON
        try {
            using json = nlohmann::json;
            const json j = json::parse(json_text);

            Config cfg;

            if (j.contains("enable"))
                cfg.enable = j.at("enable").get<bool>();
            if (j.contains("uri"))
                cfg.uri = j.at("uri").get<std::string>();
            if (j.contains("region")) {
                const auto &rv = j.at("region");
                if (rv.is_number_integer()) {
                    cfg.region = static_cast<Region>(rv.get<int>());
                }
                else if (rv.is_string()) {
                    std::string s = rv.get<std::string>();
                    for (char &c: s)
                        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    if (s == "AUTO")
                        cfg.region = Region::Auto;
                    else if (s == "KR2")
                        cfg.region = Region::KR2;
                    else if (s == "US")
                        cfg.region = Region::US;
                    else if (s == "EU")
                        cfg.region = Region::EU;
                    else
                        throw std::runtime_error("invalid region string");
                }
                else {
                    throw std::runtime_error("invalid region type");
                }
            }

            if (j.contains("antennas"))
                cfg.antennas = j.at("antennas").get<std::vector<int> >();
            if (j.contains("plan_timeout_ms"))
                cfg.plan_timeout_ms = j.at("plan_timeout_ms").get<int>();
            if (j.contains("write_power_cdbm"))
                cfg.write_power_cdbm = j.at("write_power_cdbm").get<int>();
            if (j.contains("capacity"))
                cfg.capacity = j.at("capacity").get<std::size_t>();

            out_cfg = std::move(cfg);
            return true;
        }
        catch (const std::exception &e) {
            if (out_err)
                *out_err = std::string("ParseConfigJson failed: ") + e.what();
            return false;
        }
#else
        (void) json_text;
        (void) out_cfg;
        if (out_err)
            *out_err = "ParseConfigJson failed: nlohmann/json.hpp not available";
        return false;
#endif
    }

    // Reader 생성자/소멸자/Move
    Reader::Reader() : impl_(std::make_unique<Impl>()) {}

    Reader::~Reader() {
        Destroy();
    }

    Reader::Reader(Reader &&other) = default;
    Reader& Reader::operator=(Reader &&other) = default;

    /**
     * @brief RFID 장치 초기화
     * @param[in] cfg 초기화 설정
     * @return 초기화 결과 Result
     */
    Result Reader::Init(const Config &cfg) {
        Destroy();
        if (nullptr == impl_)
            return Result::InternalError;
        if (!cfg.enable)
            return impl_->SetLastError_(Result::Disabled, "Init failed");
        if (cfg.uri.empty())
            return impl_->SetLastError_(Result::InvalidArg, "Init failed: invalid argument (uri is empty)");
        if (cfg.antennas.empty())
            return impl_->SetLastError_(Result::InvalidArg, "Init failed: invalid argument (antennas is empty)");

        impl_->EnsureBuf_(cfg.capacity);
        impl_->antennas = cfg.antennas;

        rfid_init_params_t params{};
        params.rfid_enable = 1;
        params.uri = cfg.uri.c_str();
        params.region = Impl::ToCRegion_(cfg.region);
        params.antennas = cfg.antennas.data();
        params.antenna_count = static_cast<int>(cfg.antennas.size());
        params.plan_timeout_ms = cfg.plan_timeout_ms;
        params.write_power_cdbm = cfg.write_power_cdbm;

        rfid_ctx_t *tmp = nullptr;
        uint32_t status = 0;
        const char *errstr = nullptr;
        const RFID_RESULT rc = rfid_init(&tmp, &params, &status, &errstr);
        const Result r = Impl::ToCppResult_(rc);

        if (Result::Ok != r) {
            if (nullptr != tmp)
                (void) rfid_deinit(&tmp, nullptr, nullptr);
            impl_->SetLastError_(r, "Init failed");
            impl_->AppendLastErrorDetail_(errstr);
            return r;
        }

        impl_->ctx = tmp;
        return impl_->SetLastError_(Result::Ok);
    }

    /**
     * @brief RFID 장치 해제
     * @return 해제 결과 Result
     */
    Result Reader::Destroy() {
        if (nullptr == impl_)
            return Result::InternalError;
        if (nullptr == impl_->ctx)
            return impl_->SetLastError_(Result::Ok);

        uint32_t status = 0;
        const char *errstr = nullptr;
        const RFID_RESULT rc = rfid_deinit(&impl_->ctx, &status, &errstr);
        const Result r = Impl::ToCppResult_(rc);

        if (Result::Ok != r) {
            impl_->SetLastError_(r);
            impl_->AppendLastErrorDetail_(errstr);
            return r;
        }

        impl_->ctx = nullptr;
        return impl_->SetLastError_(Result::Ok);
    }

    /**
     * @brief 초기화 여부 확인
     * @return true: 초기화됨, false: 초기화 안됨
     */
    bool Reader::IsInitialized() const {
        return impl_ && (impl_->ctx != nullptr);
    }

    /**
     * @brief RFID 태그 읽기
     * @param[in] read_timeout_ms 읽기 타임아웃(ms)
     * @param[out] out_tags 읽은 태그를 저장할 vector
     * @return 읽기 결과 Result
     */
    Result Reader::Read(const int read_timeout_ms, std::vector<Tag> &out_tags) {
        out_tags.clear();
        if (nullptr == impl_)
            return Result::InternalError;
        if (nullptr == impl_->ctx)
            return impl_->SetLastError_(Result::NotInitialized, "Read failed");
        if (read_timeout_ms < 0)
            return impl_->SetLastError_(Result::InvalidArg, "Read failed: invalid argument (read_timeout_ms < 0)");

        if (impl_->cbuf.empty())
            impl_->EnsureBuf_(64);

        int out_count = 0;
        uint32_t status = 0;
        const char *errstr = nullptr;
        const RFID_RESULT rc = rfid_read(
                impl_->ctx
                , impl_->antennas.data()
                , static_cast<int>(impl_->antennas.size())
                , read_timeout_ms
                , impl_->cbuf.data()
                , static_cast<int>(impl_->cbuf.size())
                , &out_count
                , &status
                , &errstr
                );

        const Result r = Impl::ToCppResult_(rc);
        if (Result::Ok != r) {
            impl_->SetLastError_(r, "Read failed");
            impl_->AppendLastErrorDetail_(errstr);
            return r;
        }

        if (out_count <= 0)
            return impl_->SetLastError_(Result::Ok);

        out_tags.reserve(static_cast<std::size_t>(out_count));
        for (int i = 0; i < out_count; ++i) {
            const rfid_tag_t &tags = impl_->cbuf[static_cast<std::size_t>(i)];
            Tag convTags;
            convTags.epc = tags.epc;
            convTags.rssi = tags.rssi;
            convTags.readcnt = tags.readcnt;
            convTags.antenna = tags.antenna;
            convTags.ts = tags.ts;
            out_tags.push_back(std::move(convTags));
        }

        return impl_->SetLastError_(Result::Ok);
    }

    /**
     * @brief 쓰기 전력 설정
     * @param[in] write_power_cdbm 쓰기 전력 (cdbm)
     * @return 설정 결과 Result
     */
    Result Reader::SetWritePowerCdbm(const int write_power_cdbm) {
        if (nullptr == impl_)
            return Result::InternalError;
        if (nullptr == impl_->ctx)
            return impl_->SetLastError_(Result::NotInitialized, "SetWritePower failed");

        uint32_t status = 0;
        const char *errstr = nullptr;
        const RFID_RESULT rc = rfid_set_write_power(impl_->ctx, write_power_cdbm, &status, &errstr);
        const Result r = Impl::ToCppResult_(rc);

        if (Result::Ok != r) {
            impl_->SetLastError_(r, "SetWritePower failed");
            impl_->AppendLastErrorDetail_(errstr);
            return r;
        }

        return impl_->SetLastError_(Result::Ok);
    }

    /**
     * @brief 마지막 오류 상태 반환
     * @return 마지막 오류 Result
     */
    Result Reader::GetLastError() const {
        if (nullptr == impl_)
            return Result::InternalError;
        return impl_->GetLastError();
    }

    /**
     * @brief 마지막 오류 문자열 반환
     * @return 오류 문자열
     */
    std::string Reader::GetLastErrorString() const {
        if (nullptr == impl_)
            return "Internal error: impl is null";
        return impl_->GetLastError_string();
    }
} // namespace mercuryapi
