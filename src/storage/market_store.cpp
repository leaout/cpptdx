#include "market_store.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>

#ifdef CPPTDX_WITH_DUCKDB
#include <duckdb.h>
#endif

namespace cpptdx {

struct MarketStore::Impl {
    bool available = false;
    std::mutex mutex;
#ifdef CPPTDX_WITH_DUCKDB
    duckdb_database database = nullptr;
    duckdb_connection connection = nullptr;
#endif
};

MarketStore::MarketStore(const std::string& path) : impl_(new Impl()) {
#ifdef CPPTDX_WITH_DUCKDB
    const std::filesystem::path db_path(path);
    if (!db_path.parent_path().empty()) {
        std::filesystem::create_directories(db_path.parent_path());
    }
    if (duckdb_open(path.c_str(), &impl_->database) != DuckDBSuccess ||
        duckdb_connect(impl_->database, &impl_->connection) != DuckDBSuccess) {
        std::cerr << "Failed to open DuckDB market store at " << path << std::endl;
        return;
    }
    duckdb_result result{};
    for (const auto* table : {"klines", "index_klines"}) {
        const std::string schema = std::string("CREATE TABLE IF NOT EXISTS ") + table + " ("
            "market UTINYINT NOT NULL, category UTINYINT NOT NULL, code VARCHAR NOT NULL, "
            "datetime BIGINT NOT NULL, open DOUBLE, close DOUBLE, high DOUBLE, low DOUBLE, "
            "volume DOUBLE, amount DOUBLE, PRIMARY KEY (market, category, code, datetime))";
        if (duckdb_query(impl_->connection, schema.c_str(), &result) != DuckDBSuccess) {
            std::cerr << "Failed to create DuckDB table " << table << ": " << duckdb_result_error(&result) << std::endl;
            duckdb_destroy_result(&result);
            return;
        }
        duckdb_destroy_result(&result);
    }
    const char* extra_schemas[] = {
        "CREATE TABLE IF NOT EXISTS minute_records (market UTINYINT, code VARCHAR, trade_date INTEGER, sample_index INTEGER, price DOUBLE, volume BIGINT, PRIMARY KEY(market, code, trade_date, sample_index))",
        "CREATE TABLE IF NOT EXISTS transactions (market UTINYINT, code VARCHAR, trade_date INTEGER, tick_index INTEGER, time VARCHAR, price DOUBLE, volume BIGINT, num INTEGER, buyorsell INTEGER, PRIMARY KEY(market, code, trade_date, tick_index))",
        "CREATE TABLE IF NOT EXISTS xdxr_records (market UTINYINT, code VARCHAR, trade_date INTEGER, category UTINYINT, event_index INTEGER, record_json JSON, PRIMARY KEY(market, code, trade_date, event_index))",
        "CREATE TABLE IF NOT EXISTS finance_info (market UTINYINT, code VARCHAR, record_json JSON, fetched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY(market, code))",
        "CREATE TABLE IF NOT EXISTS company_info_catalog (market UTINYINT, code VARCHAR, filename VARCHAR, name VARCHAR, start BIGINT, length BIGINT, PRIMARY KEY(market, code, filename))",
        "CREATE TABLE IF NOT EXISTS company_info_content (market UTINYINT, code VARCHAR, filename VARCHAR, start BIGINT, length BIGINT, content VARCHAR, PRIMARY KEY(market, code, filename, start, length))",
        "CREATE TABLE IF NOT EXISTS exhq_markets (market UTINYINT, category UTINYINT, record_json JSON, PRIMARY KEY(market, category))",
        "CREATE TABLE IF NOT EXISTS exhq_instruments (market UTINYINT, category UTINYINT, code VARCHAR, record_json JSON, PRIMARY KEY(market, category, code))",
        "CREATE TABLE IF NOT EXISTS security_list (market UTINYINT, start_index INTEGER, code VARCHAR, record_json JSON, PRIMARY KEY(market, start_index))",
        "CREATE TABLE IF NOT EXISTS security_snapshots (market UTINYINT, code VARCHAR, record_json JSON, updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY(market, code))",
        "CREATE TABLE IF NOT EXISTS block_files (filename VARCHAR PRIMARY KEY, size BIGINT, hash_value VARCHAR, content_hex VARCHAR, constituents JSON, fetched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS exhq_data (kind VARCHAR, market UTINYINT, code VARCHAR, trade_date INTEGER, record_index INTEGER, record_json JSON, PRIMARY KEY(kind, market, code, trade_date, record_index))"
    };
    for (const auto* schema : extra_schemas) {
        result = {};
        if (duckdb_query(impl_->connection, schema, &result) != DuckDBSuccess) {
            std::cerr << "Failed to create DuckDB intraday table: " << duckdb_result_error(&result) << std::endl;
            duckdb_destroy_result(&result);
            return;
        }
        duckdb_destroy_result(&result);
    }
    impl_->available = true;
#else
    (void)path;
    std::cerr << "DuckDB support is unavailable; install libduckdb and configure DUCKDB_ROOT." << std::endl;
#endif
}

MarketStore::~MarketStore() {
#ifdef CPPTDX_WITH_DUCKDB
    if (impl_->connection) duckdb_disconnect(&impl_->connection);
    if (impl_->database) duckdb_close(&impl_->database);
#endif
}

bool MarketStore::available() const { return impl_->available; }

bool MarketStore::save_security_list(Market market, std::uint32_t start,
                                     const std::vector<SecurityInfo>& securities) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto execute_sql = [&](const char* sql) {
        duckdb_result result{};
        const auto state = duckdb_query(impl_->connection, sql, &result);
        duckdb_destroy_result(&result);
        return state == DuckDBSuccess;
    };
    if (!execute_sql("BEGIN TRANSACTION")) return false;
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO security_list VALUES (?, ?, ?, ?::JSON) "
                      "ON CONFLICT(market, start_index) DO UPDATE SET code=excluded.code, record_json=excluded.record_json";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        execute_sql("ROLLBACK");
        return false;
    }
    bool ok = true;
    for (size_t i = 0; i < securities.size(); ++i) {
        const auto serialized = nlohmann::json(securities[i]).dump();
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
        duckdb_bind_uint32(statement, 2, start + static_cast<std::uint32_t>(i));
        duckdb_bind_varchar(statement, 3, securities[i].code.c_str());
        duckdb_bind_varchar(statement, 4, serialized.c_str());
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    const bool transaction_ok = execute_sql(ok ? "COMMIT" : "ROLLBACK");
    return ok && transaction_ok;
#else
    (void)market; (void)start; (void)securities;
    return false;
#endif
}

std::vector<SecurityInfo> MarketStore::get_security_list(Market market, std::uint32_t start,
                                                         std::uint32_t count) {
    std::vector<SecurityInfo> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT record_json FROM security_list WHERE market=? AND start_index>=? "
                      "ORDER BY start_index LIMIT ?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        return rows;
    }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_uint32(statement, 2, start);
    duckdb_bind_uint32(statement, 3, count);
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto row_count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(row_count));
        for (idx_t row = 0; row < row_count; ++row) {
            char* raw = duckdb_value_varchar(&result, 0, row);
            if (!raw) continue;
            try { rows.push_back(nlohmann::json::parse(raw).get<SecurityInfo>()); } catch (const std::exception&) {}
            duckdb_free(raw);
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)market; (void)start; (void)count;
#endif
    return rows;
}

std::uint64_t MarketStore::get_security_count(Market market) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return 0;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    if (duckdb_prepare(impl_->connection, "SELECT COUNT(*) FROM security_list WHERE market=?", &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        return 0;
    }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_result result{};
    std::uint64_t count = 0;
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess && duckdb_row_count(&result) > 0) {
        count = duckdb_value_uint64(&result, 0, 0);
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
    return count;
#else
    (void)market;
    return 0;
#endif
}

bool MarketStore::save_klines(Market market, Category category, const std::string& code,
                              const std::vector<Kline>& klines) {
    return save_klines_table("klines", market, category, code, klines);
}

bool MarketStore::save_index_klines(Market market, Category category, const std::string& code,
                                    const std::vector<Kline>& klines) {
    return save_klines_table("index_klines", market, category, code, klines);
}

bool MarketStore::save_klines_table(const char* table, Market market, Category category,
                                    const std::string& code, const std::vector<Kline>& klines) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const std::string sql = std::string("INSERT INTO ") + table + " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT (market, category, code, datetime) DO UPDATE SET "
        "open=excluded.open, close=excluded.close, high=excluded.high, low=excluded.low, "
        "volume=excluded.volume, amount=excluded.amount";
    if (duckdb_prepare(impl_->connection, sql.c_str(), &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        return false;
    }
    duckdb_result transaction_result{};
    if (duckdb_query(impl_->connection, "BEGIN TRANSACTION", &transaction_result) != DuckDBSuccess) {
        duckdb_destroy_result(&transaction_result);
        duckdb_destroy_prepare(&statement);
        return false;
    }
    duckdb_destroy_result(&transaction_result);
    bool ok = true;
    for (const auto& kline : klines) {
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
        duckdb_bind_int8(statement, 2, static_cast<int8_t>(category));
        duckdb_bind_varchar(statement, 3, code.c_str());
        duckdb_bind_int64(statement, 4, kline.datetime);
        duckdb_bind_double(statement, 5, kline.open);
        duckdb_bind_double(statement, 6, kline.close);
        duckdb_bind_double(statement, 7, kline.high);
        duckdb_bind_double(statement, 8, kline.low);
        duckdb_bind_double(statement, 9, kline.vol);
        duckdb_bind_double(statement, 10, kline.amount);
        if (duckdb_execute_prepared(statement, nullptr) != DuckDBSuccess) ok = false;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    transaction_result = {};
    duckdb_query(impl_->connection, ok ? "COMMIT" : "ROLLBACK", &transaction_result);
    duckdb_destroy_result(&transaction_result);
    return ok;
#else
    (void)table; (void)market; (void)category; (void)code; (void)klines;
    return false;
#endif
}

std::vector<Kline> MarketStore::get_klines(Market market, Category category, const std::string& code,
                                           std::uint32_t limit, std::uint32_t offset) {
    return get_klines_table("klines", market, category, code, limit, offset);
}

std::vector<Kline> MarketStore::get_index_klines(Market market, Category category, const std::string& code,
                                                 std::uint32_t limit, std::uint32_t offset) {
    return get_klines_table("index_klines", market, category, code, limit, offset);
}

std::vector<Kline> MarketStore::get_daily_klines(Market market, const std::string& code,
                                                 std::uint32_t start_date, std::uint32_t end_date) {
    std::vector<Kline> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT datetime, open, close, high, low, volume, amount FROM klines "
                      "WHERE market=? AND category=9 AND code=? AND datetime BETWEEN ? AND ? "
                      "ORDER BY datetime ASC";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        return rows;
    }
    const auto first = static_cast<int64_t>(start_date) * 100;
    const auto last = static_cast<int64_t>(end_date) * 100 + 99;
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_bind_int64(statement, 3, first);
    duckdb_bind_int64(statement, 4, last);
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            Kline kline;
            kline.datetime = duckdb_value_int64(&result, 0, row);
            kline.open = duckdb_value_double(&result, 1, row);
            kline.close = duckdb_value_double(&result, 2, row);
            kline.high = duckdb_value_double(&result, 3, row);
            kline.low = duckdb_value_double(&result, 4, row);
            kline.vol = duckdb_value_double(&result, 5, row);
            kline.amount = duckdb_value_double(&result, 6, row);
            rows.push_back(kline);
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)market; (void)code; (void)start_date; (void)end_date;
#endif
    return rows;
}

std::vector<Kline> MarketStore::get_klines_table(const char* table, Market market, Category category,
                                                 const std::string& code, std::uint32_t limit,
                                                 std::uint32_t offset) {
    std::vector<Kline> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const std::string sql = std::string("SELECT datetime, open, close, high, low, volume, amount FROM ") + table +
                      " WHERE market=? AND category=? AND code=? ORDER BY datetime DESC LIMIT ? OFFSET ?";
    if (duckdb_prepare(impl_->connection, sql.c_str(), &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        return rows;
    }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_int8(statement, 2, static_cast<int8_t>(category));
    duckdb_bind_varchar(statement, 3, code.c_str());
    duckdb_bind_uint32(statement, 4, limit);
    duckdb_bind_uint32(statement, 5, offset);
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            Kline kline;
            kline.datetime = duckdb_value_int64(&result, 0, row);
            kline.open = duckdb_value_double(&result, 1, row);
            kline.close = duckdb_value_double(&result, 2, row);
            kline.high = duckdb_value_double(&result, 3, row);
            kline.low = duckdb_value_double(&result, 4, row);
            kline.vol = duckdb_value_double(&result, 5, row);
            kline.amount = duckdb_value_double(&result, 6, row);
            rows.push_back(kline);
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)table; (void)market; (void)category; (void)code; (void)limit; (void)offset;
#endif
    return rows;
}

bool MarketStore::save_minute_data(Market market, const std::string& code, std::uint32_t date,
                                   const std::vector<MinuteRecord>& records) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto execute_sql = [&](const char* sql) {
        duckdb_result result{};
        const auto state = duckdb_query(impl_->connection, sql, &result);
        duckdb_destroy_result(&result);
        return state == DuckDBSuccess;
    };
    if (!execute_sql("BEGIN TRANSACTION")) return false;
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO minute_records VALUES (?, ?, ?, ?, ?, ?) ON CONFLICT DO UPDATE SET price=excluded.price, volume=excluded.volume";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        execute_sql("ROLLBACK");
        return false;
    }
    bool ok = true;
    for (size_t i = 0; i < records.size(); ++i) {
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
        duckdb_bind_varchar(statement, 2, code.c_str());
        duckdb_bind_uint32(statement, 3, date);
        duckdb_bind_uint32(statement, 4, static_cast<uint32_t>(i));
        duckdb_bind_double(statement, 5, records[i].price);
        duckdb_bind_int64(statement, 6, records[i].vol);
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    const bool transaction_ok = execute_sql(ok ? "COMMIT" : "ROLLBACK");
    return ok && transaction_ok;
#else
    (void)market; (void)code; (void)date; (void)records;
    return false;
#endif
}

bool MarketStore::save_transactions(Market market, const std::string& code, std::uint32_t date,
                                    std::uint32_t start, const std::vector<TickRecord>& records) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto execute_sql = [&](const char* sql) {
        duckdb_result result{};
        const auto state = duckdb_query(impl_->connection, sql, &result);
        duckdb_destroy_result(&result);
        return state == DuckDBSuccess;
    };
    if (!execute_sql("BEGIN TRANSACTION")) return false;
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO transactions VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) ON CONFLICT DO UPDATE SET time=excluded.time, price=excluded.price, volume=excluded.volume, num=excluded.num, buyorsell=excluded.buyorsell";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        execute_sql("ROLLBACK");
        return false;
    }
    bool ok = true;
    for (size_t i = 0; i < records.size(); ++i) {
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
        duckdb_bind_varchar(statement, 2, code.c_str());
        duckdb_bind_uint32(statement, 3, date);
        duckdb_bind_uint32(statement, 4, start + static_cast<uint32_t>(i));
        duckdb_bind_varchar(statement, 5, records[i].time.c_str());
        duckdb_bind_double(statement, 6, records[i].price);
        duckdb_bind_int64(statement, 7, records[i].vol);
        duckdb_bind_int32(statement, 8, records[i].num);
        duckdb_bind_int32(statement, 9, records[i].buyorsell);
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    const bool transaction_ok = execute_sql(ok ? "COMMIT" : "ROLLBACK");
    return ok && transaction_ok;
#else
    (void)market; (void)code; (void)date; (void)start; (void)records;
    return false;
#endif
}

std::vector<MinuteRecord> MarketStore::get_minute_data(Market market, const std::string& code,
                                                        std::uint32_t date, std::uint32_t limit,
                                                        std::uint32_t offset) {
    std::vector<MinuteRecord> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT price, volume FROM minute_records WHERE market=? AND code=? AND trade_date=? ORDER BY sample_index LIMIT ? OFFSET ?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return rows; }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_bind_uint32(statement, 3, date);
    duckdb_bind_uint32(statement, 4, limit);
    duckdb_bind_uint32(statement, 5, offset);
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            rows.push_back({duckdb_value_double(&result, 0, row), duckdb_value_int64(&result, 1, row)});
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)market; (void)code; (void)date; (void)limit; (void)offset;
#endif
    return rows;
}

std::vector<TickRecord> MarketStore::get_transactions(Market market, const std::string& code,
                                                       std::uint32_t date, std::uint32_t limit,
                                                       std::uint32_t offset) {
    std::vector<TickRecord> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT time, price, volume, num, buyorsell FROM transactions WHERE market=? AND code=? AND trade_date=? ORDER BY tick_index LIMIT ? OFFSET ?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return rows; }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_bind_uint32(statement, 3, date);
    duckdb_bind_uint32(statement, 4, limit);
    duckdb_bind_uint32(statement, 5, offset);
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            const char* time = duckdb_value_varchar(&result, 0, row);
            rows.push_back({time ? time : "", duckdb_value_double(&result, 1, row),
                            duckdb_value_int64(&result, 2, row), duckdb_value_int32(&result, 3, row),
                            duckdb_value_int32(&result, 4, row)});
            if (time) duckdb_free(const_cast<char*>(time));
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)market; (void)code; (void)date; (void)limit; (void)offset;
#endif
    return rows;
}

bool MarketStore::save_xdxr_info(Market market, const std::string& code, const std::vector<XdxrRecord>& records) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto execute_sql = [&](const char* sql) {
        duckdb_result result{};
        const auto state = duckdb_query(impl_->connection, sql, &result);
        duckdb_destroy_result(&result);
        return state == DuckDBSuccess;
    };
    if (!execute_sql("BEGIN TRANSACTION")) return false;
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO xdxr_records VALUES (?, ?, ?, ?, ?, ?::JSON) ON CONFLICT DO UPDATE SET category=excluded.category, record_json=excluded.record_json";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        execute_sql("ROLLBACK");
        return false;
    }
    bool ok = true;
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& record = records[i];
        const uint32_t date = static_cast<uint32_t>(record.year * 10000 + record.month * 100 + record.day);
        const auto json = nlohmann::json(record).dump();
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
        duckdb_bind_varchar(statement, 2, code.c_str());
        duckdb_bind_uint32(statement, 3, date);
        duckdb_bind_int8(statement, 4, static_cast<int8_t>(record.category));
        duckdb_bind_uint32(statement, 5, static_cast<uint32_t>(i));
        duckdb_bind_varchar(statement, 6, json.c_str());
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    const bool transaction_ok = execute_sql(ok ? "COMMIT" : "ROLLBACK");
    return ok && transaction_ok;
#else
    (void)market; (void)code; (void)records;
    return false;
#endif
}

std::vector<XdxrRecord> MarketStore::get_xdxr_info(Market market, const std::string& code) {
    std::vector<XdxrRecord> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT record_json FROM xdxr_records WHERE market=? AND code=? ORDER BY trade_date, event_index";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return rows; }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            char* raw = duckdb_value_varchar(&result, 0, row);
            if (!raw) continue;
            try {
                auto json = nlohmann::json::parse(raw);
                XdxrRecord item;
                item.year = json.value("year", 0); item.month = json.value("month", 0); item.day = json.value("day", 0);
                item.category = json.value("category", 0); item.name = json.value("name", "");
                auto set = [&](const char* key, std::optional<double>& field) { if (!json[key].is_null()) field = json[key].get<double>(); };
                set("fenhong", item.fenhong); set("peigujia", item.peigujia); set("songzhuangu", item.songzhuangu);
                set("peigu", item.peigu); set("suogu", item.suogu); set("panqianliutong", item.panqianliutong);
                set("panhouliutong", item.panhouliutong); set("qianzongguben", item.qianzongguben);
                set("houzongguben", item.houzongguben); set("fenshu", item.fenshu); set("xingquanjia", item.xingquanjia);
                rows.push_back(std::move(item));
            } catch (const nlohmann::json::exception&) {}
            duckdb_free(raw);
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)market; (void)code;
#endif
    return rows;
}

bool MarketStore::save_finance_info(Market market, const std::string& code, const std::string& json) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO finance_info(market, code, record_json) VALUES (?, ?, ?::JSON) ON CONFLICT(market, code) DO UPDATE SET record_json=excluded.record_json";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return false; }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_bind_varchar(statement, 3, json.c_str());
    const bool ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
    duckdb_destroy_prepare(&statement);
    return ok;
#else
    (void)market; (void)code; (void)json;
    return false;
#endif
}

std::string MarketStore::get_finance_info(Market market, const std::string& code) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return {};
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT record_json FROM finance_info WHERE market=? AND code=?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return {}; }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_result result{};
    std::string json;
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess && duckdb_row_count(&result) > 0) {
        char* raw = duckdb_value_varchar(&result, 0, 0);
        if (raw) { json = raw; duckdb_free(raw); }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
    return json;
#else
    (void)market; (void)code;
    return {};
#endif
}

bool MarketStore::save_company_info_category(Market market, const std::string& code,
                                             const std::vector<CompanyInfoFile>& files) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO company_info_catalog VALUES (?, ?, ?, ?, ?, ?) ON CONFLICT DO UPDATE SET name=excluded.name, start=excluded.start, length=excluded.length";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return false; }
    bool ok = true;
    for (const auto& file : files) {
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
        duckdb_bind_varchar(statement, 2, code.c_str());
        duckdb_bind_varchar(statement, 3, file.filename.c_str());
        duckdb_bind_varchar(statement, 4, file.name.c_str());
        duckdb_bind_uint32(statement, 5, file.start);
        duckdb_bind_uint32(statement, 6, file.length);
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    return ok;
#else
    (void)market; (void)code; (void)files;
    return false;
#endif
}

std::vector<CompanyInfoFile> MarketStore::get_company_info_category(Market market, const std::string& code) {
    std::vector<CompanyInfoFile> files;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return files;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT name, filename, start, length FROM company_info_catalog WHERE market=? AND code=? ORDER BY filename";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return files; }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        files.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            char* name = duckdb_value_varchar(&result, 0, row);
            char* filename = duckdb_value_varchar(&result, 1, row);
            files.push_back({name ? name : "", filename ? filename : "",
                static_cast<uint32_t>(duckdb_value_int64(&result, 2, row)),
                static_cast<uint32_t>(duckdb_value_int64(&result, 3, row))});
            if (name) duckdb_free(name);
            if (filename) duckdb_free(filename);
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)market; (void)code;
#endif
    return files;
}

bool MarketStore::save_company_info_content(Market market, const std::string& code, const std::string& filename,
                                            std::uint32_t start, std::uint32_t length, const std::string& content) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO company_info_content VALUES (?, ?, ?, ?, ?, ?) ON CONFLICT DO UPDATE SET content=excluded.content";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return false; }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_bind_varchar(statement, 3, filename.c_str());
    duckdb_bind_uint32(statement, 4, start);
    duckdb_bind_uint32(statement, 5, length);
    duckdb_bind_varchar_length(statement, 6, content.data(), content.size());
    const bool ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
    duckdb_destroy_prepare(&statement);
    return ok;
#else
    (void)market; (void)code; (void)filename; (void)start; (void)length; (void)content;
    return false;
#endif
}

std::string MarketStore::get_company_info_content(Market market, const std::string& code, const std::string& filename,
                                                   std::uint32_t start, std::uint32_t length) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return {};
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT content FROM company_info_content WHERE market=? AND code=? AND filename=? AND start=? AND length=?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return {}; }
    duckdb_bind_int8(statement, 1, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 2, code.c_str());
    duckdb_bind_varchar(statement, 3, filename.c_str());
    duckdb_bind_uint32(statement, 4, start);
    duckdb_bind_uint32(statement, 5, length);
    duckdb_result result{};
    std::string content;
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess && duckdb_row_count(&result) > 0) {
        char* raw = duckdb_value_varchar(&result, 0, 0);
        if (raw) { content = raw; duckdb_free(raw); }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
    return content;
#else
    (void)market; (void)code; (void)filename; (void)start; (void)length;
    return {};
#endif
}

bool MarketStore::save_ex_markets(const std::vector<ExMarket>& markets) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto execute_sql = [&](const char* sql) {
        duckdb_result result{};
        const auto state = duckdb_query(impl_->connection, sql, &result);
        duckdb_destroy_result(&result);
        return state == DuckDBSuccess;
    };
    if (!execute_sql("BEGIN TRANSACTION")) return false;
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO exhq_markets VALUES (?, ?, ?::JSON) ON CONFLICT(market, category) DO UPDATE SET record_json=excluded.record_json";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        execute_sql("ROLLBACK");
        return false;
    }
    bool ok = true;
    for (const auto& item : markets) {
        const auto json = nlohmann::json(item).dump();
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(item.market));
        duckdb_bind_int8(statement, 2, static_cast<int8_t>(item.category));
        duckdb_bind_varchar(statement, 3, json.c_str());
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    const bool transaction_ok = execute_sql(ok ? "COMMIT" : "ROLLBACK");
    return ok && transaction_ok;
#else
    (void)markets;
    return false;
#endif
}

std::vector<ExMarket> MarketStore::get_ex_markets() {
    std::vector<ExMarket> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_result result{};
    if (duckdb_query(impl_->connection, "SELECT record_json FROM exhq_markets ORDER BY market, category", &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            char* raw = duckdb_value_varchar(&result, 0, row);
            if (!raw) continue;
            try { rows.push_back(nlohmann::json::parse(raw).get<ExMarket>()); } catch (...) {}
            duckdb_free(raw);
        }
    }
    duckdb_destroy_result(&result);
#endif
    return rows;
}

bool MarketStore::save_ex_instruments(const std::vector<ExInstrument>& instruments) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto execute_sql = [&](const char* sql) {
        duckdb_result result{};
        const auto state = duckdb_query(impl_->connection, sql, &result);
        duckdb_destroy_result(&result);
        return state == DuckDBSuccess;
    };
    if (!execute_sql("BEGIN TRANSACTION")) return false;
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO exhq_instruments VALUES (?, ?, ?, ?::JSON) ON CONFLICT(market, category, code) DO UPDATE SET record_json=excluded.record_json";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        execute_sql("ROLLBACK");
        return false;
    }
    bool ok = true;
    for (const auto& item : instruments) {
        const auto json = nlohmann::json(item).dump();
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(item.market));
        duckdb_bind_int8(statement, 2, static_cast<int8_t>(item.category));
        duckdb_bind_varchar(statement, 3, item.code.c_str());
        duckdb_bind_varchar(statement, 4, json.c_str());
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    const bool transaction_ok = execute_sql(ok ? "COMMIT" : "ROLLBACK");
    return ok && transaction_ok;
#else
    (void)instruments;
    return false;
#endif
}

std::vector<ExInstrument> MarketStore::get_ex_instruments(std::uint32_t limit, std::uint32_t offset) {
    std::vector<ExInstrument> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT record_json FROM exhq_instruments ORDER BY market, category, code LIMIT ? OFFSET ?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return rows; }
    duckdb_bind_uint32(statement, 1, limit);
    duckdb_bind_uint32(statement, 2, offset);
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            char* raw = duckdb_value_varchar(&result, 0, row);
            if (!raw) continue;
            try { rows.push_back(nlohmann::json::parse(raw).get<ExInstrument>()); } catch (...) {}
            duckdb_free(raw);
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)limit; (void)offset;
#endif
    return rows;
}

std::uint64_t MarketStore::get_ex_instrument_count() {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return 0;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_result result{};
    std::uint64_t count = 0;
    if (duckdb_query(impl_->connection, "SELECT COUNT(*) FROM exhq_instruments", &result) == DuckDBSuccess &&
        duckdb_row_count(&result) > 0) count = duckdb_value_uint64(&result, 0, 0);
    duckdb_destroy_result(&result);
    return count;
#else
    return 0;
#endif
}

bool MarketStore::save_snapshots(const std::vector<SnapShot>& snapshots) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO security_snapshots(market, code, record_json) VALUES (?, ?, ?::JSON) "
                      "ON CONFLICT(market, code) DO UPDATE SET record_json=excluded.record_json, updated_at=CURRENT_TIMESTAMP";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return false; }
    bool ok = true;
    for (const auto& item : snapshots) {
        const auto json = nlohmann::json(item).dump();
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(item.market));
        duckdb_bind_varchar(statement, 2, item.code.c_str());
        duckdb_bind_varchar(statement, 3, json.c_str());
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    return ok;
#else
    (void)snapshots;
    return false;
#endif
}

std::vector<SnapShot> MarketStore::get_snapshots(const std::vector<std::pair<std::string, Market>>& stocks) {
    std::vector<SnapShot> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available || stocks.empty()) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT record_json FROM security_snapshots WHERE market=? AND code=?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return rows; }
    for (const auto& stock : stocks) {
        duckdb_bind_int8(statement, 1, static_cast<int8_t>(stock.second));
        duckdb_bind_varchar(statement, 2, stock.first.c_str());
        duckdb_result result{};
        if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess && duckdb_row_count(&result) > 0) {
            char* raw = duckdb_value_varchar(&result, 0, 0);
            if (raw) {
                try { rows.push_back(nlohmann::json::parse(raw).get<SnapShot>()); } catch (const std::exception&) {}
                duckdb_free(raw);
            }
        }
        duckdb_destroy_result(&result);
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
#else
    (void)stocks;
#endif
    return rows;
}

bool MarketStore::save_block_file(const std::string& filename, const BlockInfoMeta& meta,
                                 const std::string& content,
                                 const std::vector<BlockConstituent>& constituents) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return false;
    static constexpr char hex[] = "0123456789abcdef";
    std::string content_hex;
    content_hex.reserve(content.size() * 2);
    for (unsigned char byte : content) {
        content_hex.push_back(hex[byte >> 4]);
        content_hex.push_back(hex[byte & 0x0f]);
    }
    const auto parsed = nlohmann::json(constituents).dump();
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO block_files(filename, size, hash_value, content_hex, constituents) "
                      "VALUES (?, ?, ?, ?, ?::JSON) ON CONFLICT(filename) DO UPDATE SET size=excluded.size, "
                      "hash_value=excluded.hash_value, content_hex=excluded.content_hex, constituents=excluded.constituents";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        std::cerr << "Failed to prepare block-file upsert: " << duckdb_prepare_error(statement) << std::endl;
        duckdb_destroy_prepare(&statement);
        return false;
    }
    duckdb_bind_varchar(statement, 1, filename.c_str());
    duckdb_bind_uint32(statement, 2, meta.size);
    duckdb_bind_varchar(statement, 3, meta.hash_value.c_str());
    duckdb_bind_varchar_length(statement, 4, content_hex.data(), content_hex.size());
    duckdb_bind_varchar(statement, 5, parsed.c_str());
    duckdb_result result{};
    const bool ok = duckdb_execute_prepared(statement, &result) == DuckDBSuccess;
    if (!ok) std::cerr << "Failed to cache block file: " << duckdb_result_error(&result) << std::endl;
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
    return ok;
#else
    (void)filename; (void)meta; (void)content; (void)constituents;
    return false;
#endif
}

BlockInfoMeta MarketStore::get_block_info_meta(const std::string& filename) {
    BlockInfoMeta meta;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return meta;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    if (duckdb_prepare(impl_->connection, "SELECT size, hash_value FROM block_files WHERE filename=?", &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        return meta;
    }
    duckdb_bind_varchar(statement, 1, filename.c_str());
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess && duckdb_row_count(&result) > 0) {
        meta.size = static_cast<std::uint32_t>(duckdb_value_uint32(&result, 0, 0));
        char* raw = duckdb_value_varchar(&result, 1, 0);
        if (raw) { meta.hash_value = raw; duckdb_free(raw); }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)filename;
#endif
    return meta;
}

std::string MarketStore::get_block_info_content(const std::string& filename, std::uint32_t start,
                                                std::uint32_t size) {
    std::string content;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return content;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT substr(content_hex, ? + 1, ?) FROM block_files WHERE filename=?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return content; }
    duckdb_bind_int64(statement, 1, static_cast<std::int64_t>(start) * 2);
    duckdb_bind_int64(statement, 2, static_cast<std::int64_t>(size) * 2);
    duckdb_bind_varchar(statement, 3, filename.c_str());
    duckdb_result result{};
    const auto query_status = duckdb_execute_prepared(statement, &result);
    if (query_status != DuckDBSuccess) {
        std::cerr << "Failed to read block-file range: " << duckdb_result_error(&result) << std::endl;
    } else if (duckdb_row_count(&result) > 0) {
        char* raw = duckdb_value_varchar(&result, 0, 0);
        if (raw) {
            const std::string hex(raw);
            duckdb_free(raw);
            content.reserve(hex.size() / 2);
            for (size_t i = 0; i + 1 < hex.size(); i += 2) {
                content.push_back(static_cast<char>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16)));
            }
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)filename; (void)start; (void)size;
#endif
    return content;
}

std::vector<BlockConstituent> MarketStore::get_block_constituents(const std::string& filename) {
    std::vector<BlockConstituent> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    if (duckdb_prepare(impl_->connection, "SELECT constituents FROM block_files WHERE filename=?", &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        return rows;
    }
    duckdb_bind_varchar(statement, 1, filename.c_str());
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess && duckdb_row_count(&result) > 0) {
        char* raw = duckdb_value_varchar(&result, 0, 0);
        if (raw) {
            try { rows = nlohmann::json::parse(raw).get<std::vector<BlockConstituent>>(); } catch (const std::exception&) {}
            duckdb_free(raw);
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)filename;
#endif
    return rows;
}

bool MarketStore::save_ex_data(const std::string& kind, unsigned char market, const std::string& code,
                               std::uint32_t date, std::uint32_t start, const nlohmann::json& rows) {
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available || !rows.is_array()) return false;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto execute_sql = [&](const char* sql) {
        duckdb_result result{};
        const auto state = duckdb_query(impl_->connection, sql, &result);
        duckdb_destroy_result(&result);
        return state == DuckDBSuccess;
    };
    if (!execute_sql("BEGIN TRANSACTION")) return false;
    duckdb_prepared_statement statement;
    const char* sql = "INSERT INTO exhq_data VALUES (?, ?, ?, ?, ?, ?::JSON) ON CONFLICT(kind, market, code, trade_date, record_index) "
                      "DO UPDATE SET record_json=excluded.record_json";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) {
        duckdb_destroy_prepare(&statement);
        execute_sql("ROLLBACK");
        return false;
    }
    bool ok = true;
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto serialized = rows[i].dump();
        duckdb_bind_varchar(statement, 1, kind.c_str());
        duckdb_bind_int8(statement, 2, static_cast<int8_t>(market));
        duckdb_bind_varchar(statement, 3, code.c_str());
        duckdb_bind_uint32(statement, 4, date);
        duckdb_bind_uint32(statement, 5, start + static_cast<std::uint32_t>(i));
        duckdb_bind_varchar(statement, 6, serialized.c_str());
        ok = duckdb_execute_prepared(statement, nullptr) == DuckDBSuccess;
        if (!ok) break;
        duckdb_clear_bindings(statement);
    }
    duckdb_destroy_prepare(&statement);
    const bool transaction_ok = execute_sql(ok ? "COMMIT" : "ROLLBACK");
    return ok && transaction_ok;
#else
    (void)kind; (void)market; (void)code; (void)date; (void)start; (void)rows;
    return false;
#endif
}

std::vector<nlohmann::json> MarketStore::get_ex_data(const std::string& kind, unsigned char market,
    const std::string& code, std::uint32_t date, std::uint32_t limit, std::uint32_t offset) {
    std::vector<nlohmann::json> rows;
#ifdef CPPTDX_WITH_DUCKDB
    if (!impl_->available) return rows;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    duckdb_prepared_statement statement;
    const char* sql = "SELECT record_json FROM exhq_data WHERE kind=? AND market=? AND code=? AND trade_date=? "
                      "AND record_index>=? ORDER BY record_index LIMIT ?";
    if (duckdb_prepare(impl_->connection, sql, &statement) != DuckDBSuccess) { duckdb_destroy_prepare(&statement); return rows; }
    duckdb_bind_varchar(statement, 1, kind.c_str());
    duckdb_bind_int8(statement, 2, static_cast<int8_t>(market));
    duckdb_bind_varchar(statement, 3, code.c_str());
    duckdb_bind_uint32(statement, 4, date);
    duckdb_bind_uint32(statement, 5, offset);
    duckdb_bind_uint32(statement, 6, limit);
    duckdb_result result{};
    if (duckdb_execute_prepared(statement, &result) == DuckDBSuccess) {
        const auto count = duckdb_row_count(&result);
        rows.reserve(static_cast<size_t>(count));
        for (idx_t row = 0; row < count; ++row) {
            char* raw = duckdb_value_varchar(&result, 0, row);
            if (!raw) continue;
            try { rows.push_back(nlohmann::json::parse(raw)); } catch (const std::exception&) {}
            duckdb_free(raw);
        }
    }
    duckdb_destroy_result(&result);
    duckdb_destroy_prepare(&statement);
#else
    (void)kind; (void)market; (void)code; (void)date; (void)limit; (void)offset;
#endif
    return rows;
}

}  // namespace cpptdx
