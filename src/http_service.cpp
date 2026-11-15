#include "http_service.h"
#include "config.h"
#include "msg/get_block_info.h"
#include <httplib/httplib.h>
#include "json/json.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>

namespace cpptdx {

HttpServer::HttpServer()
    : running_(false) {
    tdx_hq_api_ = create_tdx_hq_api();
    tdx_ex_hq_api_ = create_tdx_ex_hq_api();
    market_store_ = std::make_unique<MarketStore>(Config::get_instance()->database_path);
    server_ = std::make_unique<httplib::Server>();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start(const std::string& host, int port) {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) return;

    tdx_thread_ = std::thread([this]() {
        auto config = Config::get_instance();
        for (auto& h : config->tdx_hosts) {
            if (tdx_hq_api_->connect(h.first, h.second)) {
                break;
            }
        }
    });
    exhq_thread_ = std::thread([this]() {
        auto config = Config::get_instance();
        for (auto& host : config->exhq_hosts) {
            if (tdx_ex_hq_api_->connect(host.first, host.second)) break;
        }
    });

    setup_routes();

    server_thread_ = std::thread([this, host, port]() {
        server_->listen(host, port);
    });
}

void HttpServer::stop() {
    running_ = false;
    if (server_ && server_->is_running()) server_->stop();
    if (server_thread_.joinable()) server_thread_.join();
    if (tdx_thread_.joinable()) tdx_thread_.join();
    if (exhq_thread_.joinable()) exhq_thread_.join();
}

void HttpServer::setup_routes() {
    const auto serve_test_page = [](const httplib::Request&, httplib::Response& res) {
        for (const auto& path : {std::string("web/index.html"), std::string("../web/index.html")}) {
            std::ifstream page(path, std::ios::binary);
            if (!page) continue;
            std::ostringstream content;
            content << page.rdbuf();
            res.set_content(content.str(), "text/html; charset=utf-8");
            return;
        }
        res.status = 404;
        res.set_content("Test page was not found. Reconfigure and rebuild the project.", "text/plain; charset=utf-8");
    };
    server_->Get("/", serve_test_page);
    server_->Get("/test", serve_test_page);

    server_->Get("/api/securities/count", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_security_count(req, res);
    });
    server_->Get("/api/local/securities/count", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_security_count(req, res, true);
    });

    server_->Get("/api/local/klines", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_local_klines(req, res, false);
    });

    server_->Get("/api/local/index/klines", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_local_klines(req, res, true);
    });
    server_->Get("/api/local/kdata", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_local_k_data(req, res);
    });

    server_->Get("/api/storage/status", [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json status = {{"backend", "duckdb"}, {"available", market_store_ && market_store_->available()}};
        res.set_content(status.dump(), "application/json");
    });
    server_->Get("/api/exhq/markets", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 0);
    });
    server_->Get("/api/exhq/instruments/count", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 1);
    });
    server_->Get("/api/exhq/instruments", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 2);
    });
    server_->Get("/api/local/exhq/markets", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 3);
    });
    server_->Get("/api/local/exhq/instruments/count", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 4);
    });
    server_->Get("/api/local/exhq/instruments", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 5);
    });
    server_->Get("/api/exhq/quote", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 6);
    });
    server_->Get("/api/exhq/quote-list", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 7);
    });
    server_->Get("/api/exhq/bars", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 8);
    });
    server_->Get("/api/exhq/bars/range", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 9);
    });
    server_->Get("/api/exhq/minute", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 10);
    });
    server_->Get("/api/exhq/history/minute", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 11);
    });
    server_->Get("/api/exhq/transactions", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 12);
    });
    server_->Get("/api/exhq/history/transactions", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 13);
    });
    server_->Get("/api/local/exhq/quote", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 14);
    });
    server_->Get("/api/local/exhq/quote-list", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 15);
    });
    server_->Get("/api/local/exhq/bars", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 16);
    });
    server_->Get("/api/local/exhq/bars/range", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 17);
    });
    server_->Get("/api/local/exhq/minute", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 18);
    });
    server_->Get("/api/local/exhq/history/minute", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 19);
    });
    server_->Get("/api/local/exhq/transactions", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 20);
    });
    server_->Get("/api/local/exhq/history/transactions", [this](const httplib::Request& req, httplib::Response& res) {
        handle_exhq(req, res, 21);
    });

    server_->Get("/api/minute", [this](const httplib::Request& req, httplib::Response& res) {
        handle_intraday_data(req, res, 0);
    });
    server_->Get("/api/history/minute", [this](const httplib::Request& req, httplib::Response& res) {
        handle_intraday_data(req, res, 1);
    });
    server_->Get("/api/transactions", [this](const httplib::Request& req, httplib::Response& res) {
        handle_intraday_data(req, res, 2);
    });
    server_->Get("/api/history/transactions", [this](const httplib::Request& req, httplib::Response& res) {
        handle_intraday_data(req, res, 3);
    });
    server_->Get("/api/company/category", [this](const httplib::Request& req, httplib::Response& res) {
        handle_company_info(req, res, false, false);
    });
    server_->Get("/api/company/content", [this](const httplib::Request& req, httplib::Response& res) {
        handle_company_info(req, res, true, false);
    });
    server_->Get("/api/local/company/category", [this](const httplib::Request& req, httplib::Response& res) {
        handle_company_info(req, res, false, true);
    });
    server_->Get("/api/local/company/content", [this](const httplib::Request& req, httplib::Response& res) {
        handle_company_info(req, res, true, true);
    });
    server_->Get("/api/xdxr", [this](const httplib::Request& req, httplib::Response& res) {
        handle_xdxr_info(req, res, false);
    });
    server_->Get("/api/local/xdxr", [this](const httplib::Request& req, httplib::Response& res) {
        handle_xdxr_info(req, res, true);
    });
    server_->Get("/api/local/minute", [this](const httplib::Request& req, httplib::Response& res) {
        handle_local_intraday_data(req, res, 0);
    });
    server_->Get("/api/local/history/minute", [this](const httplib::Request& req, httplib::Response& res) {
        handle_local_intraday_data(req, res, 1);
    });
    server_->Get("/api/local/transactions", [this](const httplib::Request& req, httplib::Response& res) {
        handle_local_intraday_data(req, res, 2);
    });
    server_->Get("/api/local/history/transactions", [this](const httplib::Request& req, httplib::Response& res) {
        handle_local_intraday_data(req, res, 3);
    });

    server_->Get("/api/snapshots", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_snapshots(req, res);
    });
    server_->Get("/api/local/snapshots", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_snapshots(req, res, true);
    });
    server_->Get("/api/block/meta", [this](const httplib::Request& req, httplib::Response& res) {
        handle_block_info(req, res, 0, false);
    });
    server_->Get("/api/block/content", [this](const httplib::Request& req, httplib::Response& res) {
        handle_block_info(req, res, 1, false);
    });
    server_->Get("/api/block/parsed", [this](const httplib::Request& req, httplib::Response& res) {
        handle_block_info(req, res, 2, false);
    });
    server_->Get("/api/local/block/meta", [this](const httplib::Request& req, httplib::Response& res) {
        handle_block_info(req, res, 0, true);
    });
    server_->Get("/api/local/block/content", [this](const httplib::Request& req, httplib::Response& res) {
        handle_block_info(req, res, 1, true);
    });
    server_->Get("/api/local/block/parsed", [this](const httplib::Request& req, httplib::Response& res) {
        handle_block_info(req, res, 2, true);
    });

    server_->Get("/api/securities", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_security_list(req, res);
    });
    server_->Get("/api/local/securities", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_security_list(req, res, true);
    });

    server_->Get("/api/klines", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_klines(req, res, false);
    });

    server_->Get("/api/index/klines", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_klines(req, res, true);
    });
    server_->Get("/api/kdata", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_k_data(req, res);
    });

    server_->Get("/api/finance", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_finance_info(req, res, false);
    });
    server_->Get("/api/local/finance", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_finance_info(req, res, true);
    });

    server_->Get("/health", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });
}

void HttpServer::handle_get_security_count(const httplib::Request& req, httplib::Response& res, bool local) {
    if (local && (!market_store_ || !market_store_->available())) {
        res.status = 503;
        res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
        return;
    }
    int market_value = static_cast<int>(Market::kSH);
    if (req.has_param("market")) {
        const auto& value = req.get_param_value("market");
        if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) {
            res.status = 400;
            res.set_content("{\"error\":\"market must be 0 (SZ) or 1 (SH)\"}", "application/json");
            return;
        }
        market_value = std::atoi(value.c_str());
    }
    if (market_value < static_cast<int>(Market::kSZ) || market_value > static_cast<int>(Market::kSH)) {
        res.status = 400;
        res.set_content("{\"error\":\"market must be 0 (SZ) or 1 (SH)\"}", "application/json");
        return;
    }

    const auto market = static_cast<Market>(market_value);
    const auto count = local ? market_store_->get_security_count(market) : tdx_hq_api_->get_security_count(market);
    nlohmann::json result = {{"market", market_value}, {"count", count}};
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_get_local_klines(const httplib::Request& req, httplib::Response& res, bool index_bars) {
    if (!market_store_ || !market_store_->available()) {
        res.status = 503;
        res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
        return;
    }
    if (!req.has_param("code") || req.get_param_value("code").empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"code parameter is required\"}", "application/json");
        return;
    }
    auto parse_uint = [&](const char* key, int fallback, int maximum, int& value) {
        value = fallback;
        if (!req.has_param(key)) return true;
        const auto& text = req.get_param_value(key);
        if (text.empty() || text.size() > 5 || text.find_first_not_of("0123456789") != std::string::npos) return false;
        value = std::atoi(text.c_str());
        return value <= maximum;
    };
    int market = 0, category = 4, start = 0, count = 100;
    if (!parse_uint("market", 0, 1, market) || !parse_uint("cat", 4, 11, category) ||
        !parse_uint("start", 0, 65535, start) || !parse_uint("count", 100, 800, count) || count == 0) {
        res.status = 400;
        res.set_content("{\"error\":\"invalid market, cat, start, or count\"}", "application/json");
        return;
    }
    auto klines = index_bars
        ? market_store_->get_index_klines(static_cast<Market>(market), static_cast<Category>(category),
                                          req.get_param_value("code"), static_cast<uint32_t>(count),
                                          static_cast<uint32_t>(start))
        : market_store_->get_klines(static_cast<Market>(market), static_cast<Category>(category),
                                    req.get_param_value("code"), static_cast<uint32_t>(count),
                                    static_cast<uint32_t>(start));
    nlohmann::json result = nlohmann::json::array();
    for (const auto& kline : klines) result.push_back(kline);
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_get_local_k_data(const httplib::Request& req, httplib::Response& res) {
    if (!market_store_ || !market_store_->available()) {
        res.status = 503;
        res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
        return;
    }
    if (!req.has_param("code") || req.get_param_value("code").empty() ||
        !req.has_param("start_date") || !req.has_param("end_date")) {
        res.status = 400;
        res.set_content("{\"error\":\"code, start_date, and end_date are required\"}", "application/json");
        return;
    }
    auto parse_date = [](std::string value, std::uint32_t& date) {
        value.erase(std::remove(value.begin(), value.end(), '-'), value.end());
        if (value.size() != 8 || value.find_first_not_of("0123456789") != std::string::npos) return false;
        const auto year = std::atoi(value.substr(0, 4).c_str());
        const auto month = std::atoi(value.substr(4, 2).c_str());
        const auto day = std::atoi(value.substr(6, 2).c_str());
        if (year < 1900 || month < 1 || month > 12 || day < 1 || day > 31) return false;
        date = static_cast<std::uint32_t>(std::strtoul(value.c_str(), nullptr, 10));
        return true;
    };
    int market = 0;
    if (req.has_param("market")) {
        const auto& value = req.get_param_value("market");
        if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos ||
            (market = std::atoi(value.c_str())) < 0 || market > 1) {
            res.status = 400;
            res.set_content("{\"error\":\"market must be 0 or 1\"}", "application/json");
            return;
        }
    }
    std::uint32_t start_date = 0, end_date = 0;
    if (!parse_date(req.get_param_value("start_date"), start_date) ||
        !parse_date(req.get_param_value("end_date"), end_date) || start_date > end_date) {
        res.status = 400;
        res.set_content("{\"error\":\"dates must be YYYY-MM-DD (or YYYYMMDD) and start_date <= end_date\"}",
                        "application/json");
        return;
    }
    const auto records = market_store_->get_daily_klines(static_cast<Market>(market),
                                                         req.get_param_value("code"), start_date, end_date);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& record : records) result.push_back(record);
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_intraday_data(const httplib::Request& req, httplib::Response& res, int kind) {
    auto parse_uint = [&](const char* key, unsigned int fallback, unsigned int maximum, unsigned int& value) {
        value = fallback;
        if (!req.has_param(key)) return true;
        const auto& text = req.get_param_value(key);
        if (text.empty() || text.size() > 8 || text.find_first_not_of("0123456789") != std::string::npos) return false;
        value = static_cast<unsigned int>(std::strtoul(text.c_str(), nullptr, 10));
        return value <= maximum;
    };
    if (!req.has_param("code") || req.get_param_value("code").empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"code parameter is required\"}", "application/json");
        return;
    }
    unsigned int market = 0, date = 0, start = 0, count = 100;
    const bool needs_date = kind == 1 || kind == 3;
    const bool needs_page = kind >= 2;
    if (!parse_uint("market", 0, 1, market) || (needs_date && !parse_uint("date", 0, 99999999, date)) ||
        (needs_page && (!parse_uint("start", 0, 65535, start) || !parse_uint("count", 100, 1800, count) || count == 0))) {
        res.status = 400;
        res.set_content("{\"error\":\"invalid market, date, start, or count\"}", "application/json");
        return;
    }
    const auto m = static_cast<Market>(market);
    const auto& code = req.get_param_value("code");
    nlohmann::json result;
    if (kind == 0 || kind == 1) {
        auto records = kind == 0 ? tdx_hq_api_->get_minute_time_data(m, code)
                                 : tdx_hq_api_->get_history_minute_time_data(m, code, date);
        if (market_store_) market_store_->save_minute_data(m, code, kind == 0 ? 0 : date, records);
        result = records;
    } else if (kind == 2) {
        auto records = tdx_hq_api_->get_transaction_data(m, code, static_cast<unsigned short>(start), static_cast<unsigned short>(count));
        if (market_store_) market_store_->save_transactions(m, code, 0, start, records);
        result = records;
    } else {
        auto records = tdx_hq_api_->get_history_transaction_data(m, code, static_cast<unsigned short>(start), static_cast<unsigned short>(count), date);
        if (market_store_) {
            std::vector<TickRecord> stored;
            stored.reserve(records.size());
            for (const auto& record : records) stored.push_back({record.time, record.price, record.vol, 0, record.buyorsell});
            market_store_->save_transactions(m, code, date, start, stored);
        }
        result = records;
    }
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_local_intraday_data(const httplib::Request& req, httplib::Response& res, int kind) {
    if (!market_store_ || !market_store_->available()) {
        res.status = 503;
        res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
        return;
    }
    if (!req.has_param("code") || req.get_param_value("code").empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"code parameter is required\"}", "application/json");
        return;
    }
    auto parse_uint = [&](const char* key, unsigned int fallback, unsigned int maximum, unsigned int& value) {
        value = fallback;
        if (!req.has_param(key)) return true;
        const auto& text = req.get_param_value(key);
        if (text.empty() || text.size() > 8 || text.find_first_not_of("0123456789") != std::string::npos) return false;
        value = static_cast<unsigned int>(std::strtoul(text.c_str(), nullptr, 10));
        return value <= maximum;
    };
    unsigned int market = 0, date = 0, start = 0, count = 100;
    const bool historical = kind == 1 || kind == 3;
    const bool transactions = kind >= 2;
    if (!parse_uint("market", 0, 1, market) ||
        (historical && !parse_uint("date", 0, 99999999, date)) ||
        (transactions && (!parse_uint("start", 0, 65535, start) || !parse_uint("count", 100, 1800, count) || count == 0))) {
        res.status = 400;
        res.set_content("{\"error\":\"invalid market, date, start, or count\"}", "application/json");
        return;
    }
    const auto m = static_cast<Market>(market);
    const auto& code = req.get_param_value("code");
    nlohmann::json result;
    if (!transactions) {
        result = market_store_->get_minute_data(m, code, historical ? date : 0, 1000, start);
    } else {
        auto records = market_store_->get_transactions(m, code, historical ? date : 0, count, start);
        if (historical) {
            std::vector<HistoryTickRecord> history;
            history.reserve(records.size());
            for (const auto& item : records) history.push_back({item.time, item.price, item.vol, item.buyorsell});
            result = history;
        } else {
            result = records;
        }
    }
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_company_info(const httplib::Request& req, httplib::Response& res, bool content, bool local) {
    auto parse_uint = [&](const char* key, unsigned int fallback, unsigned int maximum, unsigned int& value) {
        value = fallback;
        if (!req.has_param(key)) return true;
        const auto& text = req.get_param_value(key);
        if (text.empty() || text.size() > 10 || text.find_first_not_of("0123456789") != std::string::npos) return false;
        value = static_cast<unsigned int>(std::strtoul(text.c_str(), nullptr, 10));
        return value <= maximum;
    };
    if (!req.has_param("code") || req.get_param_value("code").empty() ||
        (content && (!req.has_param("filename") || req.get_param_value("filename").empty()))) {
        res.status = 400;
        res.set_content("{\"error\":\"code and filename (for content) are required\"}", "application/json");
        return;
    }
    unsigned int market = 0, start = 0, length = 0;
    if (!parse_uint("market", 0, 1, market) ||
        (content && (!parse_uint("start", 0, 4294967295U, start) || !parse_uint("length", 0, 1048576, length) || length == 0))) {
        res.status = 400;
        res.set_content("{\"error\":\"invalid market, start, or length\"}", "application/json");
        return;
    }
    const auto m = static_cast<Market>(market);
    const auto& code = req.get_param_value("code");
    if (content) {
        const auto& filename = req.get_param_value("filename");
        if (filename.size() > 80 || filename.find("..") != std::string::npos || filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid company filename\"}", "application/json");
            return;
        }
        std::string value;
        if (local) {
            if (!market_store_ || !market_store_->available()) {
                res.status = 503;
                res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
                return;
            }
            value = market_store_->get_company_info_content(m, code, filename, start, length);
        } else {
            value = tdx_hq_api_->get_company_info_content(m, code, filename, start, length);
            if (market_store_) market_store_->save_company_info_content(m, code, filename, start, length, value);
        }
        if (local && value.empty()) {
            res.status = 404;
            res.set_content("{\"error\":\"company content not found in local store\"}", "application/json");
            return;
        }
        res.set_content(value, "text/plain; charset=utf-8");
    } else {
        nlohmann::json result;
        if (local) {
            if (!market_store_ || !market_store_->available()) {
                res.status = 503;
                res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
                return;
            }
            result = market_store_->get_company_info_category(m, code);
        } else {
            auto files = tdx_hq_api_->get_company_info_category(m, code);
            if (market_store_) market_store_->save_company_info_category(m, code, files);
            result = files;
        }
        res.set_content(result.dump(), "application/json");
    }
}

void HttpServer::handle_xdxr_info(const httplib::Request& req, httplib::Response& res, bool local) {
    if (!req.has_param("code") || req.get_param_value("code").empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"code parameter is required\"}", "application/json");
        return;
    }
    unsigned int market = 0;
    if (req.has_param("market")) {
        const auto& value = req.get_param_value("market");
        if (value.empty() || value.size() > 1 || value.find_first_not_of("01") != std::string::npos) {
            res.status = 400;
            res.set_content("{\"error\":\"market must be 0 or 1\"}", "application/json");
            return;
        }
        market = static_cast<unsigned int>(std::atoi(value.c_str()));
    }
    const auto m = static_cast<Market>(market);
    const auto& code = req.get_param_value("code");
    std::vector<XdxrRecord> records;
    if (local) {
        if (!market_store_ || !market_store_->available()) {
            res.status = 503;
            res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
            return;
        }
        records = market_store_->get_xdxr_info(m, code);
    } else {
        records = tdx_hq_api_->get_xdxr_info(m, code);
        if (market_store_) market_store_->save_xdxr_info(m, code, records);
    }
    nlohmann::json result = records;
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_exhq(const httplib::Request& req, httplib::Response& res, int kind) {
    const bool local = (kind >= 3 && kind <= 5) || kind >= 14;
    if (local && (!market_store_ || !market_store_->available())) {
        res.status = 503;
        res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
        return;
    }
    if (!local && (!tdx_ex_hq_api_ || !tdx_ex_hq_api_->connected())) {
        res.status = 503;
        res.set_content("{\"error\":\"ExHq client is unavailable\"}", "application/json");
        return;
    }
    if (kind == 0 || kind == 3) {
        if (local) {
            nlohmann::json result = market_store_->get_ex_markets();
            res.set_content(result.dump(), "application/json");
            return;
        }
        const auto markets = tdx_ex_hq_api_->get_markets();
        if (market_store_) market_store_->save_ex_markets(markets);
        nlohmann::json result = markets;
        res.set_content(result.dump(), "application/json");
        return;
    }
    if (kind == 1 || kind == 4) {
        if (local) {
            res.set_content(std::to_string(market_store_->get_ex_instrument_count()), "application/json");
            return;
        }
        res.set_content(std::to_string(tdx_ex_hq_api_->get_instrument_count()), "application/json");
        return;
    }
    if (kind == 2 || kind == 5) {
        auto parse_uint = [&](const char* key, unsigned int fallback, unsigned int maximum, unsigned int& value) {
            value = fallback;
            if (!req.has_param(key)) return true;
            const auto& text = req.get_param_value(key);
            if (text.empty() || text.size() > 10 || text.find_first_not_of("0123456789") != std::string::npos) return false;
            value = static_cast<unsigned int>(std::strtoul(text.c_str(), nullptr, 10));
            return value <= maximum;
        };
        unsigned int start = 0, count = 100;
        if (!parse_uint("start", 0, 4294967295U, start) || !parse_uint("count", 100, 1000, count) || count == 0) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid instrument start/count\"}", "application/json");
            return;
        }
        if (local) {
            nlohmann::json result = market_store_->get_ex_instruments(count, start);
            res.set_content(result.dump(), "application/json");
            return;
        }
        auto instruments = tdx_ex_hq_api_->get_instrument_info(start, static_cast<unsigned short>(count));
        market_store_->save_ex_instruments(instruments);
        nlohmann::json result = instruments;
        res.set_content(result.dump(), "application/json");
        return;
    }

    const int operation = local ? kind - 8 : kind;
    auto parse_uint = [&](const char* key, unsigned int fallback, unsigned int maximum, unsigned int& value) {
        value = fallback;
        if (!req.has_param(key)) return true;
        const auto& text = req.get_param_value(key);
        if (text.empty() || text.size() > 10 || text.find_first_not_of("0123456789") != std::string::npos) return false;
        value = static_cast<unsigned int>(std::strtoul(text.c_str(), nullptr, 10));
        return value <= maximum;
    };
    unsigned int market = 0, category = 0, start = 0, count = 0, date = 0, start_date = 0, end_date = 0;
    const bool quote_list = operation == 7;
    const bool bars = operation == 8;
    const bool range = operation == 9;
    const bool history_minute = operation == 11;
    const bool live_transactions = operation == 12;
    const bool history_transactions = operation == 13;
    const bool has_code = operation != 7;
    const bool paged = quote_list || bars || live_transactions || history_transactions;
    const bool needs_date = history_minute || history_transactions;
    if (!parse_uint("market", 0, 255, market) || !req.has_param("market") ||
        (has_code && (!req.has_param("code") || req.get_param_value("code").empty())) ||
        (quote_list && (!req.has_param("category") || !parse_uint("category", 0, 255, category))) ||
        (bars && (!req.has_param("category") || !parse_uint("category", 0, 11, category))) ||
        (paged && (!parse_uint("start", 0, (bars ? 4294967295U : (operation == 7 ? 65535U : 2147483647U)), start) ||
                   !parse_uint("count", operation == 7 ? 80 : (bars ? 700 : 1800),
                               operation == 7 ? 1000 : (bars ? 800 : 1800), count) || count == 0)) ||
        (needs_date && (!req.has_param("date") || !parse_uint("date", 0, 99999999, date))) ||
        (range && (!parse_uint("start_date", 0, 99999999, start_date) ||
                   !parse_uint("end_date", 0, 99999999, end_date) || start_date > end_date ||
                   start_date < 19000101 || start_date % 100 == 0 || start_date % 100 > 31 ||
                   (start_date / 100) % 100 == 0 || (start_date / 100) % 100 > 12 ||
                   end_date < 19000101 || end_date % 100 == 0 || end_date % 100 > 31 ||
                   (end_date / 100) % 100 == 0 || (end_date / 100) % 100 > 12))) {
        res.status = 400;
        res.set_content("{\"error\":\"invalid ExHq market, code, category, date, or paging parameters\"}",
                        "application/json");
        return;
    }
    if ((quote_list && category != 2 && category != 3) ||
        (needs_date && (date < 19000101 || date % 100 < 1 || date % 100 > 31 ||
                        (date / 100) % 100 < 1 || (date / 100) % 100 > 12))) {
        res.status = 400;
        res.set_content("{\"error\":\"date must be a valid YYYYMMDD value\"}", "application/json");
        return;
    }
    const auto code = has_code ? req.get_param_value("code") : std::to_string(category);
    std::string cache_kind;
    std::uint32_t cache_date = 0;
    if (operation == 6) cache_kind = "quote";
    else if (quote_list) cache_kind = "quote-list-" + std::to_string(category);
    else if (bars) cache_kind = "bars-" + std::to_string(category);
    else if (range) { cache_kind = "bars-range-" + std::to_string(end_date); cache_date = start_date; }
    else if (operation == 10) cache_kind = "minute";
    else if (history_minute) { cache_kind = "history-minute"; cache_date = date; }
    else if (live_transactions) cache_kind = "transactions";
    else if (history_transactions) { cache_kind = "history-transactions"; cache_date = date; }
    else { res.status = 404; return; }

    if (local) {
        const auto records = market_store_->get_ex_data(cache_kind, static_cast<unsigned char>(market), code,
            cache_date, paged ? count : 10000, paged ? start : 0);
        nlohmann::json result = nlohmann::json::array();
        for (const auto& record : records) result.push_back(record);
        res.set_content(result.dump(), "application/json");
        return;
    }

    nlohmann::json result = nlohmann::json::array();
    const auto market_id = static_cast<unsigned char>(market);
    if (operation == 6) result = tdx_ex_hq_api_->get_instrument_quote(market_id, code);
    else if (quote_list) result = tdx_ex_hq_api_->get_instrument_quote_list(market_id,
        static_cast<unsigned char>(category), static_cast<unsigned short>(start), static_cast<unsigned short>(count));
    else if (bars) result = tdx_ex_hq_api_->get_instrument_bars(static_cast<unsigned char>(category), market_id,
        code, start, static_cast<unsigned short>(count));
    else if (range) result = tdx_ex_hq_api_->get_history_instrument_bars_range(market_id, code, start_date, end_date);
    else if (operation == 10) result = tdx_ex_hq_api_->get_minute_time_data(market_id, code);
    else if (history_minute) result = tdx_ex_hq_api_->get_history_minute_time_data(market_id, code, date);
    else if (live_transactions) result = tdx_ex_hq_api_->get_transaction_data(market_id, code,
        static_cast<std::int32_t>(start), static_cast<unsigned short>(count));
    else if (history_transactions) result = tdx_ex_hq_api_->get_history_transaction_data(market_id, code,
        date, static_cast<std::int32_t>(start), static_cast<unsigned short>(count));
    market_store_->save_ex_data(cache_kind, market_id, code, cache_date, range ? 0 : start, result);
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_block_info(const httplib::Request& req, httplib::Response& res,
                                   int operation, bool local) {
    if (local && (!market_store_ || !market_store_->available())) {
        res.status = 503;
        res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
        return;
    }
    if (!local && (!tdx_hq_api_)) {
        res.status = 503;
        res.set_content("{\"error\":\"HQ client is unavailable\"}", "application/json");
        return;
    }
    if (!req.has_param("blockfile")) {
        res.status = 400;
        res.set_content("{\"error\":\"blockfile is required\"}", "application/json");
        return;
    }
    const auto filename = req.get_param_value("blockfile");
    if (filename.empty() || filename.size() > 100 ||
        filename.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != std::string::npos) {
        res.status = 400;
        res.set_content("{\"error\":\"invalid blockfile name\"}", "application/json");
        return;
    }
    if (local) {
        const auto meta = market_store_->get_block_info_meta(filename);
        if (meta.size == 0) {
            res.status = 404;
            res.set_content("{\"error\":\"block file is not cached\"}", "application/json");
            return;
        }
        if (operation == 0) {
            res.set_content(nlohmann::json(meta).dump(), "application/json");
        } else if (operation == 1) {
            unsigned int start = 0, length = 0;
            auto parse = [&](const char* key, unsigned int max, unsigned int& out) {
                if (!req.has_param(key)) return false;
                const auto& value = req.get_param_value(key);
                if (value.empty() || value.size() > 10 || value.find_first_not_of("0123456789") != std::string::npos) return false;
                out = static_cast<unsigned int>(std::strtoul(value.c_str(), nullptr, 10));
                return out <= max;
            };
            if (!parse("start", meta.size, start) || !parse("size", 1000000, length) || length == 0 ||
                static_cast<std::uint64_t>(start) + length > meta.size) {
                res.status = 400;
                res.set_content("{\"error\":\"invalid block content range\"}", "application/json");
                return;
            }
            res.set_content(market_store_->get_block_info_content(filename, start, length), "application/octet-stream");
        } else {
            nlohmann::json result = market_store_->get_block_constituents(filename);
            res.set_content(result.dump(), "application/json");
        }
        return;
    }

    if (operation == 0) {
        const auto meta = tdx_hq_api_->get_block_info_meta(filename);
        if (meta.size == 0) {
            res.status = 502;
            res.set_content("{\"error\":\"TDX server returned empty block metadata\"}", "application/json");
            return;
        }
        res.set_content(nlohmann::json(meta).dump(), "application/json");
        return;
    }
    const auto meta = tdx_hq_api_->get_block_info_meta(filename);
    if (meta.size == 0) {
        res.status = 502;
        res.set_content("{\"error\":\"TDX server returned empty block metadata\"}", "application/json");
        return;
    }
    if (operation == 1) {
        unsigned int start = 0, length = 0;
        auto parse = [&](const char* key, unsigned int max, unsigned int& out) {
            if (!req.has_param(key)) return false;
            const auto& value = req.get_param_value(key);
            if (value.empty() || value.size() > 10 || value.find_first_not_of("0123456789") != std::string::npos) return false;
            out = static_cast<unsigned int>(std::strtoul(value.c_str(), nullptr, 10));
            return out <= max;
        };
        if (!parse("start", meta.size, start) || !parse("size", 1000000, length) || length == 0 ||
            static_cast<std::uint64_t>(start) + length > meta.size) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid block content range\"}", "application/json");
            return;
        }
        auto content = tdx_hq_api_->get_block_info(filename, start, length);
        if (content.size() > length) content.resize(length);
        res.set_content(content, "application/octet-stream");
        return;
    }
    constexpr std::uint32_t chunk_size = 0x7530;
    std::string content;
    content.reserve(meta.size);
    for (std::uint32_t offset = 0; offset < meta.size;) {
        const auto wanted = std::min(chunk_size, meta.size - offset);
        auto chunk = tdx_hq_api_->get_block_info(filename, offset, wanted);
        if (chunk.empty()) {
            res.status = 502;
            res.set_content("{\"error\":\"TDX server returned an incomplete block file\"}", "application/json");
            return;
        }
        if (chunk.size() > wanted) chunk.resize(wanted);
        content.append(chunk);
        offset += static_cast<std::uint32_t>(chunk.size());
    }
    if (content.size() != meta.size) {
        res.status = 502;
        res.set_content("{\"error\":\"incomplete block file\"}", "application/json");
        return;
    }
    const auto constituents = parse_block_info_file(content);
    if (!market_store_->save_block_file(filename, meta, content, constituents)) {
        res.status = 500;
        res.set_content("{\"error\":\"failed to cache block file\"}", "application/json");
        return;
    }
    nlohmann::json result = constituents;
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_get_snapshots(const httplib::Request& req, httplib::Response& res, bool local) {
    if (local && (!market_store_ || !market_store_->available())) {
        res.status = 503;
        res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
        return;
    }
    vector<pair<string, Market>> stock_list;

    if (req.has_param("stocks")) {
        auto stocks_str = req.get_param_value("stocks");
        try {
            auto array = nlohmann::json::parse(stocks_str);
            if (!array.is_array()) {
                res.status = 400;
                res.set_content("{\"error\":\"stocks must be an array\"}", "application/json");
                return;
            }
            for (const auto& item : array) {
                if (!item.is_object() || !item.contains("code") || !item["code"].is_string()) {
                    res.status = 400;
                    res.set_content("{\"error\":\"each stock requires a string code\"}", "application/json");
                    return;
                }
                const auto code = item["code"].get<std::string>();
                int market_value = 0;
                if (item.contains("market")) {
                    const auto& market = item["market"];
                    if (market.is_number_integer()) market_value = market.get<int>();
                    else if (market.is_string()) {
                        const auto text = market.get<std::string>();
                        if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
                            res.status = 400;
                            res.set_content("{\"error\":\"market must be 0 (SZ) or 1 (SH)\"}", "application/json");
                            return;
                        }
                        market_value = std::atoi(text.c_str());
                    } else {
                        res.status = 400;
                        res.set_content("{\"error\":\"market must be 0 (SZ) or 1 (SH)\"}", "application/json");
                        return;
                    }
                }
                if (code.empty() || market_value < 0 || market_value > 1) {
                    res.status = 400;
                    res.set_content("{\"error\":\"stock code and market are invalid\"}", "application/json");
                    return;
                }
                stock_list.emplace_back(code, static_cast<Market>(market_value));
            }
        } catch (const nlohmann::json::exception&) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid stocks parameter\"}", "application/json");
            return;
        }
    }

    auto snapshots = local ? market_store_->get_snapshots(stock_list)
                           : tdx_hq_api_->get_security_quotes(stock_list);
    if (!local && market_store_) market_store_->save_snapshots(snapshots);

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

void HttpServer::handle_get_security_list(const httplib::Request& req, httplib::Response& res, bool local) {
    if (local && (!market_store_ || !market_store_->available())) {
        res.status = 503;
        res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
        return;
    }
    auto parse_uint = [&](const char* key, int fallback, int maximum, int& value) {
        value = fallback;
        if (!req.has_param(key)) return true;
        const auto& text = req.get_param_value(key);
        if (text.empty() || text.size() > 5 || text.find_first_not_of("0123456789") != std::string::npos) return false;
        value = std::atoi(text.c_str());
        return value <= maximum;
    };
    int market_value = static_cast<int>(Market::kSH);
    int start = 0;
    if (!parse_uint("market", market_value, 1, market_value) || !parse_uint("start", 0, 65535, start)) {
        res.status = 400;
        res.set_content("{\"error\":\"market must be 0 or 1 and start must be 0..65535\"}", "application/json");
        return;
    }

    const auto market = static_cast<Market>(market_value);
    const auto infos = local ? market_store_->get_security_list(market, static_cast<std::uint32_t>(start), 1000)
                             : tdx_hq_api_->get_security_list(market, static_cast<unsigned short>(start));
    if (!local && market_store_) market_store_->save_security_list(market, static_cast<std::uint32_t>(start), infos);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& info : infos) result.push_back(info);
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_get_klines(const httplib::Request& req, httplib::Response& res, bool index_bars) {
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

    auto res_klines = index_bars
        ? tdx_hq_api_->get_index_bars((Category)cat, (Market)market, code, start, count)
        : tdx_hq_api_->get_security_bars((Category)cat, (Market)market, code, start, count);
    if (market_store_) {
        if (index_bars) {
            market_store_->save_index_klines(static_cast<Market>(market), static_cast<Category>(cat), code, res_klines);
        } else {
            market_store_->save_klines(static_cast<Market>(market), static_cast<Category>(cat), code, res_klines);
        }
    }

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

void HttpServer::handle_get_k_data(const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("code") || req.get_param_value("code").empty() ||
        !req.has_param("start_date") || !req.has_param("end_date")) {
        res.status = 400;
        res.set_content("{\"error\":\"code, start_date, and end_date are required\"}", "application/json");
        return;
    }
    const auto code = req.get_param_value("code");
    const auto start_date = req.get_param_value("start_date");
    const auto end_date = req.get_param_value("end_date");
    auto valid_date = [](std::string value) {
        value.erase(std::remove(value.begin(), value.end(), '-'), value.end());
        if (value.size() != 8 || value.find_first_not_of("0123456789") != std::string::npos) return false;
        const int month = std::atoi(value.substr(4, 2).c_str());
        const int day = std::atoi(value.substr(6, 2).c_str());
        return month >= 1 && month <= 12 && day >= 1 && day <= 31;
    };
    if (!valid_date(start_date) || !valid_date(end_date)) {
        res.status = 400;
        res.set_content("{\"error\":\"dates must use YYYY-MM-DD or YYYYMMDD\"}", "application/json");
        return;
    }
    auto date_digits = [](std::string value) {
        value.erase(std::remove(value.begin(), value.end(), '-'), value.end());
        return value;
    };
    if (date_digits(start_date) > date_digits(end_date)) {
        res.status = 400;
        res.set_content("{\"error\":\"start_date must not be after end_date\"}", "application/json");
        return;
    }
    auto records = tdx_hq_api_->get_k_data(code, start_date, end_date);
    const bool sh = code.front() == '5' || code.front() == '6' || code.front() == '9' ||
        code.rfind("009", 0) == 0 || code.rfind("126", 0) == 0 || code.rfind("110", 0) == 0 ||
        code.rfind("201", 0) == 0 || code.rfind("202", 0) == 0 || code.rfind("203", 0) == 0 ||
        code.rfind("204", 0) == 0;
    if (market_store_) {
        market_store_->save_klines(sh ? Market::kSH : Market::kSZ, Category::kDayK, code, records);
    }
    nlohmann::json result = nlohmann::json::array();
    for (const auto& record : records) result.push_back(record);
    res.set_content(result.dump(), "application/json");
}

void HttpServer::handle_get_finance_info(const httplib::Request& req, httplib::Response& res, bool local) {
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

    if (local) {
        if (!market_store_ || !market_store_->available()) {
            res.status = 503;
            res.set_content("{\"error\":\"DuckDB storage is unavailable\"}", "application/json");
            return;
        }
        const auto cached = market_store_->get_finance_info(static_cast<Market>(market), code);
        if (cached.empty()) {
            res.status = 404;
            res.set_content("{\"error\":\"finance record not found\"}", "application/json");
            return;
        }
        res.set_content(cached, "application/json");
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
    j["weifenpeilirun"] = finance_info.weifenlirun;
    j["meigujingzichan"] = finance_info.baoliu1;
    j["baoliu2"] = finance_info.baoliu2;

    if (market_store_) market_store_->save_finance_info(static_cast<Market>(market), code, j.dump());
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
