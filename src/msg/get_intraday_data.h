#pragma once

#include "include/tdxhq_api.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace cpptdx {

inline void append_fixed_code(std::string& request, const std::string& code) {
    char field[6] = {};
    std::memcpy(field, code.data(), std::min(code.size(), sizeof(field)));
    request.append(field, sizeof(field));
}

template <typename T>
inline void append_little_endian(std::string& request, T value) {
    request.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

inline std::string make_get_minute_time_request(Market market, const std::string& code) {
    std::string request("\x0c\x1b\x08\x00\x01\x01\x0e\x00\x0e\x00\x1d\x05", 12);
    append_little_endian(request, static_cast<uint16_t>(market));
    append_fixed_code(request, code);
    append_little_endian(request, static_cast<uint32_t>(0));
    return request;
}

inline std::string make_get_history_minute_time_request(Market market, const std::string& code, uint32_t date) {
    std::string request("\x0c\x01\x30\x00\x01\x01\x0d\x00\x0d\x00\xb4\x0f", 12);
    append_little_endian(request, date);
    append_little_endian(request, static_cast<uint8_t>(market));
    append_fixed_code(request, code);
    return request;
}

inline std::string make_get_transaction_request(Market market, const std::string& code,
                                                uint16_t start, uint16_t count) {
    std::string request("\x0c\x17\x08\x01\x01\x01\x0e\x00\x0e\x00\xc5\x0f", 12);
    append_little_endian(request, static_cast<uint16_t>(market));
    append_fixed_code(request, code);
    append_little_endian(request, start);
    append_little_endian(request, count);
    return request;
}

inline std::string make_get_history_transaction_request(Market market, const std::string& code,
                                                       uint16_t start, uint16_t count, uint32_t date) {
    std::string request("\x0c\x01\x30\x01\x00\x01\x12\x00\x12\x00\xb5\x0f", 12);
    append_little_endian(request, date);
    append_little_endian(request, static_cast<uint16_t>(market));
    append_fixed_code(request, code);
    append_little_endian(request, start);
    append_little_endian(request, count);
    return request;
}

inline bool read_delta(const char* data, size_t size, size_t& pos, int& value) {
    if (pos >= size) return false;
    const unsigned char first = static_cast<unsigned char>(data[pos++]);
    value = first & 0x3f;
    const bool negative = (first & 0x40) != 0;
    unsigned shift = 6;
    unsigned char byte = first;
    while ((byte & 0x80) != 0) {
        if (pos >= size || shift >= 31) return false;
        byte = static_cast<unsigned char>(data[pos++]);
        value |= static_cast<int>(byte & 0x7f) << shift;
        shift += 7;
    }
    if (negative) value = -value;
    return true;
}

inline bool read_minute_of_day(const char* data, size_t size, size_t& pos, int& hour, int& minute) {
    if (pos + 2 > size) return false;
    uint16_t minutes = 0;
    std::memcpy(&minutes, data + pos, sizeof(minutes));
    pos += 2;
    hour = minutes / 60;
    minute = minutes % 60;
    return hour < 24;
}

inline std::string format_hhmm(int hour, int minute) {
    std::ostringstream out;
    out << std::setfill('0') << std::setw(2) << hour << ':' << std::setw(2) << minute;
    return out.str();
}

inline std::vector<MinuteRecord> parse_minute_time_data(const char* data, size_t size, size_t header_skip) {
    if (size < header_skip || header_skip < 2) return {};
    uint16_t count = 0;
    std::memcpy(&count, data, sizeof(count));
    size_t pos = header_skip;
    std::vector<MinuteRecord> records;
    records.reserve(count);
    int last_price = 0;
    for (uint16_t i = 0; i < count; ++i) {
        int price_delta = 0, ignored = 0, volume = 0;
        if (!read_delta(data, size, pos, price_delta) || !read_delta(data, size, pos, ignored) ||
            !read_delta(data, size, pos, volume)) return {};
        last_price += price_delta;
        records.push_back({static_cast<double>(last_price) / 100.0, volume});
    }
    return records;
}

inline std::vector<TickRecord> parse_transaction_data(const char* data, size_t size, bool historical) {
    const size_t header_skip = historical ? 6 : 2;
    if (size < header_skip) return {};
    uint16_t count = 0;
    std::memcpy(&count, data, sizeof(count));
    size_t pos = header_skip;
    std::vector<TickRecord> records;
    records.reserve(count);
    int last_price = 0;
    for (uint16_t i = 0; i < count; ++i) {
        int hour = 0, minute = 0;
        if (!read_minute_of_day(data, size, pos, hour, minute)) return {};
        int price_delta = 0, volume = 0, num = 0, side = 0, ignored = 0;
        if (!read_delta(data, size, pos, price_delta) || !read_delta(data, size, pos, volume) ||
            (!historical && !read_delta(data, size, pos, num)) || !read_delta(data, size, pos, side) ||
            !read_delta(data, size, pos, ignored)) return {};
        last_price += price_delta;
        records.push_back({format_hhmm(hour, minute), static_cast<double>(last_price) / 100.0,
                           volume, num, side});
    }
    return records;
}

inline std::vector<HistoryTickRecord> to_history_ticks(const std::vector<TickRecord>& records) {
    std::vector<HistoryTickRecord> result;
    result.reserve(records.size());
    for (const auto& record : records) {
        result.push_back({record.time, record.price, record.vol, record.buyorsell});
    }
    return result;
}

}  // namespace cpptdx
