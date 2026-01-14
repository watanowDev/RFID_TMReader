/**
 * @file main.cpp
 * @brief MercuryAPI C++ 예제 실행 파일
 *
 * JSON 설정을 로드하고 RFID 리더를 초기화 및 태그 읽기 수행.
 * 단발/루프 모드 모두 지원.
 */

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "mercuryapi.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief 문자열을 기반으로 Region 열거형 변환
 * @param[in] s 문자열 ("Auto", "KR2", "US", "EU")
 * @return 대응되는 mercuryapi::Region (기본 Auto)
 */
static mercuryapi::Region ParseRegionOrAuto(const std::string &s) {
    if (s == "Auto" || s == "AUTO" || s == "auto")
        return mercuryapi::Region::Auto;
    if (s == "KR2" || s == "kr2")
        return mercuryapi::Region::KR2;
    if (s == "US" || s == "us")
        return mercuryapi::Region::US;
    if (s == "EU" || s == "eu")
        return mercuryapi::Region::EU;
    return mercuryapi::Region::Auto;
}

/**
 * @brief JSON 파일 로드
 * @param[in] path 파일 경로
 * @return 읽어온 nlohmann::json 객체
 * @throws std::runtime_error 파일을 열 수 없는 경우
 */
static json LoadJsonFile(const std::string &path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open json file: " + path);
    }

    json j;
    ifs >> j;
    return j;
}

/**
 * @brief JSON 객체에서 안테나 목록 추출
 * @param[in] j JSON 객체
 * @return 안테나 번호 벡터 (없으면 기본 1,2)
 */
static std::vector<int> GetAntennas(const json &j) {
    std::vector<int> ants;

    if (j.contains("antennas") && j["antennas"].is_array()) {
        for (const auto &v: j["antennas"]) {
            ants.push_back(v.get<int>());
        }
    }

    if (ants.empty()) {
        ants.push_back(1);
        ants.push_back(2);
    }
    return ants;
}

/**
 * @brief JSON 객체를 기반으로 Config 구조체 생성
 * @param[in] j JSON 객체
 * @return mercuryapi::Config 구조체
 */
static mercuryapi::Config BuildConfig(const json &j) {
    mercuryapi::Config cfg;

    cfg.enable = j.value("enable", true);
    cfg.uri = j.value("uri", std::string());
    cfg.region = ParseRegionOrAuto(j.value("region", std::string("Auto")));
    cfg.antennas = GetAntennas(j);
    cfg.plan_timeout_ms = j.value("plan_timeout_ms", 300);
    cfg.write_power_cdbm = j.value("write_power_cdbm", 0);
    cfg.capacity = static_cast<std::size_t>(j.value("capacity", 64));

    return cfg;
}

/**
 * @brief 읽은 태그 출력
 * @param[in] tags 읽은 태그 벡터
 */
static void PrintTags(const std::vector<mercuryapi::Tag> &tags) {
    if (tags.empty()) {
        std::cout << "[OK] Read: no tags\n";
        return;
    }

    std::cout << "[OK] Read: " << tags.size() << " tag(s)\n";
    for (std::size_t i = 0; i < tags.size(); ++i) {
        const auto &t = tags[i];
        std::cout << "  [" << i << "]"
                << " ant=" << t.antenna
                << " rssi=" << t.rssi
                << " readcnt=" << t.readcnt
                << " ts=" << t.ts
                << " epc=" << t.epc
                << "\n";
    }
}

/**
 * @brief main 함수
 * @param[in] argc 인자 개수
 * @param[in] argv 인자 배열
 * @return 0: 성공, 2: Init 실패, 3: 예외, 4: Read 실패
 */
int main(int argc, char **argv) {
    // 기본 config.json 경로
    const std::string json_path = (argc >= 2) ? argv[1] : std::string("config.json");

    try {
        const json j = LoadJsonFile(json_path);

        const int read_timeout_ms = j.value("read_timeout_ms", 650);
        const bool loop = j.value("loop", false);
        const int loop_interval_ms = j.value("loop_interval_ms", 750);
        const int loop_count = j.value("loop_count", 10);

        mercuryapi::Reader reader;
        const mercuryapi::Config cfg = BuildConfig(j);

        const mercuryapi::Result init_r = reader.Init(cfg);
        if (init_r != mercuryapi::Result::Ok) {
            std::cerr << "[ERR] Init failed (" << reader.GetLastErrorString() << ")\n";
            return 2;
        }
        std::cout << "[OK] Init success\n";

        /**
         * @brief 단발 읽기 수행 람다
         * @return Read 결과 Result
         */
        auto do_read_once = [&]() -> mercuryapi::Result {
            std::vector<mercuryapi::Tag> tags;
            tags.reserve(cfg.capacity);

            const mercuryapi::Result rr = reader.Read(read_timeout_ms, tags);
            if (rr != mercuryapi::Result::Ok) {
                std::cerr << "[ERR] Read failed (" << reader.GetLastErrorString() << ")\n";
                return rr;
            }

            PrintTags(tags);
            return mercuryapi::Result::Ok;
        };

        if (!loop) {
            // 단발 모드
            const mercuryapi::Result rr = do_read_once();
            const mercuryapi::Result dr = reader.Destroy();
            if (dr != mercuryapi::Result::Ok) {
                std::cerr << "[WARN] Destroy failed (" << reader.GetLastErrorString() << ")\n";
            }
            return (rr == mercuryapi::Result::Ok) ? 0 : 4;
        }

        // 루프 모드
        std::cout << "[OK] Loop mode start"
                << " interval_ms=" << loop_interval_ms
                << " count=" << loop_count << "\n";

        int iter = 0;
        while (true) {
            if (loop_count > 0 && iter >= loop_count)
                break;

            std::cout << "---- iteration " << iter << " ----\n";

            const mercuryapi::Result rr = do_read_once();
            if (rr != mercuryapi::Result::Ok)
                continue;

            ++iter;
            if (loop_interval_ms > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(loop_interval_ms));
        }

        const mercuryapi::Result dr = reader.Destroy();
        if (dr != mercuryapi::Result::Ok) {
            std::cerr << "[WARN] Destroy failed (" << reader.GetLastErrorString() << ")\n";
        }

        std::cout << "[OK] Done\n";
        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << "[ERR] std::exception: " << e.what() << "\n";
        return 3;
    }
}
