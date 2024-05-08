/**
 * Created by chenly on 5/7/24.
 * Description
 **/
#pragma once
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "include/tdxhq_api.h"
#include "base_msg.h"

namespace cpptdx {
using namespace std;
// def setParams(self, market, start):
// pkg = bytearray.fromhex(u'0c 01 18 64 01 01 06 00 06 00 50 04')
// pkg_param = struct.pack("<HH", market, start)
// pkg.extend(pkg_param)
// self.send_pkg = pkg
inline string make_get_security_list_request(unsigned short market, unsigned short start) {
    const std::string cmd1 = "0c0118640101060006005004";
    auto vec1 = hexStringToByteArray(cmd1);
    string ret((char*)vec1.data(), vec1.size());
    ret.append((char *)&market, 2);
    ret.append((char *)&start, 2);
    return ret;
}
#pragma pack(1)
struct SecurityBinaryInfo {
    char code[6] = {};
    unsigned short volunit = 0;
    char name_bytes[8] = {};
    char reversed_bytes1[4] = {};
    char decimal_point = 0;
    unsigned int pre_close_raw = 0;
    char reversed_bytes2[4] = {};
};
#pragma pack()

inline std::vector<SecurityInfo> parse_security_list(const char *data, int len) {
    size_t pos = 0;
    unsigned short num = 0;
    std::memcpy(&num, data, sizeof(num));
    pos += 2;
    std::vector<SecurityInfo> ret;

    for (int i = 0; i < num; ++i) {
        SecurityBinaryInfo tmp;
        std::memcpy(&tmp, data+pos, 29);
        pos += 29;
        SecurityInfo security;
        security.code = std::string(tmp.code,6);
        security.volunit = tmp.volunit;
        security.name = GBKToUTF8(std::string(tmp.name_bytes, 8));
        security.decimal_point = tmp.decimal_point;
        security.pre_close = get_volume(tmp.pre_close_raw);
        ret.push_back(security);
    }
    return ret;
}

}  // namespace cpptdx