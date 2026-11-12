#include "include/tdxhq_api.h"
#include "msg/get_security_list.h"
#include "msg/get_security_klines.h"
#include "msg/get_security_snapshot.h"
#include "msg/get_finance_info.h"
#include "msg/exhq_basic.h"
#include "msg/get_block_info.h"
#include "msg/get_exhq_data.h"
#include "msg/get_intraday_data.h"
#include "msg/get_company_info.h"
#include "msg/get_xdxr_info.h"
#include "storage/market_store.h"
#include "tdx/tdx_base.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

using namespace cpptdx;

void test_make_get_security_list_request() {
    std::cout << "Testing make_get_security_list_request..." << std::endl;
    auto req = make_get_security_list_request(0, 0);
    assert(req.size() == 16);
    std::cout << "PASSED" << std::endl;
}

void test_make_get_bars_request() {
    std::cout << "Testing make_get_bars_request..." << std::endl;
    auto req = make_get_bars_request(4, 1, "600000", 0, 100);
    assert(req.size() >= 30);
    assert(req[0] == 0x0c && req[1] == 0x01);
    std::cout << "PASSED (size: " << req.size() << ")" << std::endl;
}

void test_make_get_snap_request() {
    std::cout << "Testing make_get_snap_request..." << std::endl;
    std::vector<std::pair<std::string, Market>> stocks;
    stocks.emplace_back("600000", Market::kSH);
    stocks.emplace_back("000001", Market::kSZ);
    auto req = make_get_snap_request(stocks);
    assert(req.size() >= 20);
    std::cout << "PASSED (size: " << req.size() << ")" << std::endl;
}

void test_make_get_finance_info_request() {
    std::cout << "Testing make_get_finance_info_request..." << std::endl;
    auto req = make_get_finance_info_request(1, "600000");
    assert(req.size() >= 15);
    std::cout << "PASSED (size: " << req.size() << ")" << std::endl;
}

void test_parse_security_list() {
    std::cout << "Testing parse_security_list..." << std::endl;
    const char data[] = {
        0x02, 0x00,
        0x36, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0xe8, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x02,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    auto result = parse_security_list(data, sizeof(data));
    assert(result.size() == 2);
    std::cout << "PASSED" << std::endl;
}

void test_parse_kline() {
    std::cout << "Testing parse_kline..." << std::endl;
    unsigned char data[] = {
        0x01, 0x00,
        0x1f, 0x07, 0x10, 0x01, 0x18, 0x02, 0x01, 0x2c,
        0x00, 0x64, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    auto result = parse_kline((const char*)data, sizeof(data), 4);
    assert(result.size() == 1);
    const char truncated_response[] = {0x20, 0x03}; // claims 800 rows with no row payload
    assert(parse_kline(truncated_response, sizeof(truncated_response), 9).empty());
    std::cout << "PASSED (klines count: " << result.size() << ")" << std::endl;
}

void test_parse_finance_info() {
    std::cout << "Testing parse_finance_info..." << std::endl;
    const char data[] = {
        0x01, 0x00,
        0x01, '6', '0', '0', '0', '0', '0',
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00
    };
    auto result = parse_finance_info(data, sizeof(data));
    assert(result.code == "600000");
    std::cout << "PASSED" << std::endl;
}

void test_category_enum() {
    std::cout << "Testing Category enum..." << std::endl;
    assert((unsigned short)Category::k5Minute == 0);
    assert((unsigned short)Category::kDay == 4);
    assert((unsigned short)Category::kWeek == 5);
    std::cout << "PASSED" << std::endl;
}

void test_market_enum() {
    std::cout << "Testing Market enum..." << std::endl;
    assert((unsigned short)Market::kSZ == 0);
    assert((unsigned short)Market::kSH == 1);
    std::cout << "PASSED" << std::endl;
}

template <typename T>
void write_value(std::vector<char>& data, size_t offset, T value) {
    assert(offset + sizeof(T) <= data.size());
    std::memcpy(data.data() + offset, &value, sizeof(T));
}

void test_exhq_requests_and_parsers() {
    std::cout << "Testing ExHq requests and parsers..." << std::endl;
    const auto markets_request = make_ex_get_markets_request();
    const auto count_request = make_ex_get_instrument_count_request();
    const auto info_request = make_ex_get_instrument_info_request(0x12345678, 25);
    assert(markets_request.size() == 12 && count_request.size() == 12);
    assert(info_request.size() == 18);

    std::vector<char> markets(66, 0);
    write_value<std::uint16_t>(markets, 0, 1);
    markets[2] = 2;
    std::memcpy(markets.data() + 3, "HK Market", 9);
    markets[35] = 31;
    std::memcpy(markets.data() + 36, "HK", 2);
    const auto parsed_markets = parse_ex_markets(markets.data(), markets.size());
    assert(parsed_markets.size() == 1);
    assert(parsed_markets[0].category == 2 && parsed_markets[0].market == 31);
    assert(parsed_markets[0].name == "HK Market" && parsed_markets[0].short_name == "HK");

    std::vector<char> instrument_count(23, 0);
    write_value<std::uint32_t>(instrument_count, 19, 12345);
    assert(parse_ex_instrument_count(instrument_count.data(), instrument_count.size()) == 12345);

    std::vector<char> instruments(70, 0);
    write_value<std::uint16_t>(instruments, 4, 1);
    instruments[6] = 3;
    instruments[7] = 47;
    std::memcpy(instruments.data() + 11, "IFL0", 4);
    std::memcpy(instruments.data() + 20, "Index Future", 12);
    std::memcpy(instruments.data() + 37, "CFFEX", 5);
    const auto parsed_instruments = parse_ex_instrument_info(instruments.data(), instruments.size());
    assert(parsed_instruments.size() == 1);
    assert(parsed_instruments[0].category == 3 && parsed_instruments[0].market == 47);
    assert(parsed_instruments[0].code == "IFL0" && parsed_instruments[0].name == "Index Future");
    assert(parsed_instruments[0].desc == "CFFEX");

    auto quote_request = exhq_detail::quote_request(47, "IFL0");
    auto quote_list_request = exhq_detail::quote_list_request(29, 0, 80);
    auto bars_request = exhq_detail::bars_request(4, 47, "IFL0", 0x12345678, 100);
    auto minute_request = exhq_detail::minute_request(47, "IFL0");
    auto history_minute_request = exhq_detail::history_minute_request(47, "IFL0", 20261111);
    auto transaction_request = exhq_detail::transaction_request(47, "IFL0", 10, 100);
    auto history_transaction_request = exhq_detail::history_transaction_request(47, "IFL0", 20261111, 10, 100);
    auto range_request = exhq_detail::history_bars_range_request(47, "IFL0", 20261101, 20261111);
    assert(quote_request.size() == 22 && quote_list_request.size() == 21);
    assert(bars_request.size() == 32);
    assert(minute_request.size() == 22 && history_minute_request.size() == 26);
    assert(transaction_request.size() == 28 && history_transaction_request.size() == 32);
    assert(range_request.size() == 32);

    std::vector<char> quote_data(150, 0);
    quote_data[0] = 47;
    std::memcpy(quote_data.data() + 1, "IFL0", 4);
    write_value<float>(quote_data, 14, 100.5f);
    write_value<float>(quote_data, 30, 101.25f);
    write_value<float>(quote_data, 70, 101.0f);
    const auto quote = exhq_detail::parse_quote(quote_data.data(), quote_data.size());
    assert(quote.size() == 1 && quote[0].market == 47 && quote[0].code == "IFL0");
    assert(quote[0].pre_close == 100.5 && quote[0].price == 101.25 && quote[0].bid1 == 101.0);

    std::vector<char> minute_data(30, 0);
    write_value<std::uint16_t>(minute_data, 10, 1);
    write_value<std::uint16_t>(minute_data, 12, 61);
    write_value<float>(minute_data, 14, 101.5f);
    write_value<float>(minute_data, 18, 100.5f);
    write_value<std::uint32_t>(minute_data, 22, 12);
    write_value<std::uint32_t>(minute_data, 26, 34);
    const auto minutes = exhq_detail::parse_minutes(minute_data.data(), minute_data.size(), 12);
    assert(minutes.size() == 1 && minutes[0].hour == 1 && minutes[0].minute == 1);
    assert(minutes[0].price == 101.5 && minutes[0].volume == 12 && minutes[0].open_interest == 34);
    std::vector<char> historical_minute_data(38, 0);
    write_value<std::uint16_t>(historical_minute_data, 18, 1);
    write_value<std::uint16_t>(historical_minute_data, 20, 60);
    write_value<float>(historical_minute_data, 22, 99.5f);
    write_value<float>(historical_minute_data, 26, 99.0f);
    write_value<std::uint32_t>(historical_minute_data, 30, 7);
    write_value<std::uint32_t>(historical_minute_data, 34, 8);
    const auto historical_minutes = exhq_detail::parse_minutes(historical_minute_data.data(),
                                                                historical_minute_data.size(), 20);
    assert(historical_minutes.size() == 1 && historical_minutes[0].hour == 1);
    assert(historical_minutes[0].price == 99.5 && historical_minutes[0].volume == 7);

    std::vector<char> ex_bars(52, 0);
    write_value<std::uint16_t>(ex_bars, 18, 1);
    write_value<std::uint32_t>(ex_bars, 20, 20261111);
    write_value<float>(ex_bars, 24, 10.0f);
    write_value<float>(ex_bars, 28, 12.0f);
    write_value<float>(ex_bars, 32, 9.0f);
    write_value<float>(ex_bars, 36, 11.0f);
    write_value<std::uint32_t>(ex_bars, 40, 50);
    write_value<std::uint32_t>(ex_bars, 44, 3);
    write_value<float>(ex_bars, 48, 11.0f);
    const auto parsed_bars = exhq_detail::parse_bars(ex_bars.data(), ex_bars.size(), 4);
    assert(parsed_bars.size() == 1 && parsed_bars[0].datetime == "2026-11-11 15:00");
    assert(parsed_bars[0].open == 10.0 && parsed_bars[0].high == 12.0 && parsed_bars[0].trade == 3);

    std::vector<char> range_bars(46, 0);
    write_value<std::uint16_t>(range_bars, 12, 1);
    write_value<std::uint16_t>(range_bars, 14, static_cast<std::uint16_t>((2026 - 2004) * 2048 + 11 * 100 + 11));
    write_value<std::uint16_t>(range_bars, 16, 61);
    write_value<float>(range_bars, 18, 20.0f);
    write_value<float>(range_bars, 22, 22.0f);
    write_value<float>(range_bars, 26, 19.0f);
    write_value<float>(range_bars, 30, 21.0f);
    write_value<std::uint32_t>(range_bars, 34, 60);
    write_value<std::uint32_t>(range_bars, 38, 4);
    write_value<float>(range_bars, 42, 20.5f);
    const auto parsed_range_bars = exhq_detail::parse_history_bars_range(range_bars.data(), range_bars.size());
    assert(parsed_range_bars.size() == 1 && parsed_range_bars[0].datetime == "2026-11-11 01:01");
    assert(parsed_range_bars[0].open == 20.0 && parsed_range_bars[0].settlementprice == 20.5);

    std::vector<char> quote_list_data(302, 0);
    write_value<std::uint16_t>(quote_list_data, 0, 1);
    quote_list_data[2] = 29;
    std::memcpy(quote_list_data.data() + 3, "HSI", 3);
    write_value<std::uint32_t>(quote_list_data, 12, 8);
    write_value<float>(quote_list_data, 16, 18000.0f);
    const auto quote_list = exhq_detail::parse_quote_list(quote_list_data.data(), quote_list_data.size(), 3);
    assert(quote_list.size() == 1 && quote_list[0]["market"] == 29 && quote_list[0]["code"] == "HSI");
    assert(quote_list[0]["BiShu"] == 8 && quote_list[0]["ZuoJie"] == 18000.0f);

    std::vector<char> transaction_data(32, 0);
    write_value<std::uint16_t>(transaction_data, 14, 1);
    write_value<std::uint16_t>(transaction_data, 16, 61);
    write_value<std::uint32_t>(transaction_data, 18, 4125);
    write_value<std::uint32_t>(transaction_data, 22, 9);
    write_value<std::int32_t>(transaction_data, 26, 2);
    write_value<std::uint16_t>(transaction_data, 30, 0);
    const auto transactions = exhq_detail::parse_transactions(transaction_data.data(), transaction_data.size(),
                                                               20261111, 47);
    assert(transactions.size() == 1 && transactions[0].date == "2026-11-11");
    assert(transactions[0].hour == 1 && transactions[0].minute == 1);
    assert(transactions[0].price == 4125 && transactions[0].volume == 9 && transactions[0].zengcang == 2);
    std::cout << "PASSED" << std::endl;
}

void test_block_info_requests_and_parsers() {
    std::cout << "Testing block-info requests and parsers..." << std::endl;
    const auto meta_request = make_get_block_info_meta_request("block_gn.dat");
    const auto content_request = make_get_block_info_request("block_gn.dat", 10, 100);
    assert(meta_request.size() == 52 && content_request.size() == 120);

    std::vector<char> metadata(38, 0);
    write_value<std::uint32_t>(metadata, 0, 0x10203040);
    for (size_t i = 0; i < 32; ++i) metadata[5 + i] = static_cast<char>(i);
    const auto meta = parse_block_info_meta(metadata.data(), metadata.size());
    assert(meta.size == 0x10203040 && meta.hash_value.size() == 64);
    assert(meta.hash_value.substr(0, 4) == "0001");
    const char raw_content[] = {0, 0, 0, 0, 'a', 'b'};
    assert(parse_block_info_content(raw_content, sizeof(raw_content)) == "ab");

    std::string block(3199, '\0');
    block[384] = 1;
    block[385] = 0;
    std::memcpy(block.data() + 386, "blockname", 9);
    const std::uint16_t security_count = 1;
    const std::uint16_t block_type = 7;
    std::memcpy(block.data() + 395, &security_count, sizeof(security_count));
    std::memcpy(block.data() + 397, &block_type, sizeof(block_type));
    std::memcpy(block.data() + 399, "600000", 6);
    const auto constituents = parse_block_info_file(block);
    assert(constituents.size() == 1);
    assert(constituents[0].blockname == "blockname" && constituents[0].block_type == 7);
    assert(constituents[0].code_index == 0 && constituents[0].code == "600000");
    std::cout << "PASSED" << std::endl;
}

void test_hq_intraday_company_and_xdxr_parsers() {
    std::cout << "Testing HQ intraday, company, and XDXR requests/parsers..." << std::endl;
    assert(make_get_minute_time_request(Market::kSH, "600000").size() == 24);
    assert(make_get_history_minute_time_request(Market::kSH, "600000", 20261111).size() == 23);
    assert(make_get_transaction_request(Market::kSH, "600000", 0, 100).size() == 24);
    assert(make_get_history_transaction_request(Market::kSH, "600000", 0, 100, 20261111).size() == 28);
    assert(make_get_company_info_category_request(Market::kSH, "600000").size() == 24);
    assert(make_get_company_info_content_request(Market::kSH, "600000", "profile.txt", 0, 100).size() == 114);
    assert(make_get_xdxr_info_request(Market::kSH, "600000").size() == 21);

    const char minute_data[] = {1, 0, 10, 0, 5};
    const auto minutes = parse_minute_time_data(minute_data, sizeof(minute_data), 2);
    assert(minutes.size() == 1 && minutes[0].price == 0.1 && minutes[0].vol == 5);
    const char transaction_data[] = {1, 0, 60, 0, 10, 5, 1, 1, 0};
    const auto transactions = parse_transaction_data(transaction_data, sizeof(transaction_data), false);
    assert(transactions.size() == 1 && transactions[0].time == "01:00");
    assert(transactions[0].price == 0.1 && transactions[0].vol == 5 && transactions[0].num == 1);

    std::vector<char> company_category(154, 0);
    write_value<std::uint16_t>(company_category, 0, 1);
    std::memcpy(company_category.data() + 2, "Company Profile", 15);
    std::memcpy(company_category.data() + 66, "profile.txt", 11);
    write_value<std::uint32_t>(company_category, 146, 120);
    write_value<std::uint32_t>(company_category, 150, 55);
    const auto files = parse_company_info_category(company_category.data(), company_category.size());
    assert(files.size() == 1 && files[0].name == "Company Profile" && files[0].filename == "profile.txt");
    assert(files[0].start == 120 && files[0].length == 55);
    std::vector<char> company_content(17, 0);
    write_value<std::uint16_t>(company_content, 10, 5);
    std::memcpy(company_content.data() + 12, "hello", 5);
    assert(parse_company_info_content(company_content.data(), company_content.size()) == "hello");

    std::vector<char> xdxr(40, 0);
    write_value<std::uint16_t>(xdxr, 9, 1);
    write_value<std::uint32_t>(xdxr, 19, 20261111);
    xdxr[23] = 1;
    float dividend = 0.25f;
    std::memcpy(xdxr.data() + 24, &dividend, sizeof(dividend));
    const auto events = parse_xdxr_info(xdxr.data(), xdxr.size());
    assert(events.size() == 1 && events[0].year == 2026 && events[0].month == 11 && events[0].day == 11);
    assert(events[0].category == 1 && events[0].name == "除权除息" && events[0].fenhong == 0.25);
    std::cout << "PASSED" << std::endl;
}

void test_duckdb_market_store_roundtrip() {
#ifdef CPPTDX_WITH_DUCKDB
    std::cout << "Testing DuckDB market-store round trip..." << std::endl;
    MarketStore store(":memory:");
    assert(store.available());

    const std::vector<SecurityInfo> securities{{"600000", 100, "Pudong Development", 2, 10.5}};
    assert(store.save_security_list(Market::kSH, 0, securities));
    const auto loaded_securities = store.get_security_list(Market::kSH, 0, 10);
    assert(loaded_securities.size() == 1 && loaded_securities[0].code == "600000");
    assert(loaded_securities[0].name == "Pudong Development");
    assert(store.get_security_count(Market::kSH) == 1);

    const auto finance_json = nlohmann::json{{"code", "600000"}, {"market", 1}, {"liutongguben", 100.0}}.dump();
    assert(store.save_finance_info(Market::kSH, "600000", finance_json));
    assert(nlohmann::json::parse(store.get_finance_info(Market::kSH, "600000"))["liutongguben"] == 100.0);

    nlohmann::json ex_rows = nlohmann::json::array({{{"price", 18200.5}, {"volume", 42}}});
    assert(store.save_ex_data("quote", 29, "HSI", 0, 0, ex_rows));
    const auto loaded_ex_rows = store.get_ex_data("quote", 29, "HSI", 0, 10, 0);
    assert(loaded_ex_rows.size() == 1 && loaded_ex_rows[0]["price"] == 18200.5);

    const BlockInfoMeta meta{5, "0123456789abcdef"};
    const std::vector<BlockConstituent> constituents{{"industry", 7, 0, "600000"}};
    assert(store.save_block_file("block_gn.dat", meta, "abcde", constituents));
    const auto stored_meta = store.get_block_info_meta("block_gn.dat");
    assert(stored_meta.size == 5 && stored_meta.hash_value == meta.hash_value);
    const auto stored_content = store.get_block_info_content("block_gn.dat", 1, 3);
    if (stored_content != "bcd") std::cerr << "Unexpected cached block content: size=" << stored_content.size()
                                           << " value=" << stored_content << std::endl;
    assert(stored_content == "bcd");
    const auto stored_constituents = store.get_block_constituents("block_gn.dat");
    assert(stored_constituents.size() == 1 && stored_constituents[0].code == "600000");
    std::cout << "PASSED" << std::endl;
#else
    std::cout << "Skipping DuckDB storage tests (DuckDB C API not configured)." << std::endl;
#endif
}

int main() {
    std::cout << "=== Running TDX API Tests ===" << std::endl;
    
    test_category_enum();
    test_market_enum();
    test_make_get_security_list_request();
    test_make_get_bars_request();
    test_make_get_snap_request();
    test_make_get_finance_info_request();
    test_parse_security_list();
    test_parse_kline();
    test_parse_finance_info();
    test_exhq_requests_and_parsers();
    test_block_info_requests_and_parsers();
    test_hq_intraday_company_and_xdxr_parsers();
    test_duckdb_market_store_roundtrip();
    
    std::cout << "=== All Tests Passed ===" << std::endl;
    return 0;
}
