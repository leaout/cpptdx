#pragma once
#include <zlib.h>
#include <cmath>
#include <cstring>
#include <tuple>
#include <vector>
#include "tdx_base.h"

namespace cpptdx {

inline bool unzip_data(const char *zip_data, unsigned long zip_len,
                unsigned long unzip_len, std::vector<char> &unzip_data) {
  unzip_data.resize(unzip_len);
  int nRet = uncompress((Byte *)unzip_data.data(), &unzip_len, (Byte *)zip_data,
                        zip_len);
  if (Z_OK != nRet) {
    return false;
  }
  return true;
}

float _cal_price1000(int base_p, int diff) {
  return (float)(base_p + diff) / 1000;
}
std::tuple<int, int, int, int, int, int16_t> get_datetime(int category,
                                                          const char *data,
                                                          int pos) {
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 15;
  int minute = 0;
  if (category < 4 || category == 7 || category == 8) {
    int zipday = 0;
    int tminutes = 0;
    memcpy(&zipday, data + pos, 2);
    memcpy(&tminutes, data + pos + 2, 2);
    year = (zipday >> 11) + 2004;
    month = int((zipday % 2048) / 100);
    day = (zipday % 2048) % 100;

    hour = int(tminutes / 60);
    minute = tminutes % 60;
  } else {
    int zipday = 0;
    memcpy(&zipday, data + pos, 4);

    year = int(zipday / 10000);
    month = int((zipday % 10000) / 100);
    day = zipday % 100;
  }
  pos += 4;

  return std::make_tuple(year, month, day, hour, minute, pos);
}

std::tuple<int, int> get_price(const char *data, int pos) {
  int pos_byte = 6;
  int bdata = (int)(unsigned char)data[pos];
  int intdata = bdata & 0x3f;
  bool sign = false;

  if (bdata & 0x40) {
    sign = true;
  }

  if (bdata & 0x80) {
    while (true) {
      pos++;
      bdata = (int)(unsigned char)data[pos];
      intdata += (bdata & 0x7f) << pos_byte;
      pos_byte += 7;

      if (bdata & 0x80) {
        continue;
      } else {
        break;
      }
    }
  }

  pos++;

  if (sign) {
    intdata = -intdata;
  }

  return std::make_tuple(intdata, pos);
}

double get_volume(int ivol) {
  int logpoint = ivol >> (8 * 3);
  int hheax = ivol >> (8 * 3);           // [3]
  int hleax = (ivol >> (8 * 2)) & 0xff;  // [2]
  int lheax = (ivol >> 8) & 0xff;        // [1]
  int lleax = ivol & 0xff;               // [0]

  double dbl_1 = 1.0;
  double dbl_2 = 2.0;
  double dbl_128 = 128.0;

  int dwEcx = logpoint * 2 - 0x7f;
  int dwEdx = logpoint * 2 - 0x86;
  int dwEsi = logpoint * 2 - 0x8e;
  int dwEax = logpoint * 2 - 0x96;
  int tmpEax = dwEcx < 0 ? -dwEcx : dwEcx;

  double dbl_xmm6 = 0.0;
  dbl_xmm6 = pow(2.0, tmpEax);
  if (dwEcx < 0) {
    dbl_xmm6 = 1.0 / dbl_xmm6;
  }

  double dbl_xmm4 = 0;
  if (hleax > 0x80) {
    double tmpdbl_xmm3 = 0.0;
    double tmpdbl_xmm1 = 0.0;
    int dwtmpeax = dwEdx + 1;
    tmpdbl_xmm3 = std::pow(2.0, dwtmpeax);
    double dbl_xmm0 = pow(2.0, dwEdx) * dbl_128;
    dbl_xmm0 += (hleax & 0x7f) * tmpdbl_xmm3;
    dbl_xmm4 = dbl_xmm0;
  } else {
    double dbl_xmm0 = 0.0;
    if (dwEdx >= 0) {
      dbl_xmm0 = pow(2.0, dwEdx) * hleax;
    } else {
      dbl_xmm0 = (1 / pow(2.0, dwEdx)) * hleax;
    }
    dbl_xmm4 = dbl_xmm0;
  }

  double dbl_xmm3 = pow(2.0, dwEsi) * lheax;
  double dbl_xmm1 = pow(2.0, dwEax) * lleax;
  if (hleax & 0x80) {
    dbl_xmm3 *= 2.0;
    dbl_xmm1 *= 2.0;
  }

  double dbl_ret = dbl_xmm6 + dbl_xmm4 + dbl_xmm3 + dbl_xmm1;
  return dbl_ret;
}

std::vector<uint8_t> hexStringToByteArray(const std::string &hexString) {
  std::vector<uint8_t> byteArray;

  // Loop through the hex string, two characters at a time
  for (size_t i = 0; i < hexString.length(); i += 2) {
    // Extract two characters representing a byte
    std::string byteString = hexString.substr(i, 2);

    // Convert the byte string to a uint8_t value
    uint8_t byteValue = static_cast<uint8_t>(stoi(byteString, nullptr, 16));

    // Add the byte to the byte array
    byteArray.push_back(byteValue);
  }

  return byteArray;
}

std::vector<Kline> parse_kline(const char *data, int len,
                               unsigned short category) {
  if (len <= 0) {
    return {};
  }
  std::vector<Kline> klines;
  int pos = 0;

  unsigned short ret_count;
  memcpy(&ret_count, data + pos, sizeof(ret_count));
  pos += sizeof(ret_count);

  int pre_diff_base = 0;
  for (int i = 0; i < ret_count; ++i) {
    Kline kline;
    int year, month, day, hour, minute;
    std::tie(year, month, day, hour, minute, pos) =
        get_datetime(category, data, pos);

    int price_open_diff;
    int price_close_diff;
    int price_high_diff;
    int price_low_diff;
    std::tie(price_open_diff, pos) = get_price(data, pos);
    std::tie(price_close_diff, pos) = get_price(data, pos);
    std::tie(price_high_diff, pos) = get_price(data, pos);
    std::tie(price_low_diff, pos) = get_price(data, pos);

    int vol_raw;
    memcpy(&vol_raw, data + pos, sizeof(vol_raw));
    kline.vol = get_volume(vol_raw);
    pos += 4;

    int dbvol_raw;
    memcpy(&dbvol_raw, data + pos, 4);
    kline.amount = get_volume(dbvol_raw);
    pos += 4;

    kline.open = _cal_price1000(price_open_diff, pre_diff_base);
    price_open_diff = price_open_diff + pre_diff_base;
    kline.close = _cal_price1000(price_open_diff, price_close_diff);
    kline.high = _cal_price1000(price_open_diff, price_high_diff);
    kline.low = _cal_price1000(price_open_diff, price_low_diff);

    pre_diff_base = price_open_diff + price_close_diff;
    std::vector<int> time_vec = {minute, hour, day,
                                 month, year};
    long long ff = 1;
    for (auto i : time_vec) {
      kline.datetime += ff * i;
      ff *= 100;
    }

    klines.emplace_back(kline);
  }

  return klines;
}

}  // namespace cpptdx
