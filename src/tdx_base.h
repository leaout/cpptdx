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
struct Kline {
  double open = 0;
  double close = 0;
  double high = 0;
  double low = 0;
  long long vol = 0;
  long long amount = 0;
  long long datetime = 0;
};

}  // namespace cpptdx
