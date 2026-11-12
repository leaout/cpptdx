#pragma once
#include "include/tdxhq_api.h"
#include "httplib/httplib.h"
#include "storage/market_store.h"
#include "include/tdxexhq_api.h"
#include <atomic>
#include <thread>
#include <memory>
#include <vector>

namespace cpptdx {

class HttpServer {
public:
    HttpServer();
    ~HttpServer();
    void start(const std::string& host, int port);
    void stop();

private:
    void setup_routes();
    void handle_get_security_count(const httplib::Request& req, httplib::Response& res, bool local = false);
    void handle_get_local_klines(const httplib::Request& req, httplib::Response& res, bool index_bars);
    void handle_get_local_k_data(const httplib::Request& req, httplib::Response& res);
    void handle_intraday_data(const httplib::Request& req, httplib::Response& res, int kind);
    void handle_local_intraday_data(const httplib::Request& req, httplib::Response& res, int kind);
    void handle_company_info(const httplib::Request& req, httplib::Response& res, bool content, bool local);
    void handle_xdxr_info(const httplib::Request& req, httplib::Response& res, bool local);
    void handle_exhq(const httplib::Request& req, httplib::Response& res, int kind);
    void handle_block_info(const httplib::Request& req, httplib::Response& res, int operation, bool local);
    void handle_get_snapshots(const httplib::Request& req, httplib::Response& res, bool local = false);
    void handle_get_security_list(const httplib::Request& req, httplib::Response& res, bool local = false);
    void handle_get_klines(const httplib::Request& req, httplib::Response& res, bool index_bars);
    void handle_get_k_data(const httplib::Request& req, httplib::Response& res);
    void handle_get_finance_info(const httplib::Request& req, httplib::Response& res, bool local);

private:
    std::shared_ptr<TdxHqApi> tdx_hq_api_;
    std::shared_ptr<TdxExHqApi> tdx_ex_hq_api_;
    std::unique_ptr<MarketStore> market_store_;
    std::unique_ptr<httplib::Server> server_;
    std::thread server_thread_;
    std::thread tdx_thread_;
    std::thread exhq_thread_;
    std::atomic_bool running_{false};
};

int run_server();

}
