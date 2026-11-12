#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "json/json.hpp"

namespace cpptdx {

struct ExMarket {
    unsigned char market = 0;
    unsigned char category = 0;
    std::string name;
    std::string short_name;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExMarket, market, category, name, short_name);

struct ExInstrument {
    unsigned char category = 0;
    unsigned char market = 0;
    std::string code;
    std::string name;
    std::string desc;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExInstrument, category, market, code, name, desc);

struct ExQuote {
    unsigned char market = 0;
    std::string code;
    double pre_close = 0, open = 0, high = 0, low = 0, price = 0;
    std::uint32_t kaicang = 0, zongliang = 0, xianliang = 0, neipan = 0, waipan = 0, chicang = 0;
    double bid1 = 0, bid2 = 0, bid3 = 0, bid4 = 0, bid5 = 0;
    std::uint32_t bid_vol1 = 0, bid_vol2 = 0, bid_vol3 = 0, bid_vol4 = 0, bid_vol5 = 0;
    double ask1 = 0, ask2 = 0, ask3 = 0, ask4 = 0, ask5 = 0;
    std::uint32_t ask_vol1 = 0, ask_vol2 = 0, ask_vol3 = 0, ask_vol4 = 0, ask_vol5 = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExQuote, market, code, pre_close, open, high, low, price, kaicang,
    zongliang, xianliang, neipan, waipan, chicang, bid1, bid2, bid3, bid4, bid5, bid_vol1, bid_vol2,
    bid_vol3, bid_vol4, bid_vol5, ask1, ask2, ask3, ask4, ask5, ask_vol1, ask_vol2, ask_vol3, ask_vol4, ask_vol5);

struct ExMinuteBar {
    int hour = 0, minute = 0;
    double price = 0, avg_price = 0;
    std::uint32_t volume = 0, open_interest = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExMinuteBar, hour, minute, price, avg_price, volume, open_interest);

struct ExTransaction {
    std::string date;
    int hour = 0, minute = 0, second = 0;
    std::uint32_t price = 0, volume = 0;
    std::int32_t zengcang = 0;
    std::uint16_t nature = 0;
    int nature_mark = 0, nature_value = 0, direction = 0;
    std::string nature_name, natrue_name;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExTransaction, date, hour, minute, second, price, volume, zengcang,
    nature, nature_mark, nature_value, direction, nature_name, natrue_name);

struct ExBar {
    double open = 0, high = 0, low = 0, close = 0;
    std::uint32_t position = 0, trade = 0;
    double price = 0, amount = 0;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    std::string datetime;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExBar, open, high, low, close, position, trade, price, amount,
    year, month, day, hour, minute, datetime);

struct ExRangeBar {
    double open = 0, high = 0, low = 0, close = 0;
    std::uint32_t position = 0, trade = 0;
    double settlementprice = 0;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    std::string datetime;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExRangeBar, open, high, low, close, position, trade,
    settlementprice, year, month, day, hour, minute, datetime);

class TdxExHqApi {
public:
    virtual ~TdxExHqApi() = default;
    virtual bool connect(const std::string& host, const std::string& port) = 0;
    virtual bool connected() const = 0;
    virtual std::vector<ExMarket> get_markets() = 0;
    virtual std::size_t get_instrument_count() = 0;
    virtual std::vector<ExInstrument> get_instrument_info(unsigned int start, unsigned short count) = 0;
    virtual std::vector<ExQuote> get_instrument_quote(unsigned char market, const std::string& code) = 0;
    virtual std::vector<nlohmann::json> get_instrument_quote_list(unsigned char market, unsigned char category,
                                                                    unsigned short start, unsigned short count) = 0;
    virtual std::vector<ExBar> get_instrument_bars(unsigned char category, unsigned char market,
                                                    const std::string& code, std::uint32_t start,
                                                    unsigned short count) = 0;
    virtual std::vector<ExRangeBar> get_history_instrument_bars_range(unsigned char market,
        const std::string& code, std::uint32_t start_date, std::uint32_t end_date) = 0;
    virtual std::vector<ExMinuteBar> get_minute_time_data(unsigned char market, const std::string& code) = 0;
    virtual std::vector<ExMinuteBar> get_history_minute_time_data(unsigned char market,
        const std::string& code, std::uint32_t date) = 0;
    virtual std::vector<ExTransaction> get_transaction_data(unsigned char market, const std::string& code,
        std::int32_t start, unsigned short count) = 0;
    virtual std::vector<ExTransaction> get_history_transaction_data(unsigned char market,
        const std::string& code, std::uint32_t date, std::int32_t start, unsigned short count) = 0;
};

std::shared_ptr<TdxExHqApi> create_tdx_ex_hq_api();

}  // namespace cpptdx
