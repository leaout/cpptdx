/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once

#include <string>
#include "yaml-cpp/yaml.h"

namespace cpptdx {
using namespace std;
// using namespace lolog;
class Config {
    Config(){};

public:
    Config(const Config &) = delete;
    Config(const Config &&) = delete;
    Config &operator=(const Config &) = delete;
    Config &operator=(const Config &&) = delete;

    static Config *get_instance() {
        static Config config;
        return &config;
    }
    vector<pair<string, string>> tdx_hosts;
    string port;

    bool load_config(const string &config_file) {
        YAML::Node config;
        try {
            config = YAML::LoadFile(config_file);
        } catch (YAML::BadFile &e) {
            // LOERROR() << "yaml config read error :" << e.msg;
            return false;
        }
        try {
            port = config["server"]["port"].as<string>();
            // tdx_host = config["tdx"]["host"].as<string>();
            // tdx_port = config["tdx"]["port"].as<string>();

            auto nodeAbility = config["tdx"]["hosts"];
            tdx_hosts.reserve(nodeAbility.size());
            for (auto eg : nodeAbility) {
                tdx_hosts.emplace_back(std::make_pair(eg["host"].as<std::string>(), eg["port"].as<std::string>()));
            }

        } catch (YAML::TypedBadConversion<string> &e) {
            // LOERROR() << "yaml config read error :" << e.msg;
            return false;
        }
        return true;
    }
};
}  // namespace cpptdx