#pragma once

#include "include/tdxexhq_api.h"
#include "tdx/utils.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace cpptdx {
namespace exhq_detail {

template <typename T>
inline T read(const char* data, size_t size, size_t offset, T fallback = T{}) {
    T value = fallback;
    if (offset + sizeof(T) <= size) std::memcpy(&value, data + offset, sizeof(T));
    return value;
}

inline std::vector<char> prefix(const char* hex) {
    const auto bytes = hexStringToByteArray(hex);
    return {bytes.begin(), bytes.end()};
}

inline void append_code(std::vector<char>& request, unsigned char market, const std::string& code) {
    request.push_back(static_cast<char>(market));
    const size_t size = std::min<size_t>(code.size(), 9);
    request.insert(request.end(), code.begin(), code.begin() + size);
    request.resize(request.size() + (9 - size), 0);
}

inline std::string date_string(std::uint32_t date) {
    char text[16];
    std::snprintf(text, sizeof(text), "%04u-%02u-%02u", date / 10000,
                  (date / 100) % 100, date % 100);
    return text;
}

inline std::uint32_t today_date() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return static_cast<std::uint32_t>((local.tm_year + 1900) * 10000 + (local.tm_mon + 1) * 100 + local.tm_mday);
}

inline std::vector<char> quote_request(unsigned char market, const std::string& code) {
    auto request = prefix("0101080202010c000c00fa23");
    append_code(request, market, code);
    return request;
}

inline std::vector<char> quote_list_request(unsigned char market, unsigned short start, unsigned short count) {
    auto request = prefix("01c1060b00020b000b000024");
    const std::uint16_t reserved = 0, page = 1;
    const auto* a = reinterpret_cast<const char*>(&market);
    request.insert(request.end(), a, a + sizeof(market));
    const auto* p0 = reinterpret_cast<const char*>(&reserved);
    request.insert(request.end(), p0, p0 + sizeof(reserved));
    const auto* p1 = reinterpret_cast<const char*>(&start);
    request.insert(request.end(), p1, p1 + sizeof(start));
    const auto* p2 = reinterpret_cast<const char*>(&count);
    request.insert(request.end(), p2, p2 + sizeof(count));
    const auto* p3 = reinterpret_cast<const char*>(&page);
    request.insert(request.end(), p3, p3 + sizeof(page));
    return request;
}

inline std::vector<char> bars_request(unsigned char category, unsigned char market, const std::string& code,
                                      std::uint32_t start, unsigned short count) {
    auto request = prefix("0101086a010116001600ff23");
    append_code(request, market, code);
    const std::uint16_t step = category, adjust = 1;
    request.insert(request.end(), reinterpret_cast<const char*>(&step), reinterpret_cast<const char*>(&step) + sizeof(step));
    request.insert(request.end(), reinterpret_cast<const char*>(&adjust), reinterpret_cast<const char*>(&adjust) + sizeof(adjust));
    request.insert(request.end(), reinterpret_cast<const char*>(&start), reinterpret_cast<const char*>(&start) + sizeof(start));
    request.insert(request.end(), reinterpret_cast<const char*>(&count), reinterpret_cast<const char*>(&count) + sizeof(count));
    return request;
}

inline std::vector<char> minute_request(unsigned char market, const std::string& code) {
    auto request = prefix("0107080001010c000c000b24");
    append_code(request, market, code);
    return request;
}

inline std::vector<char> history_minute_request(unsigned char market, const std::string& code, std::uint32_t date) {
    auto request = prefix("010130000101100010000c24");
    request.insert(request.end(), reinterpret_cast<const char*>(&date), reinterpret_cast<const char*>(&date) + sizeof(date));
    append_code(request, market, code);
    return request;
}

inline std::vector<char> transaction_request(unsigned char market, const std::string& code,
                                              std::int32_t start, unsigned short count) {
    auto request = prefix("01010800030112001200fc23");
    append_code(request, market, code);
    request.insert(request.end(), reinterpret_cast<const char*>(&start), reinterpret_cast<const char*>(&start) + sizeof(start));
    request.insert(request.end(), reinterpret_cast<const char*>(&count), reinterpret_cast<const char*>(&count) + sizeof(count));
    return request;
}

inline std::vector<char> history_transaction_request(unsigned char market, const std::string& code,
    std::uint32_t date, std::int32_t start, unsigned short count) {
    auto request = prefix("010130000201160016000624");
    request.insert(request.end(), reinterpret_cast<const char*>(&date), reinterpret_cast<const char*>(&date) + sizeof(date));
    append_code(request, market, code);
    request.insert(request.end(), reinterpret_cast<const char*>(&start), reinterpret_cast<const char*>(&start) + sizeof(start));
    request.insert(request.end(), reinterpret_cast<const char*>(&count), reinterpret_cast<const char*>(&count) + sizeof(count));
    return request;
}

inline std::vector<char> history_bars_range_request(unsigned char market, const std::string& code,
    std::uint32_t start_date, std::uint32_t end_date) {
    static std::atomic<unsigned char> sequence{1};
    std::vector<char> request{0x01, static_cast<char>(sequence.fetch_add(1))};
    const auto tail = prefix("38920001160016000d24");
    request.insert(request.end(), tail.begin(), tail.end());
    append_code(request, market, code);
    request.push_back(0x07);
    request.push_back(0x00);
    request.insert(request.end(), reinterpret_cast<const char*>(&start_date), reinterpret_cast<const char*>(&start_date) + sizeof(start_date));
    request.insert(request.end(), reinterpret_cast<const char*>(&end_date), reinterpret_cast<const char*>(&end_date) + sizeof(end_date));
    return request;
}

inline std::vector<ExQuote> parse_quote(const char* data, size_t size) {
    if (!data || size < 150) return {};
    ExQuote quote;
    quote.market = read<unsigned char>(data, size, 0);
    size_t code_length = 0;
    while (code_length < 9 && data[1 + code_length] != '\0') ++code_length;
    quote.code = std::string(data + 1, code_length);
    constexpr size_t p = 14;
    quote.pre_close = read<float>(data, size, p);
    quote.open = read<float>(data, size, p + 4);
    quote.high = read<float>(data, size, p + 8);
    quote.low = read<float>(data, size, p + 12);
    quote.price = read<float>(data, size, p + 16);
    quote.kaicang = read<std::uint32_t>(data, size, p + 20);
    quote.zongliang = read<std::uint32_t>(data, size, p + 28);
    quote.xianliang = read<std::uint32_t>(data, size, p + 32);
    quote.neipan = read<std::uint32_t>(data, size, p + 40);
    quote.waipan = read<std::uint32_t>(data, size, p + 44);
    quote.chicang = read<std::uint32_t>(data, size, p + 52);
    auto set_price = [](ExQuote& q, size_t index, double value, bool bid) {
        double* target = bid ? &q.bid1 : &q.ask1;
        switch (index) {
            case 0: target[0] = value; break;
            case 1: (bid ? q.bid2 : q.ask2) = value; break;
            case 2: (bid ? q.bid3 : q.ask3) = value; break;
            case 3: (bid ? q.bid4 : q.ask4) = value; break;
            default: (bid ? q.bid5 : q.ask5) = value; break;
        }
    };
    auto set_volume = [](ExQuote& q, size_t index, std::uint32_t value, bool bid) {
        if (bid) {
            switch (index) { case 0: q.bid_vol1=value; break; case 1: q.bid_vol2=value; break;
                case 2: q.bid_vol3=value; break; case 3: q.bid_vol4=value; break; default: q.bid_vol5=value; }
        } else {
            switch (index) { case 0: q.ask_vol1=value; break; case 1: q.ask_vol2=value; break;
                case 2: q.ask_vol3=value; break; case 3: q.ask_vol4=value; break; default: q.ask_vol5=value; }
        }
    };
    for (size_t i = 0; i < 5; ++i) {
        set_price(quote, i, read<float>(data, size, p + 56 + i * 4), true);
        set_volume(quote, i, read<std::uint32_t>(data, size, p + 76 + i * 4), true);
        set_price(quote, i, read<float>(data, size, p + 96 + i * 4), false);
        set_volume(quote, i, read<std::uint32_t>(data, size, p + 116 + i * 4), false);
    }
    return {quote};
}

inline std::vector<ExMinuteBar> parse_minutes(const char* data, size_t size, size_t header_size) {
    std::vector<ExMinuteBar> rows;
    if (!data || size < header_size) return rows;
    const auto count = read<std::uint16_t>(data, size, header_size - 2);
    if (count > (size - header_size) / 18) return {};
    rows.reserve(count);
    size_t pos = header_size;
    for (std::uint16_t i = 0; i < count; ++i, pos += 18) {
        const auto raw_time = read<std::uint16_t>(data, size, pos);
        rows.push_back({raw_time / 60, raw_time % 60, read<float>(data, size, pos + 2),
                        read<float>(data, size, pos + 6), read<std::uint32_t>(data, size, pos + 10),
                        read<std::uint32_t>(data, size, pos + 14)});
    }
    return rows;
}

inline std::string nature_name(int value, std::uint32_t volume, std::int32_t change) {
    if (value == 0) {
        if (change > 0) return volume > static_cast<std::uint32_t>(change) ? "多开" : "双开";
        if (change == 0) return "多换";
        return volume == static_cast<std::uint32_t>(-static_cast<std::int64_t>(change)) ? "双平" : "空平";
    }
    if (value == 1) {
        if (change > 0) return volume > static_cast<std::uint32_t>(change) ? "空开" : "双开";
        if (change == 0) return "空换";
        return volume == static_cast<std::uint32_t>(-static_cast<std::int64_t>(change)) ? "双平" : "多平";
    }
    if (change > 0) return volume > static_cast<std::uint32_t>(change) ? "开仓" : "双开";
    if (change < 0) return volume > static_cast<std::uint32_t>(-static_cast<std::int64_t>(change)) ? "平仓" : "双平";
    return "换手";
}

inline std::vector<ExTransaction> parse_transactions(const char* data, size_t size,
    std::uint32_t date, unsigned char market) {
    std::vector<ExTransaction> rows;
    if (!data || size < 16) return rows;
    const auto count = read<std::uint16_t>(data, size, 14);
    if (count > (size - 16) / 16) return {};
    rows.reserve(count);
    const auto iso_date = date_string(date);
    size_t pos = 16;
    for (std::uint16_t i = 0; i < count; ++i, pos += 16) {
        const auto raw_time = read<std::uint16_t>(data, size, pos);
        const auto direction_raw = read<std::uint16_t>(data, size, pos + 14);
        int second = direction_raw % 10000;
        if (second > 59) second = 0;
        ExTransaction row;
        row.date = iso_date;
        row.hour = raw_time / 60;
        row.minute = raw_time % 60;
        row.second = second;
        row.price = read<std::uint32_t>(data, size, pos + 2);
        row.volume = read<std::uint32_t>(data, size, pos + 6);
        row.zengcang = read<std::int32_t>(data, size, pos + 10);
        row.nature = direction_raw;
        row.nature_mark = direction_raw / 10000;
        row.nature_value = direction_raw % 10000;
        row.direction = row.nature_mark == 0 ? 1 : (row.nature_mark == 1 ? -1 : 0);
        row.nature_name = nature_name(row.nature_mark, row.volume, row.zengcang);
        if (market == 31 || market == 48) {
            if (direction_raw == 0) { row.direction = 1; row.nature_name = "B"; }
            else if (direction_raw == 256) { row.direction = -1; row.nature_name = "S"; }
            else { row.direction = 0; row.nature_name.clear(); }
        }
        row.natrue_name = row.nature_name;
        rows.push_back(std::move(row));
    }
    return rows;
}

inline std::vector<ExBar> parse_bars(const char* data, size_t size, unsigned char category) {
    std::vector<ExBar> rows;
    if (!data || size < 20) return rows;
    const auto count = read<std::uint16_t>(data, size, 18);
    if (count > (size - 20) / 32) return {};
    rows.reserve(count);
    size_t pos = 20;
    for (std::uint16_t i = 0; i < count; ++i) {
        int year, month, day, hour, minute;
        std::tie(year, month, day, hour, minute, pos) = get_datetime(category, data, static_cast<int>(pos));
        if (pos + 28 > size) return {};
        ExBar row;
        row.open = read<float>(data, size, pos);
        row.high = read<float>(data, size, pos + 4);
        row.low = read<float>(data, size, pos + 8);
        row.close = read<float>(data, size, pos + 12);
        row.position = read<std::uint32_t>(data, size, pos + 16);
        row.trade = read<std::uint32_t>(data, size, pos + 20);
        row.price = read<float>(data, size, pos + 24);
        row.amount = read<float>(data, size, pos + 16);
        row.year = year; row.month = month; row.day = day; row.hour = hour; row.minute = minute;
        char datetime[24];
        std::snprintf(datetime, sizeof(datetime), "%04d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
        row.datetime = datetime;
        rows.push_back(std::move(row));
        pos += 28;
    }
    return rows;
}

inline std::vector<ExRangeBar> parse_history_bars_range(const char* data, size_t size) {
    std::vector<ExRangeBar> rows;
    if (!data || size < 14) return rows;
    const auto count = read<std::uint16_t>(data, size, 12);
    if (count > (size - 14) / 32) return {};
    size_t pos = 14;
    rows.reserve(count);
    for (std::uint16_t i = 0; i < count; ++i, pos += 32) {
        const auto date = read<std::uint16_t>(data, size, pos);
        const auto raw_time = read<std::uint16_t>(data, size, pos + 2);
        ExRangeBar row;
        row.year = date / 2048 + 2004;
        row.month = (date % 2048) / 100;
        row.day = (date % 2048) % 100;
        row.hour = raw_time / 60;
        row.minute = raw_time % 60;
        row.open = read<float>(data, size, pos + 4);
        row.high = read<float>(data, size, pos + 8);
        row.low = read<float>(data, size, pos + 12);
        row.close = read<float>(data, size, pos + 16);
        row.position = read<std::uint32_t>(data, size, pos + 20);
        row.trade = read<std::uint32_t>(data, size, pos + 24);
        row.settlementprice = read<float>(data, size, pos + 28);
        char datetime[24];
        std::snprintf(datetime, sizeof(datetime), "%04d-%02d-%02d %02d:%02d", row.year, row.month,
                      row.day, row.hour, row.minute);
        row.datetime = datetime;
        rows.push_back(std::move(row));
    }
    return rows;
}

inline std::vector<nlohmann::json> parse_quote_list(const char* data, size_t size, unsigned char category) {
    std::vector<nlohmann::json> rows;
    if (!data || size < 2 || (category != 2 && category != 3)) return rows;
    const auto count = read<std::uint16_t>(data, size, 0);
    if (count > (size - 2) / 300) return {};
    size_t pos = 2;
    rows.reserve(count);
    for (std::uint16_t row = 0; row < count; ++row, pos += 300) {
        const auto market = read<unsigned char>(data, size, pos);
        const char* code_data = data + pos + 1;
        size_t code_length = 0;
        while (code_length < 9 && code_data[code_length] != '\0') ++code_length;
        const auto code = GBKToUTF8(std::string(code_data, code_length));
        const size_t p = pos + 10;
        nlohmann::json item = {{"market", market}, {"code", code}};
        if (category == 2) {
            const char* names[] = {"HuoYueDu","ZuoShou","JinKai","ZuiGao","ZuiDi","XianJia","MaiRuJia",
                "ZongLiang","XianLiang","ZongJinE","Nei","Wai"};
            item[names[0]] = read<std::uint32_t>(data, size, p);
            const char* float_names[] = {"ZuoShou","JinKai","ZuiGao","ZuiDi","XianJia"};
            for (size_t i = 0; i < 5; ++i) item[float_names[i]] = read<float>(data, size, p + 4 + i * 4);
            item["MaiRuJia"] = read<float>(data, size, p + 28);
            item["ZongLiang"] = read<std::uint32_t>(data, size, p + 32);
            item["XianLiang"] = read<std::uint32_t>(data, size, p + 36);
            item["ZongJinE"] = read<float>(data, size, p + 40);
            item["Nei"] = read<std::uint32_t>(data, size, p + 52);
            item["Wai"] = read<std::uint32_t>(data, size, p + 56);
            for (size_t i = 0; i < 5; ++i) {
                item["MaiRuJia" + std::to_string(i + 1)] = read<float>(data, size, p + 60 + i * 4);
                item["MaiRuLiang" + std::to_string(i + 1)] = read<std::uint32_t>(data, size, p + 80 + i * 4);
                item["MaiChuJia" + std::to_string(i + 1)] = read<float>(data, size, p + 100 + i * 4);
                item["MaiChuLiang" + std::to_string(i + 1)] = read<std::uint32_t>(data, size, p + 120 + i * 4);
            }
        } else {
            item["BiShu"] = read<std::uint32_t>(data, size, p);
            item["ZuoJie"] = read<float>(data, size, p + 4);
            item["JinKai"] = read<float>(data, size, p + 8);
            item["ZuiGao"] = read<float>(data, size, p + 12);
            item["ZuiDi"] = read<float>(data, size, p + 16);
            item["MaiChu"] = read<float>(data, size, p + 20);
            item["KaiCang"] = read<std::uint32_t>(data, size, p + 24);
            item["ZongLiang"] = read<std::uint32_t>(data, size, p + 32);
            item["XianLiang"] = read<std::uint32_t>(data, size, p + 36);
            item["ZongJinE"] = read<float>(data, size, p + 40);
            item["NeiPan"] = read<std::uint32_t>(data, size, p + 44);
            item["WaiPan"] = read<std::uint32_t>(data, size, p + 48);
            item["ChiCangLiang"] = read<std::uint32_t>(data, size, p + 56);
            item["MaiRuJia"] = read<float>(data, size, p + 60);
            item["MaiRuLiang"] = read<std::uint32_t>(data, size, p + 80);
            item["MaiChuJia"] = read<float>(data, size, p + 100);
            item["MaiChuLiang"] = read<std::uint32_t>(data, size, p + 120);
        }
        rows.push_back(std::move(item));
    }
    return rows;
}

}  // namespace exhq_detail
}  // namespace cpptdx
