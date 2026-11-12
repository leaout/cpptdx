/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#ifdef YAML_CPP_AVAILABLE
#include <yaml-cpp/yaml.h>
#endif

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
    vector<pair<string, string>> exhq_hosts;
    string port;
    string database_path = "data/cpptdx.duckdb";

    bool load_config(const string& config_file) {
#ifdef YAML_CPP_AVAILABLE
        YAML::Node config;
        try {
            config = YAML::LoadFile(config_file);
        } catch (YAML::BadFile& e) {
            return false;
        }
        try {
            tdx_hosts.clear();
            exhq_hosts.clear();
            port = config["server"]["port"].as<string>();
            if (config["database"] && config["database"]["path"]) {
                database_path = config["database"]["path"].as<string>();
            }
            auto nodeAbility = config["tdx"]["hosts"];
            tdx_hosts.reserve(nodeAbility.size());
            for (auto eg : nodeAbility) {
                tdx_hosts.emplace_back(std::make_pair(eg["host"].as<std::string>(), eg["port"].as<std::string>()));
            }
            auto exhq = config["exhq"]["hosts"];
            if (exhq) {
                exhq_hosts.reserve(exhq.size());
                for (auto host : exhq) exhq_hosts.emplace_back(host["host"].as<std::string>(), host["port"].as<std::string>());
            }
            return true;
        } catch (const YAML::Exception&) {
            return false;
        }
#else
        (void)config_file;
        port = "8022";
        database_path = "data/cpptdx.duckdb";
        tdx_hosts.emplace_back(std::make_pair("119.147.212.81", "7709"));
        tdx_hosts.emplace_back(std::make_pair("119.147.212.81", "7709"));
        return true;
#endif
    }
};
}
