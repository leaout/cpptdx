/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once
#include <memory>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>
#include "json/json.hpp"

namespace cpptdx {
using namespace std;

struct Kline {
    double open = 0;
    double close = 0;
    double high = 0;
    double low = 0;
    double vol = 0;
    double amount = 0;
    long long datetime = 0;
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
};
inline void to_json(nlohmann::json& j, const Kline& k) {
    const int year = k.year ? k.year : static_cast<int>(k.datetime / 100000000LL);
    const int month = k.month ? k.month : static_cast<int>((k.datetime / 1000000LL) % 100);
    const int day = k.day ? k.day : static_cast<int>((k.datetime / 10000LL) % 100);
    const int hour = k.hour ? k.hour : static_cast<int>((k.datetime / 100LL) % 100);
    const int minute = k.minute ? k.minute : static_cast<int>(k.datetime % 100);
    char date[24];
    std::snprintf(date, sizeof(date), "%04d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
    j = nlohmann::json{{"open", k.open}, {"close", k.close}, {"high", k.high}, {"low", k.low},
        {"vol", k.vol}, {"amount", k.amount}, {"year", year}, {"month", month}, {"day", day},
        {"hour", hour}, {"minute", minute}, {"datetime", date}};
}

struct MinuteRecord {
    double price = 0;
    long long vol = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MinuteRecord, price, vol);

struct TickRecord {
    std::string time;
    double price = 0;
    long long vol = 0;
    int num = 0;
    int buyorsell = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TickRecord, time, price, vol, num, buyorsell);

struct HistoryTickRecord {
    std::string time;
    double price = 0;
    long long vol = 0;
    int buyorsell = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HistoryTickRecord, time, price, vol, buyorsell);

struct CompanyInfoFile {
    std::string name;
    std::string filename;
    std::uint32_t start = 0;
    std::uint32_t length = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompanyInfoFile, name, filename, start, length);

struct XdxrRecord {
    int year = 0;
    int month = 0;
    int day = 0;
    int category = 0;
    std::string name;
    std::optional<double> fenhong, peigujia, songzhuangu, peigu, suogu;
    std::optional<double> panqianliutong, panhouliutong, qianzongguben, houzongguben;
    std::optional<double> fenshu, xingquanjia;
};

inline void to_json(nlohmann::json& j, const XdxrRecord& row) {
    j = nlohmann::json{{"year", row.year}, {"month", row.month}, {"day", row.day},
        {"category", row.category}, {"name", row.name},
        {"fenhong", row.fenhong ? nlohmann::json(*row.fenhong) : nlohmann::json(nullptr)},
        {"peigujia", row.peigujia ? nlohmann::json(*row.peigujia) : nlohmann::json(nullptr)},
        {"songzhuangu", row.songzhuangu ? nlohmann::json(*row.songzhuangu) : nlohmann::json(nullptr)},
        {"peigu", row.peigu ? nlohmann::json(*row.peigu) : nlohmann::json(nullptr)},
        {"suogu", row.suogu ? nlohmann::json(*row.suogu) : nlohmann::json(nullptr)},
        {"panqianliutong", row.panqianliutong ? nlohmann::json(*row.panqianliutong) : nlohmann::json(nullptr)},
        {"panhouliutong", row.panhouliutong ? nlohmann::json(*row.panhouliutong) : nlohmann::json(nullptr)},
        {"qianzongguben", row.qianzongguben ? nlohmann::json(*row.qianzongguben) : nlohmann::json(nullptr)},
        {"houzongguben", row.houzongguben ? nlohmann::json(*row.houzongguben) : nlohmann::json(nullptr)},
        {"fenshu", row.fenshu ? nlohmann::json(*row.fenshu) : nlohmann::json(nullptr)},
        {"xingquanjia", row.xingquanjia ? nlohmann::json(*row.xingquanjia) : nlohmann::json(nullptr)}};
}

// 0 -   5 分钟K 线
// 1 -   15 分钟K 线
// 2 -   30 分钟K 线
// 3 -   1 小时K 线
// 4 -   日K 线
// 5 -   周K 线
// 6 -   月K 线
// 7 -   1 分钟
// 8 -   1 分钟K 线
// 9 -   日K 线
// 10 -  季K 线
// 11 -  年K 线
enum class Category : unsigned short {
    k5Minute = 0,
    k15Minute = 1,
    k30Minute = 2,
    k1Hour = 3,
    kDay = 4,
    kWeek = 5,
    kMonth = 6,
    k1Minute = 7,
    k1MinuteK = 8,
    kDayK = 9,
    kQuarterK = 10,
    kYearK = 11,
};
// 0  # 深圳
// 1  # 上海
enum class Market : unsigned short { kSZ = 0, kSH = 1 };

struct SnapShot {
    Market market;
    string code;
    unsigned short active1;
    double price;
    double last_close;
    double open;
    double high;
    double low;
    long long datetime;
    long long reversed_bytes0;
    long long reversed_bytes1;
    long long vol;
    long long cur_vol;
    long long amount;
    long long s_vol;
    long long b_vol;
    long long reversed_bytes2;
    long long reversed_bytes3;
    double bid1;
    double ask1;
    long long bid_vol1;
    long long ask_vol1;
    double bid2;
    double ask2;
    long long bid_vol2;
    long long ask_vol2;
    double bid3;
    double ask3;
    long long bid_vol3;
    long long ask_vol3;
    double bid4;
    double ask4;
    long long bid_vol4;
    long long ask_vol4;
    double bid5;
    double ask5;
    long long bid_vol5;
    long long ask_vol5;
    long long reversed_bytes4;
    long long reversed_bytes5;
    long long reversed_bytes6;
    long long reversed_bytes7;
    long long reversed_bytes8;
    double reversed_bytes9;
    double active2;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SnapShot,market,code,active1,price,last_close,open,high,low,datetime,reversed_bytes0,reversed_bytes1,vol,cur_vol,amount,s_vol,b_vol,reversed_bytes2,reversed_bytes3,bid1,ask1,bid_vol1,ask_vol1,bid2,ask2,bid_vol2,ask_vol2,bid3,ask3,bid_vol3,ask_vol3,bid4,ask4,bid_vol4,ask_vol4,bid5,ask5,bid_vol5,ask_vol5,reversed_bytes4,reversed_bytes5,reversed_bytes6,reversed_bytes7,reversed_bytes8,reversed_bytes9,active2);

struct SecurityInfo {
    string code;
    unsigned short volunit = 0;
    string name;
    char decimal_point = 0;
    double pre_close = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SecurityInfo, code, volunit,name,decimal_point,pre_close);

struct FinanceInfo {
    string code;
    Market market = Market::kSZ;
    float liutongguben = 0;
    unsigned short province = 0;
    unsigned short industry = 0;
    unsigned int updated_date = 0;
    unsigned int ipo_date = 0;
    float zongguben = 0;
    float guojiagu = 0;
    float faqirenfarengu = 0;
    float farengu = 0;
    float bgu = 0;
    float hgu = 0;
    float zhigonggu = 0;
    float zongzichan = 0;
    float liudongzichan = 0;
    float gudingzichan = 0;
    float wuxingzichan = 0;
    float gudongrenshu = 0;
    float liudongfuzhai = 0;
    float changqifuzhai = 0;
    float zibengongjijin = 0;
    float jingzichan = 0;
    float zhuyingshouru = 0;
    float zhuyinglirun = 0;
    float yingshouzhangkuan = 0;
    float yingyelirun = 0;
    float touzishouyu = 0;
    float jingyingxianjinliu = 0;
    float zongxianjinliu = 0;
    float cunhuo = 0;
    float lirunzonghe = 0;
    float shuihoulirun = 0;
    float jinglirun = 0;
    float weifenlirun = 0;
    float baoliu1 = 0;
    float baoliu2 = 0;
};

struct BlockInfoMeta {
    std::uint32_t size = 0;
    std::string hash_value;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BlockInfoMeta, size, hash_value);

struct BlockConstituent {
    std::string blockname;
    std::uint16_t block_type = 0;
    std::uint16_t code_index = 0;
    std::string code;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BlockConstituent, blockname, block_type, code_index, code);

class TdxHqApi {
public:
    TdxHqApi(){};
    virtual ~TdxHqApi(){};
    virtual bool connect(const string& host, const string& port) = 0;
    virtual vector<Kline> get_security_klines(Category cat, Market market, const std::string& code, unsigned short start, unsigned short count) = 0;
    virtual vector<Kline> get_security_bars(Category cat, Market market, const std::string& code,
                                            unsigned short start, unsigned short count) {
        return get_security_klines(cat, market, code, start, count);
    }
    virtual vector<Kline> get_index_klines(Category cat, Market market, const std::string& code,
                                           unsigned short start, unsigned short count) {
        return get_security_klines(cat, market, code, start, count);
    }
    virtual vector<Kline> get_index_bars(Category cat, Market market, const std::string& code,
                                         unsigned short start, unsigned short count) {
        return get_index_klines(cat, market, code, start, count);
    }
    virtual vector<MinuteRecord> get_minute_time_data(Market, const std::string&) { return {}; }
    virtual vector<MinuteRecord> get_history_minute_time_data(Market, const std::string&, unsigned int) { return {}; }
    virtual vector<TickRecord> get_transaction_data(Market, const std::string&, unsigned short, unsigned short) { return {}; }
    virtual vector<HistoryTickRecord> get_history_transaction_data(Market, const std::string&, unsigned short, unsigned short, unsigned int) { return {}; }
    virtual vector<CompanyInfoFile> get_company_info_category(Market, const std::string&) { return {}; }
    virtual std::string get_company_info_content(Market, const std::string&, const std::string&,
                                                 std::uint32_t, std::uint32_t) { return {}; }
    virtual vector<XdxrRecord> get_xdxr_info(Market, const std::string&) { return {}; }
    virtual vector<SnapShot> get_security_snapshots(const vector<pair<string, Market>>& stock_list) = 0;
    virtual vector<SnapShot> get_security_quotes(const vector<pair<string, Market>>& stock_list) {
        return get_security_snapshots(stock_list);
    }
    virtual vector<SnapShot> get_security_quotes(Market market, const std::string& code) {
        return get_security_quotes({{code, market}});
    }
    virtual vector<SecurityInfo> get_security_list(Market market, unsigned short start) = 0;
    virtual FinanceInfo get_finance_info(Market market, const string& code) = 0;
    virtual size_t get_security_count(Market market) = 0;
    virtual BlockInfoMeta get_block_info_meta(const std::string& blockfile) = 0;
    virtual std::string get_block_info(const std::string& blockfile, std::uint32_t start,
                                       std::uint32_t size) = 0;
    virtual std::vector<BlockConstituent> get_and_parse_block_info(const std::string& blockfile) = 0;

    // pytdx convenience helper: collect daily bars and filter them inclusively
    // by YYYY-MM-DD (or YYYYMMDD) bounds. This is intentionally implemented in
    // terms of the protocol-level get_security_bars method.
    std::vector<Kline> get_k_data(const std::string& code, const std::string& start_date,
                                  const std::string& end_date) {
        auto parse_date = [](std::string text, std::uint32_t& value) {
            text.erase(std::remove(text.begin(), text.end(), '-'), text.end());
            if (text.size() != 8 || text.find_first_not_of("0123456789") != std::string::npos) return false;
            value = static_cast<std::uint32_t>(std::strtoul(text.c_str(), nullptr, 10));
            return true;
        };
        std::uint32_t first = 0, last = 0;
        if (code.empty() || !parse_date(start_date, first) || !parse_date(end_date, last) || first > last) return {};

        const bool sh = code.front() == '5' || code.front() == '6' || code.front() == '9' ||
            code.rfind("009", 0) == 0 || code.rfind("126", 0) == 0 || code.rfind("110", 0) == 0 ||
            code.rfind("201", 0) == 0 || code.rfind("202", 0) == 0 || code.rfind("203", 0) == 0 ||
            code.rfind("204", 0) == 0;
        const auto market = sh ? Market::kSH : Market::kSZ;
        std::vector<Kline> result;
        for (unsigned int start = 0; start < 8000; start += 800) {
            auto page = get_security_bars(Category::kDayK, market, code,
                                          static_cast<unsigned short>(start), 800);
            if (page.empty()) break;
            for (auto& row : page) {
                const auto date = static_cast<std::uint32_t>(row.year * 10000 + row.month * 100 + row.day);
                if (date >= first && date <= last) result.push_back(row);
            }
            if (page.size() < 800) break;
        }
        std::sort(result.begin(), result.end(), [](const Kline& a, const Kline& b) {
            return a.datetime < b.datetime;
        });
        return result;
    }
};
shared_ptr<TdxHqApi> create_tdx_hq_api();
}  // namespace cpptdx
