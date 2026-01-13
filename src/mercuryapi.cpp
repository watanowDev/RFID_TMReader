#include "mercuryapi.hpp"

// C 레이어는 구현부에서만 include (공개 헤더 노출 0)
extern "C" {
#include "rfid_api.h"
#include "rfid_types.h"
}

#include <algorithm>
#include <cstring>
#include <cctype>
#include <string_view>

#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#define MERCURYAPI_HAS_NLOHMANN_JSON 1
#else
#define MERCURYAPI_HAS_NLOHMANN_JSON 0
#endif

namespace mercuryapi {
    class Reader::Impl {
    public:

        rfid_ctx_t *ctx = nullptr;

        // Init 시 설정된 안테나 목록을 저장한다(Read 시 사용)
        std::vector<int> antennas{1, 2};

        std::vector<rfid_tag_t> cbuf; // C read 결과 버퍼(내부용)

    private:
        Result last_error = Result::Ok;
        std::string last_error_string;

    public:
        Impl() = default;
        ~Impl() {
            if (nullptr != ctx) {
                // 소멸자에서는 상세 에러를 저장할 필요가 없으므로 out 파라미터는 무시한다.
                (void) rfid_deinit(&ctx, nullptr, nullptr);
                ctx = nullptr;
            }
        }
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

        Result SetLastError_(const Result r, const char *prefix = nullptr) {
            last_error = r;
            last_error_string = std::string(ResultToString_(r));

            if (prefix != nullptr && std::strlen(prefix) > 0)
                last_error_string += ": " + std::string(prefix);

            return r;
        }

        void AppendLastErrorDetail_(const char *detail) {
            if (nullptr == detail)
                return;

            // 공백/빈 문자열은 무시
            if (detail[0] == '\0')
                return;

            // 기존 문자열 뒤에 구분자 포함하여 추가
            if (!last_error_string.empty())
                last_error_string += " | ";
            last_error_string += detail;
        }

        Result GetLastError() const {
            return last_error;
        }

        std::string GetLastError_string() const {
            return last_error_string;
        }

        int EnsureBuf_(std::size_t cap) {
            if (cap == 0)
                cap = 1;
            if (cbuf.size() != cap)
                cbuf.resize(cap);

            return static_cast<int>(cbuf.size());
        }
    };

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
                    const int v = rv.get<int>();
                    cfg.region = static_cast<Region>(v);
                } else if (rv.is_string()) {
                    std::string s = rv.get<std::string>();
                    // upper
                    for (char &c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    if (s == "AUTO") cfg.region = Region::Auto;
                    else if (s == "KR2") cfg.region = Region::KR2;
                    else if (s == "US") cfg.region = Region::US;
                    else if (s == "EU") cfg.region = Region::EU;
                    else throw std::runtime_error("invalid region string");
                } else {
                    throw std::runtime_error("invalid region type");
                }
            }

            if (j.contains("antennas"))
                cfg.antennas = j.at("antennas").get<std::vector<int>>();

            if (j.contains("plan_timeout_ms"))
                cfg.plan_timeout_ms = j.at("plan_timeout_ms").get<int>();

            if (j.contains("write_power_cdbm"))
                cfg.write_power_cdbm = j.at("write_power_cdbm").get<int>();

            if (j.contains("capacity"))
                cfg.capacity = j.at("capacity").get<std::size_t>();

            out_cfg = std::move(cfg);
            return true;
        } catch (const std::exception &e) {
            if (out_err)
                *out_err = std::string("ParseConfigJson failed: ") + e.what();
            return false;
        }
#else
        (void)json_text;
        (void)out_cfg;
        if (out_err)
            *out_err = "ParseConfigJson failed: nlohmann/json.hpp not available";
        return false;
#endif
    }

    Reader::Reader() : impl_(std::make_unique<Impl>()) {}

    Reader::~Reader() {
        Destroy();
    }

    Reader::Reader(Reader &&other) = default;
    Reader& Reader::operator=(Reader &&other) = default;

    Result Reader::Init(const Config &cfg) {
        // 기존 연결 정리 후 재-init
        Destroy();

        if (nullptr == impl_)
            return Result::InternalError;

        // 비활성화 모드
        if (false == cfg.enable)
            return impl_->SetLastError_(Result::Disabled, "Init failed");

        // Port Num 검사
        if (cfg.uri.empty())
            return impl_->SetLastError_(Result::InvalidArg
                                        , "Init failed: invalid argument (uri is empty)");

        // Antenna Array Info 검사
        if (cfg.antennas.empty())
            return impl_->SetLastError_(Result::InvalidArg
                                        , "Init failed: invalid argument (antennas is empty)");

        impl_->EnsureBuf_(cfg.capacity);

        // Read 시 사용할 안테나 목록 저장
        impl_->antennas = cfg.antennas;

        // C init params 구성 (C API 요구사항: 포인터 전달)
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

    Result Reader::Destroy() {
        if (nullptr == impl_)
            return Result::InternalError;

        // 이미 해제된 상태
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

    bool Reader::IsInitialized() const {
        return impl_ && (impl_->ctx != nullptr);
    }

    Result Reader::Read(const int read_timeout_ms, std::vector<Tag> &out_tags) {
        out_tags.clear();

        if (nullptr == impl_)
            return Result::InternalError;

        // Init 안 된 상태
        if (nullptr == impl_->ctx)
            return impl_->SetLastError_(Result::NotInitialized, "Read failed");

        // 잘못된 인자
        if (read_timeout_ms < 0)
            return impl_->SetLastError_(Result::InvalidArg, "Read failed: invalid argument (read_timeout_ms < 0)");

        // 내부 버퍼 확보
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
                , &errstr);

        const Result r = Impl::ToCppResult_(rc);
        if (Result::Ok != r) {
            impl_->SetLastError_(r, "Read failed");
            impl_->AppendLastErrorDetail_(errstr);
            return r;
        }

        // 태그 없음: Ok + empty
        if (out_count <= 0)
            return impl_->SetLastError_(Result::Ok);

        out_tags.reserve(static_cast<std::size_t>(out_count));
        for (int i = 0; i < out_count; ++i) {
            const rfid_tag_t &tags = impl_->cbuf[static_cast<std::size_t>(i)];
            Tag convTags;
            convTags.epc = tags.epc; // EPC 문자열
            convTags.rssi = tags.rssi; // RSSI
            convTags.readcnt = tags.readcnt;
            convTags.antenna = tags.antenna;
            convTags.ts = tags.ts;
            out_tags.push_back(std::move(convTags));
        }

        return impl_->SetLastError_(Result::Ok);
    }

    Result Reader::SetWritePowerCdbm(const int write_power_cdbm) {
        if (nullptr == impl_)
            return Result::InternalError;

        if (nullptr == impl_->ctx)
            return impl_->SetLastError_(Result::NotInitialized, "SetWritePower failed");

        uint32_t status = 0;
        const char *errstr = nullptr;

        // NOTE: rfid_api 업데이트에 따라 read->write로 명명 변경됨
        const RFID_RESULT rc = rfid_set_write_power(impl_->ctx, write_power_cdbm, &status, &errstr);
        const Result r = Impl::ToCppResult_(rc);
        if (Result::Ok != r) {
            impl_->SetLastError_(r, "SetWritePower failed");
            impl_->AppendLastErrorDetail_(errstr);
            return r;
        }

        return impl_->SetLastError_(Result::Ok);
    }

    Result Reader::GetLastError() const {
        if (nullptr == impl_)
            return Result::InternalError;

        return impl_->GetLastError();
    }

    std::string Reader::GetLastErrorString() const {
        if (nullptr == impl_)
            return "Internal error: impl is null";

        return impl_->GetLastError_string();
    }
} // namespace mercuryapi
