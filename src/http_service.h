#pragma once
#include "include/tdxhq_api.h"
#include "httplib/httplib.h"
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
    void handle_get_snapshots(const httplib::Request& req, httplib::Response& res);
    void handle_get_security_list(const httplib::Request& req, httplib::Response& res);
    void handle_get_security_klines(const httplib::Request& req, httplib::Response& res);
    void handle_get_finance_info(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<TdxHqApi> tdx_hq_api_;
    std::unique_ptr<httplib::Server> server_;
    std::thread server_thread_;
    bool running_;
};

int run_server();

}
