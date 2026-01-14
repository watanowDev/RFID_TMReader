#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "mercuryapi.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static mercuryapi::Region ParseRegionOrAuto(const std::string& s)
{
  if (s == "Auto" || s == "AUTO" || s == "auto") return mercuryapi::Region::Auto;
  if (s == "KR2" || s == "kr2") return mercuryapi::Region::KR2;
  if (s == "US" || s == "us") return mercuryapi::Region::US;
  if (s == "EU" || s == "eu") return mercuryapi::Region::EU;
  return mercuryapi::Region::Auto;
}

static json LoadJsonFile(const std::string& path)
{
  std::ifstream ifs(path);
  if (!ifs.is_open())
  {
    throw std::runtime_error("Failed to open json file: " + path);
  }

  json j;
  ifs >> j;
  return j;
}

static std::vector<int> GetAntennas(const json& j)
{
  std::vector<int> ants;

  if (j.contains("antennas") && j["antennas"].is_array())
  {
    for (const auto& v : j["antennas"])
    {
      ants.push_back(v.get<int>());
    }
  }

  if (ants.empty())
  {
    ants.push_back(1);
    ants.push_back(2);
  }
  return ants;
}

static mercuryapi::Config BuildConfig(const json& j)
{
  mercuryapi::Config cfg;

  cfg.enable = j.value("enable", true);
  cfg.uri = j.value("uri", std::string());

  cfg.region = ParseRegionOrAuto(j.value("region", std::string("Auto")));
  cfg.antennas = GetAntennas(j);

  cfg.plan_timeout_ms = j.value("plan_timeout_ms", 300);
  // Write power(cdBm). 0 이하면 "기본값 사용"(C 레이어에서 설정 생략) 정책.
  cfg.write_power_cdbm = j.value("write_power_cdbm", 0);
  cfg.capacity = static_cast<std::size_t>(j.value("capacity", 64));

  return cfg;
}

static void PrintTags(const std::vector<mercuryapi::Tag>& tags)
{
  if (tags.empty())
  {
    std::cout << "[OK] Read: no tags\n";
    return;
  }

  std::cout << "[OK] Read: " << tags.size() << " tag(s)\n";
  for (std::size_t i = 0; i < tags.size(); ++i)
  {
    const auto& t = tags[i];
    std::cout << "  [" << i << "]"
              << " ant=" << t.antenna
              << " rssi=" << t.rssi
              << " readcnt=" << t.readcnt
              << " ts=" << t.ts
              << " epc=" << t.epc
              << "\n";
  }
}

int main(int argc, char** argv)
{
  // 기본: 같은 디렉터리의 config.json
  const std::string json_path = (argc >= 2) ? argv[1] : std::string("config.json");

  try
  {
    const json j = LoadJsonFile(json_path);

    const int read_timeout_ms = j.value("read_timeout_ms", 650);

    const bool loop = j.value("loop", false);
    const int loop_interval_ms = j.value("loop_interval_ms", 750);
    const int loop_count = j.value("loop_count", 10);

    mercuryapi::Reader reader;
    const mercuryapi::Config cfg = BuildConfig(j);

    const mercuryapi::Result init_r = reader.Init(cfg);
    if (init_r != mercuryapi::Result::Ok)
    {
      std::cerr << "[ERR] Init failed (" << reader.GetLastErrorString() << ")\n";
      return 2;
    }
    std::cout << "[OK] Init success\n";

    auto do_read_once = [&]() -> mercuryapi::Result {
      std::vector<mercuryapi::Tag> tags;
      tags.reserve(cfg.capacity);

      const mercuryapi::Result rr = reader.Read(read_timeout_ms, tags);
      if (rr != mercuryapi::Result::Ok)
      {
        std::cerr << "[ERR] Read failed (" << reader.GetLastErrorString() << ")\n";
        return rr;
      }

      PrintTags(tags);
      return mercuryapi::Result::Ok;
    };

    if (!loop)
    {
      const mercuryapi::Result rr = do_read_once();
      const mercuryapi::Result dr = reader.Destroy();
      if (dr != mercuryapi::Result::Ok)
      {
        std::cerr << "[WARN] Destroy failed (" << reader.GetLastErrorString() << ")\n";
      }
      return (rr == mercuryapi::Result::Ok) ? 0 : 4;
    }

    // loop==true
    std::cout << "[OK] Loop mode start"
              << " interval_ms=" << loop_interval_ms
              << " count=" << loop_count << "\n";

    int iter = 0;
    while (true)
    {
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
    if (dr != mercuryapi::Result::Ok)
    {
      std::cerr << "[WARN] Destroy failed (" << reader.GetLastErrorString() << ")\n";
    }

    std::cout << "[OK] Done\n";
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "[ERR] std::exception: " << e.what() << "\n";
    return 3;
  }
}
