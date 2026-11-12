#pragma once

#include "include/tdxhq_api.h"
#include "include/tdxexhq_api.h"
#include <cstdint>
#include <memory>
#include <string>

namespace cpptdx {

class MarketStore {
public:
    explicit MarketStore(const std::string& path);
    ~MarketStore();
    MarketStore(const MarketStore&) = delete;
    MarketStore& operator=(const MarketStore&) = delete;

    bool available() const;
    bool save_security_list(Market market, std::uint32_t start,
                            const std::vector<SecurityInfo>& securities);
    std::vector<SecurityInfo> get_security_list(Market market, std::uint32_t start,
                                                std::uint32_t count);
    std::uint64_t get_security_count(Market market);
    bool save_klines(Market market, Category category, const std::string& code,
                     const std::vector<Kline>& klines);
    bool save_index_klines(Market market, Category category, const std::string& code,
                           const std::vector<Kline>& klines);
    std::vector<Kline> get_klines(Market market, Category category, const std::string& code,
                                  std::uint32_t limit, std::uint32_t offset);
    std::vector<Kline> get_index_klines(Market market, Category category, const std::string& code,
                                        std::uint32_t limit, std::uint32_t offset);
    std::vector<Kline> get_daily_klines(Market market, const std::string& code,
                                        std::uint32_t start_date, std::uint32_t end_date);
    bool save_minute_data(Market market, const std::string& code, std::uint32_t date,
                          const std::vector<MinuteRecord>& records);
    bool save_transactions(Market market, const std::string& code, std::uint32_t date,
                           std::uint32_t start, const std::vector<TickRecord>& records);
    std::vector<MinuteRecord> get_minute_data(Market market, const std::string& code,
                                              std::uint32_t date, std::uint32_t limit,
                                              std::uint32_t offset);
    std::vector<TickRecord> get_transactions(Market market, const std::string& code,
                                             std::uint32_t date, std::uint32_t limit,
                                             std::uint32_t offset);
    bool save_xdxr_info(Market market, const std::string& code, const std::vector<XdxrRecord>& records);
    std::vector<XdxrRecord> get_xdxr_info(Market market, const std::string& code);
    bool save_finance_info(Market market, const std::string& code, const std::string& json);
    std::string get_finance_info(Market market, const std::string& code);
    bool save_company_info_category(Market market, const std::string& code, const std::vector<CompanyInfoFile>& files);
    std::vector<CompanyInfoFile> get_company_info_category(Market market, const std::string& code);
    bool save_company_info_content(Market market, const std::string& code, const std::string& filename,
                                   std::uint32_t start, std::uint32_t length, const std::string& content);
    std::string get_company_info_content(Market market, const std::string& code, const std::string& filename,
                                         std::uint32_t start, std::uint32_t length);
    bool save_ex_markets(const std::vector<ExMarket>& markets);
    std::vector<ExMarket> get_ex_markets();
    bool save_ex_instruments(const std::vector<ExInstrument>& instruments);
    std::vector<ExInstrument> get_ex_instruments(std::uint32_t limit, std::uint32_t offset);
    std::uint64_t get_ex_instrument_count();
    bool save_snapshots(const std::vector<SnapShot>& snapshots);
    std::vector<SnapShot> get_snapshots(const std::vector<std::pair<std::string, Market>>& stocks);
    bool save_block_file(const std::string& filename, const BlockInfoMeta& meta, const std::string& content,
                         const std::vector<BlockConstituent>& constituents);
    BlockInfoMeta get_block_info_meta(const std::string& filename);
    std::string get_block_info_content(const std::string& filename, std::uint32_t start, std::uint32_t size);
    std::vector<BlockConstituent> get_block_constituents(const std::string& filename);
    bool save_ex_data(const std::string& kind, unsigned char market, const std::string& code,
                      std::uint32_t date, std::uint32_t start, const nlohmann::json& rows);
    std::vector<nlohmann::json> get_ex_data(const std::string& kind, unsigned char market,
        const std::string& code, std::uint32_t date, std::uint32_t limit, std::uint32_t offset);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool save_klines_table(const char* table, Market market, Category category,
                           const std::string& code, const std::vector<Kline>& klines);
    std::vector<Kline> get_klines_table(const char* table, Market market, Category category,
                                        const std::string& code, std::uint32_t limit,
                                        std::uint32_t offset);
};

}  // namespace cpptdx
