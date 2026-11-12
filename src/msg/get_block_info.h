#pragma once

#include "include/tdxhq_api.h"
#include "tdx/utils.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cpptdx {

inline std::vector<char> make_get_block_info_meta_request(const std::string& filename) {
    const auto prefix = hexStringToByteArray("0c39186900012a002a00c502");
    std::vector<char> request(prefix.begin(), prefix.end());
    const size_t length = std::min<size_t>(filename.size(), 40);
    request.insert(request.end(), filename.begin(), filename.begin() + length);
    request.resize(52, 0);
    return request;
}

inline BlockInfoMeta parse_block_info_meta(const char* data, size_t size) {
    BlockInfoMeta result;
    if (size < 38) return result;
    std::memcpy(&result.size, data, sizeof(result.size));
    static constexpr char hex[] = "0123456789abcdef";
    result.hash_value.reserve(64);
    for (size_t i = 5; i < 37; ++i) {
        const auto byte = static_cast<unsigned char>(data[i]);
        result.hash_value.push_back(hex[byte >> 4]);
        result.hash_value.push_back(hex[byte & 0x0f]);
    }
    return result;
}

inline std::vector<char> make_get_block_info_request(const std::string& filename,
                                                     std::uint32_t start, std::uint32_t size) {
    const auto prefix = hexStringToByteArray("0c37186a00016e006e00b906");
    std::vector<char> request(prefix.begin(), prefix.end());
    request.insert(request.end(), reinterpret_cast<const char*>(&start), reinterpret_cast<const char*>(&start) + sizeof(start));
    request.insert(request.end(), reinterpret_cast<const char*>(&size), reinterpret_cast<const char*>(&size) + sizeof(size));
    const size_t length = std::min<size_t>(filename.size(), 100);
    request.insert(request.end(), filename.begin(), filename.begin() + length);
    request.resize(120, 0);
    return request;
}

inline std::string parse_block_info_content(const char* data, size_t size) {
    if (size <= 4) return {};
    return std::string(data + 4, size - 4);
}

inline std::vector<BlockConstituent> parse_block_info_file(const std::string& data) {
    std::vector<BlockConstituent> result;
    if (data.size() < 386) return result;
    uint16_t block_count = 0;
    std::memcpy(&block_count, data.data() + 384, sizeof(block_count));
    size_t pos = 386;
    for (uint16_t block = 0; block < block_count; ++block) {
        if (pos + 13 > data.size()) return {};
        const std::string name_raw(data.data() + pos, 9);
        const auto nul = name_raw.find('\0');
        const auto blockname = GBKToUTF8(name_raw.substr(0, nul));
        pos += 9;
        uint16_t stock_count = 0, block_type = 0;
        std::memcpy(&stock_count, data.data() + pos, sizeof(stock_count));
        std::memcpy(&block_type, data.data() + pos + 2, sizeof(block_type));
        pos += 4;
        const size_t stock_begin = pos;
        if (stock_count > 400 || stock_begin + static_cast<size_t>(stock_count) * 7 > data.size()) return {};
        for (uint16_t index = 0; index < stock_count; ++index) {
            const char* code_data = data.data() + pos;
            size_t code_length = 0;
            while (code_length < 7 && code_data[code_length] != '\0') ++code_length;
            result.push_back({blockname, block_type, index, std::string(code_data, code_length)});
            pos += 7;
        }
        const size_t next = stock_begin + 2800;
        if (next > data.size()) return {};
        pos = next;
    }
    return result;
}

}  // namespace cpptdx
