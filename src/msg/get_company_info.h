#pragma once

#include "include/tdxhq_api.h"
#include "tdx/utils.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cpptdx {

inline std::string make_get_company_info_category_request(Market market, const std::string& code) {
    std::string request("\x0c\x0f\x10\x9b\x00\x01\x0e\x00\x0e\x00\xcf\x02", 12);
    const auto m = static_cast<uint16_t>(market);
    request.append(reinterpret_cast<const char*>(&m), sizeof(m));
    char stock_code[6] = {};
    std::memcpy(stock_code, code.data(), std::min(code.size(), sizeof(stock_code)));
    request.append(stock_code, sizeof(stock_code));
    const uint32_t zero = 0;
    request.append(reinterpret_cast<const char*>(&zero), sizeof(zero));
    return request;
}

inline std::string make_get_company_info_content_request(Market market, const std::string& code,
                                                         const std::string& filename,
                                                         uint32_t start, uint32_t length) {
    std::string request("\x0c\x07\x10\x9c\x00\x01\x68\x00\x68\x00\xd0\x02", 12);
    const auto m = static_cast<uint16_t>(market);
    request.append(reinterpret_cast<const char*>(&m), sizeof(m));
    char stock_code[6] = {};
    std::memcpy(stock_code, code.data(), std::min(code.size(), sizeof(stock_code)));
    request.append(stock_code, sizeof(stock_code));
    const uint16_t zero16 = 0;
    request.append(reinterpret_cast<const char*>(&zero16), sizeof(zero16));
    char file_field[80] = {};
    std::memcpy(file_field, filename.data(), std::min(filename.size(), sizeof(file_field)));
    request.append(file_field, sizeof(file_field));
    request.append(reinterpret_cast<const char*>(&start), sizeof(start));
    request.append(reinterpret_cast<const char*>(&length), sizeof(length));
    const uint32_t zero32 = 0;
    request.append(reinterpret_cast<const char*>(&zero32), sizeof(zero32));
    return request;
}

inline std::vector<CompanyInfoFile> parse_company_info_category(const char* data, size_t size) {
    if (size < 2) return {};
    uint16_t count = 0;
    std::memcpy(&count, data, sizeof(count));
    if (size < 2 + static_cast<size_t>(count) * 152) return {};
    auto read_text = [](const char* field, size_t length) {
        size_t actual = 0;
        while (actual < length && field[actual] != '\0') ++actual;
        return GBKToUTF8(std::string(field, actual));
    };
    std::vector<CompanyInfoFile> files;
    files.reserve(count);
    size_t pos = 2;
    for (uint16_t i = 0; i < count; ++i, pos += 152) {
        CompanyInfoFile file;
        file.name = read_text(data + pos, 64);
        file.filename = read_text(data + pos + 64, 80);
        std::memcpy(&file.start, data + pos + 144, sizeof(file.start));
        std::memcpy(&file.length, data + pos + 148, sizeof(file.length));
        files.push_back(std::move(file));
    }
    return files;
}

inline std::string parse_company_info_content(const char* data, size_t size) {
    if (size < 12) return {};
    uint16_t length = 0;
    std::memcpy(&length, data + 10, sizeof(length));
    if (length > size - 12) return {};
    return GBKToUTF8(std::string(data + 12, length));
}

}  // namespace cpptdx
