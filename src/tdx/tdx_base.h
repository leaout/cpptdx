#pragma once
namespace cpptdx {

const int kHeaderLen = 0x10;
#pragma pack(4)
struct resp_header {
    unsigned int v1 = 0;
    unsigned int v2 = 0;
    unsigned int v3 = 0;
    unsigned short zip_size = 0;
    unsigned short unzip_size = 0;
};
#pragma pack()

}  // namespace cpptdx
