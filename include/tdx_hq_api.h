/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once
#include <memory>
#include <string>
#include <vector>

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
// OrderedDict([
//                 ("market", market),
//                 ("code", code.decode("utf-8")),
//                 ("active1", active1),
//                 ("price", self._cal_price(price, 0)),
//                 ("last_close", self._cal_price(price, last_close_diff)),
//                 ("open", self._cal_price(price, open_diff)),
//                 ("high", self._cal_price(price, high_diff)),
//                 ("low", self._cal_price(price, low_diff)),
//                 ("servertime", self._format_time('%s' % reversed_bytes0)),
//                 ("reversed_bytes0", reversed_bytes0),
//                 ("reversed_bytes1", reversed_bytes1),
//                 ("vol", vol),
//                 ("cur_vol", cur_vol),
//                 ("amount", amount),
//                 ("s_vol", s_vol),
//                 ("b_vol", b_vol),
//                 ("reversed_bytes2", reversed_bytes2),
//                 ("reversed_bytes3", reversed_bytes3),
//                 ("bid1", self._cal_price(price, bid1)),
//                 ("ask1", self._cal_price(price, ask1)),
//                 ("bid_vol1", bid_vol1),
//                 ("ask_vol1", ask_vol1),
//                 ("bid2", self._cal_price(price, bid2)),
//                 ("ask2", self._cal_price(price, ask2)),
//                 ("bid_vol2", bid_vol2),
//                 ("ask_vol2", ask_vol2),
//                 ("bid3", self._cal_price(price, bid3)),
//                 ("ask3", self._cal_price(price, ask3)),
//                 ("bid_vol3", bid_vol3),
//                 ("ask_vol3", ask_vol3),
//                 ("bid4", self._cal_price(price, bid4)),
//                 ("ask4", self._cal_price(price, ask4)),
//                 ("bid_vol4", bid_vol4),
//                 ("ask_vol4", ask_vol4),
//                 ("bid5", self._cal_price(price, bid5)),
//                 ("ask5", self._cal_price(price, ask5)),
//                 ("bid_vol5", bid_vol5),
//                 ("ask_vol5", ask_vol5),
//                 ("reversed_bytes4", reversed_bytes4),
//                 ("reversed_bytes5", reversed_bytes5),
//                 ("reversed_bytes6", reversed_bytes6),
//                 ("reversed_bytes7", reversed_bytes7),
//                 ("reversed_bytes8", reversed_bytes8),
//                 ("reversed_bytes9", reversed_bytes9/100.0),  # 涨速
//                 ("active2", active2)
//             ])
struct SnapShot {
    Market market;
    string code;
    double active1;
    double price;
    double last_close;
    double open;
    double high;
    double low;
    long long datetime;
    long long reversed_bytes0;
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
    long long reversed_bytes9;
    double active2;
};


class TdxHqApi {
public:
    TdxHqApi(){};
    virtual ~TdxHqApi(){};
    virtual bool connect(const string& host, const string& port) = 0;
    virtual vector<Kline> get_security_klines(Category cat, Market market, const std::string& code, unsigned short start, unsigned short count) = 0;
    virtual size_t get_security_count(Market market) = 0;
};
shared_ptr<TdxHqApi> create_tdx_hq_api();
}  // namespace cpptdx