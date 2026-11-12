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
    boost::asio::io_context io_service_;
    shared_ptr<tcp::socket> socket_;
    std::atomic_bool is_running_;
    std::thread th_;
    std::mutex mutex_;
    std::chrono::high_resolution_clock::time_point last_send_time_;
public:
    TdxHqApiImpl(){}
    ~TdxHqApiImpl();
    bool connect(const string& host, const string& port) override;
    vector<Kline> get_security_klines(Category cat, Market market, const std::string& code, unsigned short start, unsigned short count) override;
    vector<Kline> get_index_klines(Category cat, Market market, const std::string& code, unsigned short start, unsigned short count) override;
    vector<MinuteRecord> get_minute_time_data(Market market, const std::string& code) override;
    vector<MinuteRecord> get_history_minute_time_data(Market market, const std::string& code, unsigned int date) override;
    vector<TickRecord> get_transaction_data(Market market, const std::string& code, unsigned short start, unsigned short count) override;
    vector<HistoryTickRecord> get_history_transaction_data(Market market, const std::string& code,
                                                           unsigned short start, unsigned short count,
                                                           unsigned int date) override;
    vector<CompanyInfoFile> get_company_info_category(Market market, const std::string& code) override;
    std::string get_company_info_content(Market market, const std::string& code,
                                         const std::string& filename, std::uint32_t start,
                                         std::uint32_t length) override;
    vector<XdxrRecord> get_xdxr_info(Market market, const std::string& code) override;
    size_t get_security_count(Market market) override;
    vector<SnapShot> get_security_snapshots(const vector<pair<string, Market>>& stock_list) override;
    vector<SecurityInfo> get_security_list(Market market, unsigned short start) override;
    FinanceInfo get_finance_info(Market market, const string& code) override;
    BlockInfoMeta get_block_info_meta(const std::string& blockfile) override;
    std::string get_block_info(const std::string& blockfile, std::uint32_t start, std::uint32_t size) override;
    std::vector<BlockConstituent> get_and_parse_block_info(const std::string& blockfile) override;
private:
    void stop(){
        is_running_ = false;
        join();
        if(socket_)
            socket_->close();
        
    }
    void join() {
        if (th_.joinable()) {
            th_.join();
        }
    }
    void heart_beat_thread() {
        while (is_running_) {
            auto elapsed = std::chrono::high_resolution_clock::now() - last_send_time_;
            if(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 10)
                get_security_count(Market::kSH);
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    std::vector<char> send(const std::vector<uint8_t>& send_data) { 
        return send((char*)send_data.data(), send_data.size()); 
    }
    std::vector<char> send(const char* send_data, size_t send_len);
};
}  // namespace cpptdx
