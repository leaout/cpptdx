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
#include "include/tdx_hq_api.h"

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
std::string format_time(int time_stamp) {
    std::string time = std::to_string(time_stamp);
    if (time.length() > 6) {
        time = time.substr(0, time.length() - 6) + ":";
        if (time.length() > 4) {
            time += std::to_string(time.back() - '0');
            time += std::to_string((time.back() - '0') * 100 + (time.at(time.length() - 5) - '0'));
            time += std::to_string((time.back() - '0') * 10000 + (time.at(time.length() - 4) - '0') * 100 + (time.at(time.length() - 3) - '0'));
        } else {
            time += std::to_string((time.back() - '0') * 1000000 + (time.at(time.length() - 5) - '0') * 10000 + (time.at(time.length() - 4) - '0') * 100 + (time.at(time.length() - 3) - '0'));
            time += std::to_string((time.back() - '0') * 1000000 % 1000000);
        }
    }
    return time;
}
std::vector<std::map<std::string, std::string>> parseResponse(const char *data, int len) {
    std::vector<std::map<std::string, std::string>> stocks;
    size_t pos = 0;
    pos += 2; // skip b1 cb

    unsigned short num_stock = *(unsigned short *)(data+pos);
    pos += 2;

    for (size_t i = 0; i < num_stock; ++i) {
        char market = *(char *)(data+pos);
        pos += 1;
        char code[6];
        memcpy(code, data+pos, 6);
        pos += 6;
        unsigned short active1 = *(unsigned short *)(data+pos);
   
        pos += 2;
        // std::tie(price_open_diff, pos)
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

        // double amount = get_volume(body_buf.substr(pos, 4));
        // pos += 4;
        // double s_vol = get_price(body_buf, pos);
        // double b_vol = get_price(body_buf, pos);
        // double reversed_bytes2 = get_price(body_buf, pos);
        // double reversed_bytes3 = get_price(body_buf, pos);
        // double bid1 = get_price(body_buf, pos);
        // double ask1 = get_price(body_buf, pos);
        // double bid_vol1 = get_price(body_buf, pos);
        // double ask_vol1 = get_price(body_buf, pos);
        // double bid2 = get_price(body_buf, pos);
        // double ask2 = get_price(body_buf, pos);
        // double bid_vol2 = get_price(body_buf, pos);
        // double ask_vol2 = get_price(body_buf, pos);
        // double bid3 = get_price(body_buf, pos);
        // double ask3 = get_price(body_buf, pos);
        // double bid_vol3 = get_price(body_buf, pos);
        // double ask_vol3 = get_price(body_buf, pos);
        // double bid4 = get_price(body_buf, pos);
        // double ask4 = get_price(body_buf, pos);
        // double bid_vol4 = get_price(body_buf, pos);
        // double ask_vol4 = get_price(body_buf, pos);
        // double bid5 = get_price(body_buf, pos);
        // double ask5 = get_price(body_buf, pos);
        // double bid_vol5 = get_price(body_buf, pos);
        // double ask_vol5 = get_price(body_buf, pos);
        // double reversed_bytes4 = get_price(body_buf, pos);
        // double reversed_bytes5 = get_price(body_buf, pos);
        // double reversed_bytes6 = get_price(body_buf, pos);
        // double reversed_bytes7 = get_price(body_buf, pos);
        // double reversed_bytes8 = get_price(body_buf, pos);
        // double reversed_bytes9 = get_price(body_buf, pos);
        // double active2 = get_price(body_buf, pos);
    }
}
    // inline vector<> parse_kline(const char *data, int len, unsigned short category) {
    // if (len <= 0) {
    //     return {};
    // }

}  // namespace cpptdx