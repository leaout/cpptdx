/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once
#include "utils.h"

namespace cpptdx {
inline std::vector<uint8_t> setup_cmd1() {
    const std::string cmd1 = "0c0218930001030003000d0001";
    return hexStringToByteArray(cmd1);
}

inline std::vector<uint8_t> setup_cmd2() {
    const std::string cmd2 = "0c0218940001030003000d0002";
    return hexStringToByteArray(cmd2);
}

inline std::vector<uint8_t> setup_cmd3() {
    const std::string cmd3 =
        "0c031899000120002000db0fd5d0c9ccd6a4a8af0000008fc22540130000d500c9ccbdf0"
        "d7ea00000002";
    return hexStringToByteArray(cmd3);
}

inline std::vector<uint8_t> get_stock_count_cmd(unsigned short market) {
    const std::string cmd1 = "0c0c186c0001080008004e04";
    const std::string cmd2 = "75c73301";
    auto vec1 = hexStringToByteArray(cmd1);
    vec1.emplace_back(((uint8_t *)&market)[0]);
    vec1.emplace_back(((uint8_t *)&market)[1]);
    auto vec2 = hexStringToByteArray(cmd2);
    vec1.insert(vec1.end(), vec2.begin(), vec2.end());
    return vec1;
}
}  // namespace cpptdx