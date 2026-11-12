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
    if (!data || len < static_cast<int>(sizeof(std::uint16_t))) {
        return {};
    }
    std::vector<Kline> klines;
    int pos = 0;

    unsigned short ret_count = 0;
    memcpy(&ret_count, data + pos, sizeof(ret_count));
    pos += sizeof(ret_count);
    constexpr int minimum_row_size = 16;
    if (ret_count > static_cast<unsigned int>((len - pos) / minimum_row_size)) return {};

    int pre_diff_base = 0;
    for (int i = 0; i < ret_count; ++i) {
        if (pos + 4 > len) return {};
        Kline kline;
        int year, month, day, hour, minute;
        std::tie(year, month, day, hour, minute, pos) = get_datetime(category, data, pos);
        kline.year = year;
        kline.month = month;
        kline.day = day;
        kline.hour = hour;
        kline.minute = minute;

        int price_open_diff;
        int price_close_diff;
        int price_high_diff;
        int price_low_diff;
        auto read_price = [&](int& value) {
            if (pos >= len) return false;
            const auto first = static_cast<unsigned char>(data[pos++]);
            value = first & 0x3f;
            const bool negative = (first & 0x40) != 0;
            unsigned shift = 6;
            auto byte = first;
            while (byte & 0x80) {
                if (pos >= len || shift >= 31) return false;
                byte = static_cast<unsigned char>(data[pos++]);
                value |= static_cast<int>(byte & 0x7f) << shift;
                shift += 7;
            }
            if (negative) value = -value;
            return true;
        };
        if (!read_price(price_open_diff) || !read_price(price_close_diff) ||
            !read_price(price_high_diff) || !read_price(price_low_diff)) return {};

        if (pos + 8 > len) return {};
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
