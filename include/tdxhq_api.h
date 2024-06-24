/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once
#include <memory>
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
    long long vol = 0;
    long long amount = 0;
    long long datetime = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Kline, open, close,high,low,vol,amount,datetime);

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

struct SecurityInfo {
    string code;
    unsigned short volunit = 0;
    string name;
    char decimal_point = 0;
    double pre_close = 0;
};


struct FinanceInfo {
    string code;
    Market market;
    float liutongguben;
    unsigned short province;
    unsigned short industry;
    unsigned int updated_date;
    unsigned int ipo_date;
    float zongguben;
    float guojiagu;
    float faqirenfarengu;
    float farengu;
    float bgu;
    float hgu;
    float zhigonggu;
    float zongzichan;
    float liudongzichan;
    float gudingzichan;
    float wuxingzichan;
    float gudongrenshu;
    float liudongfuzhai;
    float changqifuzhai;
    float zibengongjijin;
    float jingzichan;
    float zhuyingshouru;
    float zhuyinglirun;
    float yingshouzhangkuan;
    float yingyelirun;
    float touzishouyu;
    float jingyingxianjinliu;
    float zongxianjinliu;
    float cunhuo;
    float lirunzonghe;
    float shuihoulirun;
    float jinglirun;
    float weifenlirun;
    float baoliu1;
    float baoliu2;
};

class TdxHqApi {
public:
    TdxHqApi(){};
    virtual ~TdxHqApi(){};
    virtual bool connect(const string& host, const string& port) = 0;
    virtual vector<Kline> get_security_klines(Category cat, Market market, const std::string& code, unsigned short start, unsigned short count) = 0;
    virtual vector<SnapShot> get_security_snapshots(const vector<pair<string, Market>>& stock_list) = 0;
    virtual vector<SecurityInfo> get_security_list(Market market, unsigned short start) = 0;
    virtual FinanceInfo get_finance_info(Market market, const string& code) = 0;
    virtual size_t get_security_count(Market market) = 0;
};
shared_ptr<TdxHqApi> create_tdx_hq_api();
}  // namespace cpptdx