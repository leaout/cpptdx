/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once

#include <string>
#include "lolog/Lolog-Inl.h"
#include "yaml-cpp/yaml.h"

namespace cpptdx {
using namespace std;
using namespace lolog;
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

    string tdx_host;
    string tdx_port;
    string port;

    bool load_config(const string &config_file) {
        YAML::Node config;
        try {
            config = YAML::LoadFile(config_file);
        } catch (YAML::BadFile &e) {
            LOERROR() << "yaml config read error :" << e.msg;
            return false;
        }
        try {
            port = config["server"]["port"].as<string>();
            tdx_host = config["tdx"]["host"].as<string>();
            tdx_port = config["tdx"]["port"].as<string>();
        } catch (YAML::TypedBadConversion<string> &e) {
            LOERROR() << "yaml config read error :" << e.msg;
            return false;
        }
        return true;
    }
};
}  // namespace cpptdx