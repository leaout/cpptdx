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
// 0 -   5 分钟K 线
// 1 -   15 分钟K 线
// 2 -   30 分钟K 线
// 3 -   1 小时K 线
// 4 -   日K 线
// 5 -   周K 线
// 6 -   月K 线
// 7 -   1 分钟
// 8 -   1 分钟K 线
// 9 -   日K 线
// 10 -  季K 线
// 11 -  年K 线
enum class Category : unsigned short {
  k5Minute = 0,
  k15Minute = 1,
  k30Minute = 2,
  k1Hour = 3,
  kDay = 4,
  kWeek = 5,
  kMonth = 6,
  k1Minute = 7,
  k1MinuteK = 8,
  kDayK = 9,
  kQuarterK = 10,
  kYearK = 11,
};
// 0  # 深圳
// 1  # 上海
enum class Market : unsigned short {
  kSZ = 0,
  kSH = 1
};

}  // namespace cpptdx
