#include "include/tdxhq_api.h"
#include "msg/get_security_list.h"
#include "msg/get_security_klines.h"
#include "msg/get_security_snapshot.h"
#include "msg/get_finance_info.h"
#include "tdx/tdx_base.h"
#include <cassert>
#include <iostream>
#include <cstring>

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
    
    std::cout << "=== All Tests Passed ===" << std::endl;
    return 0;
}
