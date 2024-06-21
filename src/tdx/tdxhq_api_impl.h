/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once
#include <zlib.h>

#include <atomic>
#include <boost/asio.hpp>
#include <cmath>
#include <cstring>
#include <deque>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <mutex>
#include <thread>
#include "include/tdxhq_api.h"

using boost::asio::ip::tcp;

namespace cpptdx {
    
class TdxHqApiImpl : public TdxHqApi {
    boost::asio::io_service io_service_;
    shared_ptr<tcp::socket> socket_;
    std::atomic_bool is_running_;
    std::thread th_;
    std::mutex mutex_;
public:
    TdxHqApiImpl(){}
    ~TdxHqApiImpl();
    bool connect(const string& host, const string& port) override;
    vector<Kline> get_security_klines(Category cat, Market market, const std::string& code, unsigned short start, unsigned short count) override;
    size_t get_security_count(Market market) override;
    vector<SnapShot> get_security_snapshots(const vector<pair<string, Market>>& stock_list) override;
    vector<SecurityInfo> get_security_list(Market market, unsigned short start) override;
    FinanceInfo get_finance_info(Market market, const string& code) override;
private:
    void stop(){
        is_running_ = false;
        if(socket_)
            socket_->close();
        join();
    }
    void join() {
        if (th_.joinable()) {
            th_.join();
        }
    }
    void heart_beat_thread() {
        while (is_running_) {
            get_security_count(Market::kSH);
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    std::vector<char> send(const std::vector<uint8_t>& send_data) { 
        return send((char*)send_data.data(), send_data.size()); 
    }
    std::vector<char> send(const char* send_data, size_t send_len);
};
}  // namespace cpptdx
