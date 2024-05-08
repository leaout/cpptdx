/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once
#include "base_msg.h"
#include <string>
#include <cstring>
#include <vector>
#include "include/tdxhq_api.h"

namespace cpptdx{
    using namespace std;
    inline string make_get_bars_request(unsigned short category, unsigned short market, 
                                    const std::string& code, unsigned short start, unsigned short count) {
        unsigned short h1 = 0x10c;
        unsigned int h2 = 0x01016408;
        unsigned short h3 = 0x1c;
        unsigned short h4 = 0x1c;
        unsigned short h5 = 0x052d;
        unsigned int paddle = 0;
        unsigned short paddle1 = 1;
        string req_msg;
        req_msg.append((char*)&h1, 2);
        req_msg.append((char*)&h2, 4);
        req_msg.append((char*)&h3, 2);
        req_msg.append((char*)&h4, 2);
        req_msg.append((char*)&h5, 2);
        req_msg.append((char*)&market, 2);
        req_msg.append(code.c_str(), code.size());
        req_msg.append((char*)&category, 2);
        req_msg.append((char*)&paddle1, 2);
        req_msg.append((char*)&start, 2);
        req_msg.append((char*)&count, 2);
        req_msg.append((char*)&paddle, 4);
        req_msg.append((char*)&paddle, 4);
        req_msg.append((char*)&paddle, 2);

        return req_msg;
    }

    inline std::vector<Kline> parse_kline(const char *data, int len, unsigned short category) {
    if (len <= 0) {
        return {};
    }
    std::vector<Kline> klines;
    int pos = 0;

    unsigned short ret_count;
    memcpy(&ret_count, data + pos, sizeof(ret_count));
    pos += sizeof(ret_count);

    int pre_diff_base = 0;
    for (int i = 0; i < ret_count; ++i) {
        Kline kline;
        int year, month, day, hour, minute;
        std::tie(year, month, day, hour, minute, pos) = get_datetime(category, data, pos);

        int price_open_diff;
        int price_close_diff;
        int price_high_diff;
        int price_low_diff;
        std::tie(price_open_diff, pos) = get_price(data, pos);
        std::tie(price_close_diff, pos) = get_price(data, pos);
        std::tie(price_high_diff, pos) = get_price(data, pos);
        std::tie(price_low_diff, pos) = get_price(data, pos);

        int vol_raw;
        memcpy(&vol_raw, data + pos, sizeof(vol_raw));
        kline.vol = get_volume(vol_raw);
        pos += 4;

        int dbvol_raw;
        memcpy(&dbvol_raw, data + pos, 4);
        kline.amount = get_volume(dbvol_raw);
        pos += 4;

        kline.open = _cal_price1000(price_open_diff, pre_diff_base);
        price_open_diff = price_open_diff + pre_diff_base;
        kline.close = _cal_price1000(price_open_diff, price_close_diff);
        kline.high = _cal_price1000(price_open_diff, price_high_diff);
        kline.low = _cal_price1000(price_open_diff, price_low_diff);

        pre_diff_base = price_open_diff + price_close_diff;
        std::vector<int> time_vec = {minute, hour, day, month, year};
        long long ff = 1;
        for (auto i : time_vec) {
            kline.datetime += ff * i;
            ff *= 100;
        }

        klines.emplace_back(kline);
    }

    return klines;
}
}