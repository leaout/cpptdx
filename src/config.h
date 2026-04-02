/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace cpptdx {
using namespace std;

class Config {
    Config() = default;

public:
    Config(const Config&) = delete;
    Config(const Config&&) = delete;
    Config& operator=(const Config&) = delete;
    Config& operator=(const Config&&) = delete;

    static Config* get_instance() {
        static Config config;
        return &config;
    }
    vector<pair<string, string>> tdx_hosts;
    string port;

    bool load_config(const string& config_file) {
#ifdef YAML_CPP_AVAILABLE
        YAML::Node config;
        try {
            config = YAML::LoadFile(config_file);
        } catch (YAML::BadFile& e) {
            return false;
        }
        try {
            port = config["server"]["port"].as<string>();
            auto nodeAbility = config["tdx"]["hosts"];
            tdx_hosts.reserve(nodeAbility.size());
            for (auto eg : nodeAbility) {
                tdx_hosts.emplace_back(std::make_pair(eg["host"].as<std::string>(), eg["port"].as<std::string>()));
            }
            return true;
        } catch (YAML::TypedBadConversion<string>& e) {
            return false;
        }
#else
        (void)config_file;
        port = "8022";
        tdx_hosts.emplace_back(std::make_pair("119.147.212.81", "7709"));
        tdx_hosts.emplace_back(std::make_pair("119.147.212.81", "7709"));
        return true;
#endif
    }
};
}
