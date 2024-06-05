/**
 * Created by chenly on 5/9/24.
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

inline string make_get_finance_info_request(unsigned short market, const string &code) {
    const std::string cmd1 = "0c1f187600010b000b0010000100";
    auto vec1 = hexStringToByteArray(cmd1);
    string ret((char*)vec1.data(), vec1.size());
    char tmp_market = market;
    ret.append((char *)&tmp_market, 1);
    ret.append((char *)code.c_str(), code.length());
    return ret;
}

#pragma pack(1)
struct FinanceInfoBin {
    char market;
    char code[6];
    float liutongguben;
    unsigned short province;
    unsigned short industry;
    unsigned int updated_date;
    unsigned int ipo_date;
    float zongguben;
    float guojiagu;
    float faqirenfarengu;
    float farengu;
    float bgu;
    float hgu;
    float zhigonggu;
    float zongzichan;
    float liudongzichan;
    float gudingzichan;
    float wuxingzichan;
    float gudongrenshu;
    float liudongfuzhai;
    float changqifuzhai;
    float zibengongjijin;
    float jingzichan;
    float zhuyingshouru;
    float zhuyinglirun;
    float yingshouzhangkuan;
    float yingyelirun;
    float touzishouyu;
    float jingyingxianjinliu;
    float zongxianjinliu;
    float cunhuo;
    float lirunzonghe;
    float shuihoulirun;
    float jinglirun;
    float weifenlirun;
    float baoliu1;
    float baoliu2;
};
#pragma pack()

inline FinanceInfo parse_finance_info(const char *data, int len) {
    if(len == 2){
        return FinanceInfo();
    }
    size_t pos = 0;
    pos += 2; //skip num ,we only query 1 in this case
    FinanceInfoBin finance_bin;
    std::memcpy(&finance_bin, data+pos, sizeof(FinanceInfoBin));

    FinanceInfo ret;
    ret.code = std::string(finance_bin.code, 6);
    ret.market = (Market)finance_bin.market;
    std::memcpy(&(ret.liutongguben), &(finance_bin.liutongguben), sizeof(FinanceInfoBin)-7);

    ret.liutongguben = ret.liutongguben *10000;
    ret.zongguben = ret.zongguben *10000;
    ret.guojiagu = ret.guojiagu *10000;
    ret.faqirenfarengu = ret.faqirenfarengu *10000;
    ret.farengu = ret.farengu *10000;
    ret.bgu = ret.bgu *10000;
    ret.hgu = ret.hgu *10000;
    ret.zhigonggu = ret.zhigonggu *10000;
    ret.zongzichan = ret.zongzichan *10000;
    ret.liudongzichan = ret.liudongzichan *10000;
    ret.gudingzichan = ret.gudingzichan *10000;
    ret.wuxingzichan = ret.wuxingzichan *10000;
    ret.liudongfuzhai = ret.liudongfuzhai *10000;
    ret.changqifuzhai = ret.changqifuzhai *10000;
    ret.zibengongjijin = ret.zibengongjijin *10000;
    ret.jingzichan = ret.jingzichan *10000;
    ret.zhuyingshouru = ret.zhuyingshouru *10000;
    ret.zhuyinglirun = ret.zhuyinglirun *10000;
    ret.yingshouzhangkuan = ret.yingshouzhangkuan *10000;
    ret.yingyelirun = ret.yingyelirun *10000;
    ret.touzishouyu = ret.touzishouyu *10000;
    ret.jingyingxianjinliu = ret.jingyingxianjinliu *10000;
    ret.zongxianjinliu = ret.zongxianjinliu *10000;
    ret.cunhuo = ret.cunhuo *10000;
    ret.lirunzonghe = ret.lirunzonghe *10000;
    ret.shuihoulirun = ret.shuihoulirun *10000;
    ret.jinglirun = ret.jinglirun *10000;
    ret.weifenlirun = ret.weifenlirun *10000;
    return ret;
}

}