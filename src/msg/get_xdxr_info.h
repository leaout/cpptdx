#pragma once

#include "include/tdxhq_api.h"
#include "tdx/utils.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cpptdx {

inline std::string make_get_xdxr_info_request(Market market, const std::string& code) {
    std::string request("\x0c\x1f\x18\x76\x00\x01\x0b\x00\x0b\x00\x0f\x00\x01\x00", 14);
    request.push_back(static_cast<char>(market));
    char stock_code[6] = {};
    std::memcpy(stock_code, code.data(), std::min(code.size(), sizeof(stock_code)));
    request.append(stock_code, sizeof(stock_code));
    return request;
}

inline const char* xdxr_name(int category) {
    switch (category) {
        case 1: return "除权除息";
        case 2: return "送配股上市";
        case 3: return "非流通股上市";
        case 4: return "未知股本变动";
        case 5: return "股本变化";
        case 6: return "增发新股";
        case 7: return "股份回购";
        case 8: return "增发新股上市";
        case 9: return "转配股上市";
        case 10: return "可转债上市";
        case 11: return "扩缩股";
        case 12: return "非流通股缩股";
        case 13: return "送认购权证";
        case 14: return "送认沽权证";
        default: return nullptr;
    }
}

inline std::vector<XdxrRecord> parse_xdxr_info(const char* data, size_t size) {
    if (size < 11) return {};
    size_t pos = 9;
    uint16_t count = 0;
    std::memcpy(&count, data + pos, sizeof(count));
    pos += 2;
    if (size - pos < static_cast<size_t>(count) * 29) return {};
    std::vector<XdxrRecord> records;
    records.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        if (size - pos < 29) return {};
        pos += 8; // market/code and reserved byte
        int year, month, day, hour, minute;
        std::tie(year, month, day, hour, minute, pos) = get_datetime(9, data, static_cast<int>(pos));
        const unsigned char category = static_cast<unsigned char>(data[pos++]);
        XdxrRecord row;
        row.year = year; row.month = month; row.day = day; row.category = category;
        const auto* name = xdxr_name(category);
        row.name = name ? name : std::to_string(category);
        if (category == 1) {
            float values[4]; std::memcpy(values, data + pos, sizeof(values));
            row.fenhong = values[0]; row.peigujia = values[1]; row.songzhuangu = values[2]; row.peigu = values[3];
        } else if (category == 11 || category == 12) {
            float suogu = 0; std::memcpy(&suogu, data + pos + 8, sizeof(suogu)); row.suogu = suogu;
        } else if (category == 13 || category == 14) {
            float xingquanjia = 0, fenshu = 0;
            std::memcpy(&xingquanjia, data + pos, sizeof(xingquanjia));
            std::memcpy(&fenshu, data + pos + 8, sizeof(fenshu));
            row.xingquanjia = xingquanjia; row.fenshu = fenshu;
        } else {
            uint32_t values[4]; std::memcpy(values, data + pos, sizeof(values));
            auto decode = [](uint32_t v) { return v == 0 ? 0.0 : get_volume(static_cast<int>(v)); };
            row.panqianliutong = decode(values[0]); row.qianzongguben = decode(values[1]);
            row.panhouliutong = decode(values[2]); row.houzongguben = decode(values[3]);
        }
        pos += 16;
        records.push_back(std::move(row));
    }
    return records;
}

}  // namespace cpptdx
