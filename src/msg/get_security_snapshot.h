/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "base_msg.h"
#include "include/tdxhq_api.h"

namespace cpptdx {
//     stock_len = len(all_stock)
// if stock_len <= 0:
//     return False

// pkgdatalen = stock_len * 7 + 12

// values = (
//     0x10c,
//     0x02006320,
//     pkgdatalen,
//     pkgdatalen,
//     0x5053e,
//     0,
//     0,
//     stock_len,
// )

// pkg_header = struct.pack("<HIHHIIHH", *values)
// pkg = bytearray(pkg_header)
// for stock in all_stock:
//     market, code = stock
//     if type(code) is six.text_type:
//         code = code.encode("utf-8")
//     one_stock_pkg = struct.pack("<B6s", market, code)
//     pkg.extend(one_stock_pkg)

// self.send_pkg = pkg

using namespace std;
inline string make_get_snap_request(const vector<pair<string, Market>>& stock_list) {
    unsigned short h1 = 0x10c;
    unsigned int h2 = 0x02006320;
    unsigned int h3 = 0x5053e;
    unsigned int paddle = 0;
    unsigned short stock_len = stock_list.size();
    unsigned short pkgdatalen = stock_len * 7 + 12;
    string req_msg;
    req_msg.append((char*)&h1, 2);
    req_msg.append((char*)&h2, 4);
    req_msg.append((char*)&pkgdatalen, 2);
    req_msg.append((char*)&pkgdatalen, 2);
    req_msg.append((char*)&h3, 4);
    req_msg.append((char*)&paddle, 4);
    req_msg.append((char*)&paddle, 2);
    req_msg.append((char*)&stock_len, 2);
    for (const auto& stock : stock_list) {
        unsigned char market = (unsigned short)stock.second;
        req_msg.append(1, market);
        req_msg.append(stock.first.c_str(), stock.first.size());
    }

    return req_msg;
}

long long format_time(int time_stamp) {
    int hour = time_stamp/1000000;
    int min = 0;
    long long sec = 0;
    int vir_value = (time_stamp/10000)%100;
    if(vir_value < 60){
        min = (time_stamp/10000)%100;
       sec=  (time_stamp %10000) * 60 / 10000.0;
    }else{
        min = (time_stamp%1000000) *60/1000000;
        sec= ((time_stamp%1000000) *60%1000000) * 60 / 1000;
    }
    long long ret = hour*10000000 + min*100000 +sec;
    return ret;
}

std::vector<SnapShot> parse_snapshot(const char *data, int len) {
    std::vector<SnapShot> snapshots;
    size_t pos = 0;
    pos += 2; // skip b1 cb

    unsigned short num_stock = *(unsigned short *)(data+pos);
    pos += 2;
    snapshots.reserve(num_stock);
    for (size_t i = 0; i < num_stock; ++i) {
        char market = *(char *)(data+pos);
        pos += 1;
        char code[6];
        memcpy(code, data+pos, 6);
        pos += 6;
        unsigned short active1 = *(unsigned short *)(data+pos);
        pos += 2;

        int price = 0;
        int last_close_diff = 0;
        int open_diff = 0;
        int high_diff = 0;
        int low_diff = 0;
        int reversed_bytes0 = 0;
        int reversed_bytes1 = 0;
        int vol = 0;
        int cur_vol = 0;
        std::tie(price,pos) = get_price(data, pos);
        std::tie(last_close_diff, pos) = get_price(data, pos);
        std::tie(open_diff, pos) = get_price(data, pos);
        std::tie(high_diff, pos) = get_price(data, pos);
        std::tie(low_diff, pos) = get_price(data, pos);
        std::tie(reversed_bytes0, pos) = get_price(data, pos);
        std::tie(reversed_bytes1, pos) = get_price(data, pos);
        std::tie(vol, pos) = get_price(data, pos);
        std::tie(cur_vol, pos) = get_price(data, pos);
        int amout_raw = 0;
        std::memcpy(&amout_raw, data+pos, 4);
        double amount = get_volume(amout_raw);
        pos += 4;

        int s_vol = 0;
        int b_vol = 0;
        int reversed_bytes2 = 0;
        int reversed_bytes3 = 0;
        int bid1 = 0;
        int ask1 = 0;
        int bid_vol1 = 0;
        int ask_vol1 = 0;
        int bid2 = 0;
        int ask2 = 0;
        int bid_vol2 = 0;
        int ask_vol2 = 0;
        int bid3 = 0;
        int ask3 = 0;
        int bid_vol3 = 0;
        int ask_vol3 = 0;
        int bid4 = 0;
        int ask4 = 0;
        int bid_vol4 = 0;
        int ask_vol4 = 0;
        int bid5 = 0;
        int ask5 = 0;
        int bid_vol5 = 0;
        int ask_vol5 = 0;

        int reversed_bytes5 = 0;
        int reversed_bytes6 = 0;
        int reversed_bytes7 = 0;
        int reversed_bytes8 = 0;

        std::tie(s_vol, pos) = get_price(data, pos);
        std::tie(b_vol, pos) = get_price(data, pos);
        std::tie(reversed_bytes2, pos) = get_price(data, pos);
        std::tie(reversed_bytes3, pos) = get_price(data, pos);

        std::tie(bid1, pos) = get_price(data, pos);
        std::tie(ask1, pos) = get_price(data, pos);
        std::tie(bid_vol1, pos) = get_price(data, pos);
        std::tie(ask_vol1, pos) = get_price(data, pos);

        std::tie(bid2, pos) = get_price(data, pos);
        std::tie(ask2, pos) = get_price(data, pos);
        std::tie(bid_vol2, pos) = get_price(data, pos);
        std::tie(ask_vol2, pos) = get_price(data, pos);

        std::tie(bid3, pos) = get_price(data, pos);
        std::tie(ask3, pos) = get_price(data, pos);
        std::tie(bid_vol3, pos) = get_price(data, pos);
        std::tie(ask_vol3, pos) = get_price(data, pos);

        std::tie(bid4, pos) = get_price(data, pos);
        std::tie(ask4, pos) = get_price(data, pos);
        std::tie(bid_vol4, pos) = get_price(data, pos);
        std::tie(ask_vol4, pos) = get_price(data, pos);

        std::tie(bid5, pos) = get_price(data, pos);
        std::tie(ask5, pos) = get_price(data, pos);
        std::tie(bid_vol5, pos) = get_price(data, pos);
        std::tie(ask_vol5, pos) = get_price(data, pos);
        unsigned short reversed_bytes4 = 0;
        std::memcpy(&reversed_bytes4, data+pos, 2);
        pos += 2;
        std::tie(reversed_bytes5, pos) = get_price(data, pos);
        std::tie(reversed_bytes6, pos) = get_price(data, pos);
        std::tie(reversed_bytes7, pos) = get_price(data, pos);
        std::tie(reversed_bytes8, pos) = get_price(data, pos);
        short reversed_bytes9 = 0;
        unsigned short active2 = 0;
        std::memcpy(&reversed_bytes9, data+pos, 2);
        pos += 2;
        std::memcpy(&active2, data+pos, 2);
        pos += 2;

        SnapShot snap_shot;
        snap_shot.market = (Market)market;
        snap_shot.code = code;
        snap_shot.active1 = active1;
        snap_shot.price = _cal_price100(price,0);
        snap_shot.last_close = _cal_price100(price,last_close_diff);
        snap_shot.open = _cal_price100(price,open_diff);
        snap_shot.high = _cal_price100(price,high_diff);
        snap_shot.low = _cal_price100(price,low_diff);
        snap_shot.datetime = format_time(reversed_bytes0);
        snap_shot.reversed_bytes0 = reversed_bytes0;
        snap_shot.reversed_bytes1 = reversed_bytes1;
        snap_shot.vol = vol;
        snap_shot.cur_vol = cur_vol;
        snap_shot.amount = amount;
        snap_shot.s_vol = s_vol;
        snap_shot.b_vol = b_vol;
        snap_shot.reversed_bytes2 = reversed_bytes2;
        snap_shot.reversed_bytes3 = reversed_bytes3;
        snap_shot.bid1 = _cal_price100(price,bid1);
        snap_shot.ask1 = _cal_price100(price,ask1);
        snap_shot.bid_vol1 = bid_vol1;
        snap_shot.ask_vol1 = ask_vol1;
        snap_shot.bid2 = _cal_price100(price,bid2);
        snap_shot.ask2 = _cal_price100(price,ask2);
        snap_shot.bid_vol2 = bid_vol2;
        snap_shot.ask_vol2 = ask_vol2;
        snap_shot.bid3 = _cal_price100(price,bid3);
        snap_shot.ask3 = _cal_price100(price,ask3);
        snap_shot.bid_vol3 = bid_vol3;
        snap_shot.ask_vol3 = ask_vol3;
        snap_shot.bid4 = _cal_price100(price,bid4);
        snap_shot.ask4 = _cal_price100(price,ask4);
        snap_shot.bid_vol4 = bid_vol4;
        snap_shot.ask_vol4 = ask_vol4;
        snap_shot.bid5 = _cal_price100(price,bid5);
        snap_shot.ask5 = _cal_price100(price,ask5);
        snap_shot.bid_vol5 = bid_vol5;
        snap_shot.ask_vol5 = ask_vol5;
        snap_shot.reversed_bytes4 = reversed_bytes4;
        snap_shot.reversed_bytes5 = reversed_bytes5;
        snap_shot.reversed_bytes6 = reversed_bytes6;
        snap_shot.reversed_bytes7 = reversed_bytes7;
        snap_shot.reversed_bytes8 = reversed_bytes8;
        snap_shot.reversed_bytes9 = reversed_bytes9/100.0;
        snap_shot.active2 = active2;
        snapshots.emplace_back(snap_shot);
    }
    return snapshots;
}
}  // namespace cpptdx