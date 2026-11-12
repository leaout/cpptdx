#pragma once

#include "include/tdxexhq_api.h"
#include <boost/asio.hpp>
#include <atomic>
#include <mutex>

namespace cpptdx {

class TdxExHqApiImpl : public TdxExHqApi {
public:
    ~TdxExHqApiImpl() override;
    bool connect(const std::string& host, const std::string& port) override;
    bool connected() const override;
    std::vector<ExMarket> get_markets() override;
    std::size_t get_instrument_count() override;
    std::vector<ExInstrument> get_instrument_info(unsigned int start, unsigned short count) override;
    std::vector<ExQuote> get_instrument_quote(unsigned char market, const std::string& code) override;
    std::vector<nlohmann::json> get_instrument_quote_list(unsigned char market, unsigned char category,
        unsigned short start, unsigned short count) override;
    std::vector<ExBar> get_instrument_bars(unsigned char category, unsigned char market,
        const std::string& code, std::uint32_t start, unsigned short count) override;
    std::vector<ExRangeBar> get_history_instrument_bars_range(unsigned char market, const std::string& code,
        std::uint32_t start_date, std::uint32_t end_date) override;
    std::vector<ExMinuteBar> get_minute_time_data(unsigned char market, const std::string& code) override;
    std::vector<ExMinuteBar> get_history_minute_time_data(unsigned char market, const std::string& code,
        std::uint32_t date) override;
    std::vector<ExTransaction> get_transaction_data(unsigned char market, const std::string& code,
        std::int32_t start, unsigned short count) override;
    std::vector<ExTransaction> get_history_transaction_data(unsigned char market, const std::string& code,
        std::uint32_t date, std::int32_t start, unsigned short count) override;

private:
    std::vector<char> send(const std::vector<char>& request);
    boost::asio::io_context io_service_;
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    std::mutex mutex_;
    std::atomic_bool connected_{false};
};

}  // namespace cpptdx
