#pragma once

#include "include/tdxexhq_api.h"
#include "tdx/utils.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cpptdx {

inline std::vector<char> make_ex_get_markets_request() {
    return {0x01, 0x02, 0x48, 0x69, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, static_cast<char>(0xf4), 0x23};
}

inline std::vector<char> make_ex_get_instrument_count_request() {
    return {0x01, 0x03, 0x48, 0x66, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, static_cast<char>(0xf0), 0x23};
}

inline std::vector<char> make_ex_get_instrument_info_request(uint32_t start, uint16_t count) {
    std::vector<char> request{0x01, 0x04, 0x48, 0x67, 0x00, 0x01, 0x08, 0x00, 0x08, 0x00, static_cast<char>(0xf5), 0x23};
    request.insert(request.end(), reinterpret_cast<const char*>(&start), reinterpret_cast<const char*>(&start) + sizeof(start));
    request.insert(request.end(), reinterpret_cast<const char*>(&count), reinterpret_cast<const char*>(&count) + sizeof(count));
    return request;
}

inline std::vector<ExMarket> parse_ex_markets(const char* data, size_t size) {
    if (size < 2) return {};
    uint16_t count = 0;
    std::memcpy(&count, data, sizeof(count));
    if (size < 2 + static_cast<size_t>(count) * 64) return {};
    std::vector<ExMarket> result;
    size_t pos = 2;
    for (uint16_t i = 0; i < count; ++i, pos += 64) {
        const auto category = static_cast<unsigned char>(data[pos]);
        const auto market = static_cast<unsigned char>(data[pos + 33]);
        if (category == 0 && market == 0) continue;
        auto field = [&](size_t offset, size_t length) {
            size_t n = 0;
            while (n < length && data[pos + offset + n] != '\0') ++n;
            return GBKToUTF8(std::string(data + pos + offset, n));
        };
        result.push_back({market, category, field(1, 32), field(34, 2)});
    }
    return result;
}

inline size_t parse_ex_instrument_count(const char* data, size_t size) {
    if (size < 23) return 0;
    uint32_t count = 0;
    std::memcpy(&count, data + 19, sizeof(count));
    return count;
}

inline std::vector<ExInstrument> parse_ex_instrument_info(const char* data, size_t size) {
    if (size < 6) return {};
    uint16_t count = 0;
    std::memcpy(&count, data + 4, sizeof(count));
    if (size < 6 + static_cast<size_t>(count) * 64) return {};
    auto read = [&](size_t pos, size_t length) {
        size_t n = 0;
        while (n < length && data[pos + n] != '\0') ++n;
        return GBKToUTF8(std::string(data + pos, n));
    };
    std::vector<ExInstrument> result;
    result.reserve(count);
    size_t pos = 6;
    for (uint16_t i = 0; i < count; ++i, pos += 64) {
        result.push_back({static_cast<unsigned char>(data[pos]), static_cast<unsigned char>(data[pos + 1]),
                          read(pos + 5, 9), read(pos + 14, 17), read(pos + 31, 9)});
    }
    return result;
}

}  // namespace cpptdx
