#include "http_service.h"
#include "config.h"
#include <httplib/httplib.h>
#include "json/json.hpp"
#include <thread>
#include <chrono>

namespace cpptdx {

HttpServer::HttpServer()
    : running_(false) {
    tdx_hq_api_ = create_tdx_hq_api();
    server_ = std::make_unique<httplib::Server>();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start(const std::string& host, int port) {
    std::thread tdx_thread([this]() {
        auto config = Config::get_instance();
        for (auto& h : config->tdx_hosts) {
            if (tdx_hq_api_->connect(h.first, h.second)) {
                break;
            }
        }
    });
    tdx_thread.detach();

    setup_routes();

    running_ = true;
    server_thread_ = std::thread([this, host, port]() {
        server_->listen(host, port);
    });
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        if (server_->is_running()) {
            server_->stop();
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }
}

void HttpServer::setup_routes() {
    server_->Get("/api/snapshots", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_snapshots(req, res);
    });

    server_->Get("/api/securities", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_security_list(req, res);
    });

    server_->Get("/api/klines", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_security_klines(req, res);
    });

    server_->Get("/api/finance", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_finance_info(req, res);
    });

    server_->Get("/health", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });
}

void HttpServer::handle_get_snapshots(const httplib::Request& req, httplib::Response& res) {
    vector<pair<string, Market>> stock_list;

    if (req.has_param("stocks")) {
        auto stocks_str = req.get_param_value("stocks");
        try {
            auto array = nlohmann::json::parse(stocks_str);
            if (array.is_array()) {
                for (const auto& item : array.items()) {
                    std::string code_str = item.value().value("code", "");
                    std::string market_str = item.value().value("market", "0");
                    Market market = (Market)std::atoi(market_str.c_str());
                    stock_list.emplace_back(code_str, market);
                }
            }
        } catch (nlohmann::json::type_error& e) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid stocks parameter\"}", "application/json");
            return;
        }
    }

    auto snapshots = tdx_hq_api_->get_security_snapshots(stock_list);

    nlohmann::json ret_array = nlohmann::json::array();
    try {
        for (const auto& snap : snapshots) {
            nlohmann::json j;
            to_json(j, snap);
            ret_array.emplace_back(j);
        }
    } catch (nlohmann::json::type_error& e) {
        res.status = 500;
        res.set_content("{\"error\":\"serialization error\"}", "application/json");
        return;
    }

    res.set_content(ret_array.dump(), "application/json");
}

void HttpServer::handle_get_security_list(const httplib::Request& req, httplib::Response& res) {
    Market market = Market::kSH;
    int start = 0;

    if (req.has_param("market")) {
        market = (Market)std::atoi(req.get_param_value("market").c_str());
    }
    if (req.has_param("start")) {
        start = std::atoi(req.get_param_value("start").c_str());
    }

    nlohmann::json array = nlohmann::json::array();
    int count = tdx_hq_api_->get_security_count(market);
    for (int i = 0; i < (count / 1000) + 1; ++i) {
        auto security_infos = tdx_hq_api_->get_security_list(market, i * 1000);
        vector<SecurityInfo> filtered_infos;
        std::copy_if(security_infos.begin(), security_infos.end(), std::back_inserter(filtered_infos), [&](const SecurityInfo& info) {
            if (market == Market::kSH) {
                return info.code.size() > 1 && info.code[0] == '6';
            } else {
                return info.code.size() > 1 && 
                       (std::strncmp(info.code.c_str(), "00", 2) == 0 || 
                        std::strncmp(info.code.c_str(), "30", 2) == 0);
            }
        });

        try {
            for (const auto& info : filtered_infos) {
                nlohmann::json j;
                to_json(j, info);
                array.emplace_back(j);
            }
        } catch (nlohmann::json::type_error& e) {
            res.status = 500;
            res.set_content("{\"error\":\"serialization error\"}", "application/json");
            return;
        }
    }

    res.set_content(array.dump(), "application/json");
}

void HttpServer::handle_get_security_klines(const httplib::Request& req, httplib::Response& res) {
    int cat = 0;
    int market = 0;
    std::string code;
    int start = 0;
    int count = 100;

    if (req.has_param("cat")) {
        cat = std::atoi(req.get_param_value("cat").c_str());
    }
    if (req.has_param("market")) {
        market = std::atoi(req.get_param_value("market").c_str());
    }
    if (req.has_param("code")) {
        code = req.get_param_value("code");
    }
    if (req.has_param("start")) {
        start = std::atoi(req.get_param_value("start").c_str());
    }
    if (req.has_param("count")) {
        count = std::atoi(req.get_param_value("count").c_str());
    }

    if (code.empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"code parameter is required\"}", "application/json");
        return;
    }

    auto res_klines = tdx_hq_api_->get_security_klines((Category)cat, (Market)market, code, start, count);

    nlohmann::json array = nlohmann::json::array();
    try {
        for (const auto& k : res_klines) {
            nlohmann::json j;
            to_json(j, k);
            array.emplace_back(j);
        }
    } catch (nlohmann::json::type_error& e) {
        res.status = 500;
        res.set_content("{\"error\":\"serialization error\"}", "application/json");
        return;
    }

    res.set_content(array.dump(), "application/json");
}

void HttpServer::handle_get_finance_info(const httplib::Request& req, httplib::Response& res) {
    int market = 0;
    std::string code;

    if (req.has_param("market")) {
        market = std::atoi(req.get_param_value("market").c_str());
    }
    if (req.has_param("code")) {
        code = req.get_param_value("code");
    }

    if (code.empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"code parameter is required\"}", "application/json");
        return;
    }

    auto finance_info = tdx_hq_api_->get_finance_info((Market)market, code);

    nlohmann::json j;
    j["code"] = finance_info.code;
    j["market"] = (int)finance_info.market;
    j["liutongguben"] = finance_info.liutongguben;
    j["province"] = finance_info.province;
    j["industry"] = finance_info.industry;
    j["updated_date"] = finance_info.updated_date;
    j["ipo_date"] = finance_info.ipo_date;
    j["zongguben"] = finance_info.zongguben;
    j["guojiagu"] = finance_info.guojiagu;
    j["faqirenfarengu"] = finance_info.faqirenfarengu;
    j["farengu"] = finance_info.farengu;
    j["bgu"] = finance_info.bgu;
    j["hgu"] = finance_info.hgu;
    j["zhigonggu"] = finance_info.zhigonggu;
    j["zongzichan"] = finance_info.zongzichan;
    j["liudongzichan"] = finance_info.liudongzichan;
    j["gudingzichan"] = finance_info.gudingzichan;
    j["wuxingzichan"] = finance_info.wuxingzichan;
    j["gudongrenshu"] = finance_info.gudongrenshu;
    j["liudongfuzhai"] = finance_info.liudongfuzhai;
    j["changqifuzhai"] = finance_info.changqifuzhai;
    j["zibengongjijin"] = finance_info.zibengongjijin;
    j["jingzichan"] = finance_info.jingzichan;
    j["zhuyingshouru"] = finance_info.zhuyingshouru;
    j["zhuyinglirun"] = finance_info.zhuyinglirun;
    j["yingshouzhangkuan"] = finance_info.yingshouzhangkuan;
    j["yingyelirun"] = finance_info.yingyelirun;
    j["touzishouyu"] = finance_info.touzishouyu;
    j["jingyingxianjinliu"] = finance_info.jingyingxianjinliu;
    j["zongxianjinliu"] = finance_info.zongxianjinliu;
    j["cunhuo"] = finance_info.cunhuo;
    j["lirunzonghe"] = finance_info.lirunzonghe;
    j["shuihoulirun"] = finance_info.shuihoulirun;
    j["jinglirun"] = finance_info.jinglirun;
    j["weifenlirun"] = finance_info.weifenlirun;

    res.set_content(j.dump(), "application/json");
}

int run_server() {
    HttpServer server;
    auto config = Config::get_instance();
    int port = std::stoi(config->port);
    server.start("0.0.0.0", port);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}

}
