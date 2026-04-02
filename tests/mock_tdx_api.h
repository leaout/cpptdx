#pragma once
#include "include/tdxhq_api.h"
#include <vector>
#include <string>

namespace cpptdx {

class MockTdxHqApi : public TdxHqApi {
public:
    MockTdxHqApi() : connected_(false) {}
    ~MockTdxHqApi() override {}

    void set_connected(bool connected) { connected_ = connected; }
    void set_klines(std::vector<Kline> klines) { klines_ = std::move(klines); }
    void set_snapshots(std::vector<SnapShot> snapshots) { snapshots_ = std::move(snapshots); }
    void set_securities(std::vector<SecurityInfo> securities) { securities_ = std::move(securities); }
    void set_finance_info(FinanceInfo info) { finance_info_ = info; }
    void set_security_count(size_t count) { security_count_ = count; }

    bool connect(const std::string& host, const std::string& port) override {
        return connected_;
    }

    std::vector<Kline> get_security_klines(Category cat, Market market, 
                                           const std::string& code, 
                                           unsigned short start, 
                                           unsigned short count) override {
        return klines_;
    }

    std::vector<SnapShot> get_security_snapshots(const std::vector<std::pair<std::string, Market>>& stock_list) override {
        return snapshots_;
    }

    std::vector<SecurityInfo> get_security_list(Market market, unsigned short start) override {
        return securities_;
    }

    FinanceInfo get_finance_info(Market market, const std::string& code) override {
        return finance_info_;
    }

    size_t get_security_count(Market market) override {
        return security_count_;
    }

private:
    bool connected_;
    std::vector<Kline> klines_;
    std::vector<SnapShot> snapshots_;
    std::vector<SecurityInfo> securities_;
    FinanceInfo finance_info_;
    size_t security_count_ = 0;
};

}
