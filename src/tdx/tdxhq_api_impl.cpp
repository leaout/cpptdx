#include "tdxhq_api_impl.h"
#include "msg/get_security_klines.h"
#include "msg/get_security_snapshot.h"
#include "msg/get_security_list.h"
#include "msg/get_finance_info.h"
#include "msg/get_intraday_data.h"
#include "msg/get_company_info.h"
#include "msg/get_xdxr_info.h"
#include "msg/get_block_info.h"
#include "tdx_base.h"
#include "utils.h"
#include <array>
#include <algorithm>
#include <chrono>
#include <limits>

namespace cpptdx {

namespace {
void append_u16(std::vector<char>& out, std::uint16_t value) {
    out.push_back(static_cast<char>(value & 0xff));
    out.push_back(static_cast<char>((value >> 8) & 0xff));
}

void append_u32(std::vector<char>& out, std::uint32_t value) {
    for (unsigned int shift = 0; shift < 32; shift += 8)
        out.push_back(static_cast<char>((value >> shift) & 0xff));
}

std::uint16_t read_u16(const char* data) {
    return static_cast<std::uint16_t>(static_cast<unsigned char>(data[0])) |
        (static_cast<std::uint16_t>(static_cast<unsigned char>(data[1])) << 8);
}

std::uint32_t read_u32(const char* data) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(data[0])) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(data[1])) << 8) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(data[2])) << 16) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(data[3])) << 24);
}

bool read_round2_varint(const std::vector<char>& data, size_t& pos, std::int64_t& value) {
    if (pos >= data.size()) return false;
    const size_t start = pos;
    const auto first = static_cast<unsigned char>(data[pos++]);
    std::uint64_t magnitude = first & 0x3f;
    unsigned int shift = 6;
    auto byte = first;
    unsigned int groups = 1;
    while (byte & 0x80) {
        if (pos >= data.size() || groups++ > 10 || shift >= 64) return false;
        byte = static_cast<unsigned char>(data[pos++]);
        const auto part = static_cast<std::uint64_t>(byte & 0x7f);
        if (shift > 63 || part > (std::numeric_limits<std::uint64_t>::max() >> shift)) return false;
        magnitude |= part << shift;
        shift += 7;
    }
    if (magnitude > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return false;
    value = (static_cast<unsigned char>(data[start]) & 0x40) ? -static_cast<std::int64_t>(magnitude)
                                                              : static_cast<std::int64_t>(magnitude);
    return true;
}

double decode_compact_float(std::uint32_t value) {
    if (value == 0) return 0.0;
    const auto logpoint = static_cast<int>(value >> 24);
    const auto high_byte = static_cast<unsigned char>((value >> 16) & 0xff);
    const auto middle_byte = static_cast<unsigned char>((value >> 8) & 0xff);
    const auto low_byte = static_cast<unsigned char>(value & 0xff);
    const double base = std::exp2(logpoint * 2 - 0x7f);
    const double high = high_byte > 0x80 ? base * (64 + (high_byte & 0x7f)) / 64.0
                                         : base * high_byte / 128.0;
    const double scale = (high_byte & 0x80) ? 2.0 : 1.0;
    return base + high + base * middle_byte / 32768.0 * scale + base * low_byte / 8388608.0 * scale;
}

std::vector<Kline> parse_round2_klines(const std::vector<char>& data, unsigned short category,
                                       bool index) {
    if (data.size() < 2) return {};
    const auto count = read_u16(data.data());
    const size_t minimum_record = index ? 20 : 16;
    if (count > (data.size() - 2) / minimum_record) return {};
    std::vector<Kline> result;
    result.reserve(count);
    size_t pos = 2;
    std::int64_t previous_close = 0;
    for (std::uint16_t i = 0; i < count; ++i) {
        if (pos + 4 > data.size()) return {};
        Kline bar;
        if (category < 4 || category == 7 || category == 8) {
            const auto packed_date = read_u16(data.data() + pos);
            const auto minute_of_day = read_u16(data.data() + pos + 2);
            bar.year = (packed_date >> 11) + 2004;
            bar.month = (packed_date % 2048) / 100;
            bar.day = (packed_date % 2048) % 100;
            bar.hour = minute_of_day / 60;
            bar.minute = minute_of_day % 60;
        } else {
            const auto date = read_u32(data.data() + pos);
            bar.year = static_cast<int>(date / 10000);
            bar.month = static_cast<int>((date % 10000) / 100);
            bar.day = static_cast<int>(date % 100);
        }
        pos += 4;
        std::int64_t open_delta, close_delta, high_delta, low_delta;
        if (!read_round2_varint(data, pos, open_delta) || !read_round2_varint(data, pos, close_delta) ||
            !read_round2_varint(data, pos, high_delta) || !read_round2_varint(data, pos, low_delta) ||
            pos + 8 > data.size()) return {};
        const auto open = previous_close + open_delta;
        const auto close = open + close_delta;
        bar.open = open / 1000.0;
        bar.close = close / 1000.0;
        bar.high = (open + high_delta) / 1000.0;
        bar.low = (open + low_delta) / 1000.0;
        previous_close = close;
        bar.vol = decode_compact_float(read_u32(data.data() + pos));
        bar.amount = decode_compact_float(read_u32(data.data() + pos + 4));
        pos += 8;
        if (index) {
            if (pos + 4 > data.size()) return {};
            pos += 4; // index up/down counts are not part of the current public Kline model.
        }
        const int time_values[] = {bar.minute, bar.hour, bar.day, bar.month, bar.year};
        long long multiplier = 1;
        for (const auto value : time_values) {
            bar.datetime += multiplier * value;
            multiplier *= 100;
        }
        result.push_back(bar);
    }
    return pos == data.size() ? result : std::vector<Kline>{};
}

std::vector<SnapShot> parse_round2_snapshots(const std::vector<char>& data,
                                             const std::vector<std::pair<std::string, Market>>& codes) {
    if (data.size() < 4 || read_u16(data.data() + 2) != codes.size()) return {};
    const size_t data_start = 4;
    std::vector<size_t> starts;
    size_t search_from = data_start;
    for (const auto& [code, market] : codes) {
        std::array<char, 7> marker{};
        marker[0] = static_cast<char>(market);
        std::copy_n(code.data(), std::min<size_t>(code.size(), 6), marker.data() + 1);
        auto found = std::search(data.begin() + static_cast<std::ptrdiff_t>(search_from), data.end(),
                                 marker.begin(), marker.end());
        if (found == data.end()) return {};
        search_from = static_cast<size_t>(std::distance(data.begin(), found));
        starts.push_back(search_from);
        search_from += marker.size();
    }
    std::vector<SnapShot> result;
    result.reserve(codes.size());
    for (size_t i = 0; i < codes.size(); ++i) {
        const size_t end = i + 1 < starts.size() ? starts[i + 1] : data.size();
        std::vector<char> record(data.begin() + static_cast<std::ptrdiff_t>(starts[i]),
                                 data.begin() + static_cast<std::ptrdiff_t>(end));
        if (record.size() < 9) return {};
        SnapShot snapshot{};
        snapshot.market = static_cast<Market>(static_cast<unsigned char>(record[0]));
        snapshot.code.assign(record.data() + 1, 6);
        const auto code_end = snapshot.code.find('\0');
        if (code_end != std::string::npos) snapshot.code.resize(code_end);
        snapshot.active1 = read_u16(record.data() + 7);
        size_t pos = 9;
        std::int64_t price, pre_close, open, high, low, time, unknown1, volume, current_volume;
        if (!read_round2_varint(record, pos, price) || !read_round2_varint(record, pos, pre_close) ||
            !read_round2_varint(record, pos, open) || !read_round2_varint(record, pos, high) ||
            !read_round2_varint(record, pos, low) || !read_round2_varint(record, pos, time) ||
            !read_round2_varint(record, pos, unknown1) || !read_round2_varint(record, pos, volume) ||
            !read_round2_varint(record, pos, current_volume) || pos + 4 > record.size()) return {};
        const double price_value = price / 100.0;
        snapshot.price = price_value;
        snapshot.last_close = (price + pre_close * 10) / 100.0;
        snapshot.open = (price + open * 10) / 100.0;
        snapshot.high = (price + high * 10) / 100.0;
        snapshot.low = (price + low * 10) / 100.0;
        snapshot.reversed_bytes0 = time;
        snapshot.reversed_bytes1 = unknown1;
        snapshot.vol = volume;
        snapshot.cur_vol = current_volume;
        snapshot.amount = static_cast<long long>(decode_compact_float(read_u32(record.data() + pos)));
        pos += 4;
        std::int64_t inside, outside, unknown2, open_amount, bid, ask, bid_volume, ask_volume;
        if (!read_round2_varint(record, pos, inside) || !read_round2_varint(record, pos, outside) ||
            !read_round2_varint(record, pos, unknown2) || !read_round2_varint(record, pos, open_amount) ||
            !read_round2_varint(record, pos, bid) || !read_round2_varint(record, pos, ask) ||
            !read_round2_varint(record, pos, bid_volume) || !read_round2_varint(record, pos, ask_volume)) return {};
        snapshot.s_vol = inside;
        snapshot.b_vol = outside;
        snapshot.reversed_bytes2 = unknown2;
        snapshot.reversed_bytes3 = open_amount * 100;
        snapshot.bid1 = (price + bid * 10) / 100.0;
        snapshot.ask1 = (price + ask * 10) / 100.0;
        snapshot.bid_vol1 = bid_volume;
        snapshot.ask_vol1 = ask_volume;
        // Round2 records may include a short opaque trailer after the exposed fields.
        result.push_back(snapshot);
    }
    return result;
}

template <typename Initiate>
bool timed_transfer(boost::asio::io_context& io, tcp::socket& socket, Initiate initiate) {
    io.restart();
    boost::asio::steady_timer timer(io);
    bool succeeded = false;
    timer.expires_after(std::chrono::seconds(5));
    timer.async_wait([&](const boost::system::error_code& ec) {
        if (!ec) {
            boost::system::error_code ignored;
            socket.cancel(ignored);
        }
    });
    initiate([&](const boost::system::error_code& ec, std::size_t) {
        succeeded = !ec;
        timer.cancel();
    });
    io.run();
    return succeeded;
}
}

TdxHqApiImpl::~TdxHqApiImpl() {
    stop();
}

bool TdxHqApiImpl::connect(const string& host, const string& port) {
    tcp::resolver resolver(io_service_);
    boost::system::error_code ec;
    const auto endpoints = resolver.resolve(host, port, ec);
    if (ec) {
        std::cerr << "Resolve failed: " << ec.message() << std::endl;
        return false;
    }
    socket_.reset(new boost::asio::ip::tcp::socket(io_service_));
    boost::asio::connect(*socket_, endpoints, ec);
    if (ec) {
        std::cerr << "Connection failed: " << ec.message() << std::endl;
        socket_->close();
        return false;
    }
    // registry
    send(setup_cmd1());
    send(setup_cmd2());
    send(setup_cmd3());
    // Keep the legacy connection for APIs that still use its frame format, and
    // open a second connection because round2 requires its handshake as frame 1.
    round2_host_ = host;
    round2_port_ = port;
    connect_round2();
    // start heart beat thread
    is_running_ = true;
    std::thread th(&TdxHqApiImpl::heart_beat_thread, this);
    th_.swap(th);
    return true;
}

vector<Kline> TdxHqApiImpl::get_security_klines(Category cat, Market market, 
                                                const std::string& code, 
                                                unsigned short start, unsigned short count) {
    if (round2_socket_ && count > 0 && count <= 800) {
        unsigned short period = static_cast<unsigned short>(cat);
        if (period == 9) period = 4; // pytdx's kDayK alias is daily bars.
        std::vector<char> body;
        append_u16(body, static_cast<unsigned short>(market));
        std::array<char, 6> code_bytes{};
        std::copy_n(code.data(), std::min<size_t>(code.size(), code_bytes.size()), code_bytes.data());
        body.insert(body.end(), code_bytes.begin(), code_bytes.end());
        append_u16(body, period);
        append_u16(body, 1); // period parameter
        append_u16(body, start);
        append_u16(body, count);
        append_u16(body, 0); // no adjustment
        append_u32(body, 0); // no fixed-adjustment anchor
        body.insert(body.end(), 20, 0);
        auto response = send_round2(0x052d, body);
        auto bars = parse_round2_klines(response, period, false);
        if (!bars.empty()) return bars;
    }
    auto request = make_get_bars_request((unsigned short)cat, (unsigned short)market, code, start, count);
    std::vector<char> rev_data = send(request.data(), request.length());
    return parse_kline(rev_data.data(), rev_data.size(), (unsigned short)cat);
}

vector<Kline> TdxHqApiImpl::get_index_klines(Category cat, Market market,
                                              const std::string& code,
                                              unsigned short start, unsigned short count) {
    if (round2_socket_ && count > 0 && count <= 800) {
        unsigned short period = static_cast<unsigned short>(cat);
        if (period == 9) period = 4;
        std::vector<char> body;
        append_u16(body, static_cast<unsigned short>(market));
        std::array<char, 6> code_bytes{};
        std::copy_n(code.data(), std::min<size_t>(code.size(), code_bytes.size()), code_bytes.data());
        body.insert(body.end(), code_bytes.begin(), code_bytes.end());
        append_u16(body, period);
        append_u16(body, 1);
        append_u16(body, start);
        append_u16(body, count);
        append_u16(body, 0);
        append_u32(body, 0);
        body.insert(body.end(), 20, 0);
        auto response = send_round2(0x052d, body);
        auto bars = parse_round2_klines(response, period, true);
        if (!bars.empty()) return bars;
    }
    // pytdx GetIndexBarsCmd uses the same request and response encoding as GetSecurityBarsCmd.
    auto request = make_get_bars_request((unsigned short)cat, (unsigned short)market, code, start, count);
    auto response = send(request.data(), request.length());
    return parse_kline(response.data(), response.size(), (unsigned short)cat);
}

vector<MinuteRecord> TdxHqApiImpl::get_minute_time_data(Market market, const std::string& code) {
    const auto request = make_get_minute_time_request(market, code);
    const auto response = send(request.data(), request.size());
    return parse_minute_time_data(response.data(), response.size(), 4);
}

vector<MinuteRecord> TdxHqApiImpl::get_history_minute_time_data(Market market, const std::string& code,
                                                                 unsigned int date) {
    const auto request = make_get_history_minute_time_request(market, code, date);
    const auto response = send(request.data(), request.size());
    return parse_minute_time_data(response.data(), response.size(), 6);
}

vector<TickRecord> TdxHqApiImpl::get_transaction_data(Market market, const std::string& code,
                                                       unsigned short start, unsigned short count) {
    const auto request = make_get_transaction_request(market, code, start, count);
    const auto response = send(request.data(), request.size());
    return parse_transaction_data(response.data(), response.size(), false);
}

vector<HistoryTickRecord> TdxHqApiImpl::get_history_transaction_data(Market market, const std::string& code,
                                                                     unsigned short start, unsigned short count,
                                                                     unsigned int date) {
    const auto request = make_get_history_transaction_request(market, code, start, count, date);
    const auto response = send(request.data(), request.size());
    return to_history_ticks(parse_transaction_data(response.data(), response.size(), true));
}

vector<CompanyInfoFile> TdxHqApiImpl::get_company_info_category(Market market, const std::string& code) {
    const auto request = make_get_company_info_category_request(market, code);
    const auto response = send(request.data(), request.size());
    return parse_company_info_category(response.data(), response.size());
}

std::string TdxHqApiImpl::get_company_info_content(Market market, const std::string& code,
                                                  const std::string& filename, std::uint32_t start,
                                                  std::uint32_t length) {
    const auto request = make_get_company_info_content_request(market, code, filename, start, length);
    const auto response = send(request.data(), request.size());
    return parse_company_info_content(response.data(), response.size());
}

vector<XdxrRecord> TdxHqApiImpl::get_xdxr_info(Market market, const std::string& code) {
    const auto request = make_get_xdxr_info_request(market, code);
    const auto response = send(request.data(), request.size());
    return parse_xdxr_info(response.data(), response.size());
}

size_t TdxHqApiImpl::get_security_count(Market market) {
    auto cmd = get_stock_count_cmd((unsigned short)market);
    auto receive_data = send(cmd);
    unsigned short stock_count = 0;
    if(receive_data.size() > 1)
        stock_count = *(unsigned short*)receive_data.data();
    return stock_count;
}

vector<SnapShot> TdxHqApiImpl::get_security_snapshots(const vector<pair<string, Market>>& stock_list){
    if (round2_socket_ && !stock_list.empty() && stock_list.size() <= (65535 - 12) / 7) {
        std::vector<char> body{5, 0, 0, 0, 0, 0, 0, 0};
        append_u16(body, static_cast<std::uint16_t>(stock_list.size()));
        for (const auto& [code, market] : stock_list) {
            body.push_back(static_cast<char>(market));
            std::array<char, 6> code_bytes{};
            std::copy_n(code.data(), std::min<size_t>(code.size(), code_bytes.size()), code_bytes.data());
            body.insert(body.end(), code_bytes.begin(), code_bytes.end());
        }
        const auto response = send_round2(0x054c, body);
        auto snapshots = parse_round2_snapshots(response, stock_list);
        if (!snapshots.empty()) return snapshots;
    }
    vector<SnapShot> ret;
    auto cmd = make_get_snap_request(stock_list);
    auto receive_data = send(cmd.data(), cmd.size());
    if (receive_data.size() < 4) return ret;
    return parse_snapshot(receive_data.data(), receive_data.size());
}
vector<SecurityInfo> TdxHqApiImpl::get_security_list(Market market, unsigned short start){
    vector<SecurityInfo> ret;
    auto cmd = make_get_security_list_request((unsigned short)market, start);
    auto receive_data = send(cmd.data(), cmd.size());
    return parse_security_list(receive_data.data(),receive_data.size());
}

FinanceInfo TdxHqApiImpl::get_finance_info(Market market, const string& code){
    auto cmd = make_get_finance_info_request((unsigned short)market, code);
    auto receive_data = send(cmd.data(), cmd.size());
    return parse_finance_info(receive_data.data(),receive_data.size());
}

BlockInfoMeta TdxHqApiImpl::get_block_info_meta(const std::string& blockfile) {
    const auto request = make_get_block_info_meta_request(blockfile);
    const auto response = send(request.data(), request.size());
    return parse_block_info_meta(response.data(), response.size());
}

std::string TdxHqApiImpl::get_block_info(const std::string& blockfile, std::uint32_t start,
                                         std::uint32_t size) {
    const auto request = make_get_block_info_request(blockfile, start, size);
    const auto response = send(request.data(), request.size());
    return parse_block_info_content(response.data(), response.size());
}

std::vector<BlockConstituent> TdxHqApiImpl::get_and_parse_block_info(const std::string& blockfile) {
    constexpr std::uint32_t chunk_size = 0x7530;
    const auto meta = get_block_info_meta(blockfile);
    if (meta.size == 0) return {};
    std::string content;
    content.reserve(meta.size);
    for (std::uint32_t offset = 0; offset < meta.size;) {
        const auto wanted = std::min(chunk_size, meta.size - offset);
        auto chunk = get_block_info(blockfile, offset, wanted);
        if (chunk.empty()) return {};
        if (chunk.size() > wanted) chunk.resize(wanted);
        content.append(chunk);
        offset += static_cast<std::uint32_t>(chunk.size());
    }
    if (content.size() != meta.size) return {};
    return parse_block_info_file(content);
}

std::vector<char> TdxHqApiImpl::send(const char* send_data, size_t send_len) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_send_time_ = std::chrono::high_resolution_clock::now();
    std::vector<char> recived_data;
    const bool sent = timed_transfer(io_service_, *socket_, [&](auto handler) {
        boost::asio::async_write(*socket_, boost::asio::buffer(send_data, send_len), handler);
    });
    if (!sent) {
        std::cerr << "boost::asio::write timed out or failed" << std::endl;
        boost::system::error_code close_error;
        socket_->close(close_error);
        return {};
    }

    resp_header resp_header;
    const bool header_received = timed_transfer(io_service_, *socket_, [&](auto handler) {
        boost::asio::async_read(*socket_, boost::asio::buffer((char*)&resp_header, kHeaderLen), handler);
    });
    if (!header_received) {
        std::cerr << "boost::asio::read header timed out or failed" << std::endl;
        boost::system::error_code close_error;
        socket_->close(close_error);
        return {};
    }
    size_t offset = 0;
    std::vector<char> tmp_data;
    if (resp_header.zip_size == 0 || resp_header.unzip_size == 0) return {};
    tmp_data.resize(resp_header.zip_size);
    const auto remaining = resp_header.zip_size - offset;
    const bool body_received = timed_transfer(io_service_, *socket_, [&](auto handler) {
        boost::asio::async_read(*socket_, boost::asio::buffer(tmp_data.data() + offset, remaining), handler);
    });
    if (!body_received) {
        std::cerr << "boost::asio::read body timed out or failed" << std::endl;
        boost::system::error_code close_error;
        socket_->close(close_error);
        return {};
    }
    if (resp_header.zip_size != resp_header.unzip_size) {
        unzip_data(tmp_data.data(), resp_header.zip_size, resp_header.unzip_size, recived_data);
    } else {
        tmp_data.swap(recived_data);
    }
    return recived_data;
}

bool TdxHqApiImpl::connect_round2() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connect_round2_unlocked();
}

bool TdxHqApiImpl::connect_round2_unlocked() {
    if (round2_host_.empty() || round2_port_.empty()) return false;
    if (round2_socket_) {
        boost::system::error_code ignored;
        round2_socket_->close(ignored);
    }
    tcp::resolver resolver(io_service_);
    boost::system::error_code ec;
    const auto endpoints = resolver.resolve(round2_host_, round2_port_, ec);
    if (ec) return false;
    round2_socket_ = std::make_shared<tcp::socket>(io_service_);
    boost::asio::connect(*round2_socket_, endpoints, ec);
    if (ec) {
        round2_socket_.reset();
        return false;
    }
    round2_msg_id_ = 0;
    const auto handshake = send_round2_unlocked(0x000d, std::vector<char>{1});
    if (handshake.size() >= 189) return true;
    if (round2_socket_) {
        boost::system::error_code ignored;
        round2_socket_->close(ignored);
        round2_socket_.reset();
    }
    return false;
}

std::vector<char> TdxHqApiImpl::send_round2(std::uint16_t command, const std::vector<char>& body) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (body.size() > std::numeric_limits<std::uint16_t>::max() - 2) return {};
    if ((!round2_socket_ || !round2_socket_->is_open()) && !connect_round2_unlocked()) return {};
    auto response = send_round2_unlocked(command, body);
    if (response.empty() && command != 0x000d && connect_round2_unlocked())
        response = send_round2_unlocked(command, body);
    return response;
}

std::vector<char> TdxHqApiImpl::send_round2_unlocked(std::uint16_t command,
                                                     const std::vector<char>& body) {
    if (!round2_socket_ || !round2_socket_->is_open()) return {};
    last_send_time_ = std::chrono::high_resolution_clock::now();
    const auto msg_id = ++round2_msg_id_;
    auto fail = [this]() {
        if (round2_socket_) {
            boost::system::error_code ignored;
            round2_socket_->close(ignored);
            round2_socket_.reset();
        }
        return std::vector<char>{};
    };
    std::vector<char> frame;
    frame.reserve(12 + body.size());
    frame.push_back(0x0c);
    append_u32(frame, msg_id);
    frame.push_back(0x01);
    const auto length = static_cast<std::uint16_t>(body.size() + 2);
    append_u16(frame, length);
    append_u16(frame, length);
    append_u16(frame, command);
    frame.insert(frame.end(), body.begin(), body.end());

    const bool sent = timed_transfer(io_service_, *round2_socket_, [&](auto handler) {
        boost::asio::async_write(*round2_socket_, boost::asio::buffer(frame), handler);
    });
    if (!sent) return fail();
    std::array<char, 16> header{};
    const bool received_header = timed_transfer(io_service_, *round2_socket_, [&](auto handler) {
        boost::asio::async_read(*round2_socket_, boost::asio::buffer(header), handler);
    });
    if (!received_header || std::memcmp(header.data(), "\xb1\xcb\x74\x00", 4) != 0 ||
        read_u32(header.data() + 5) != msg_id || read_u16(header.data() + 10) != command) return fail();
    const auto zip_size = read_u16(header.data() + 12);
    const auto unzip_size = read_u16(header.data() + 14);
    if (zip_size == 0 || unzip_size == 0) return fail();
    std::vector<char> raw(zip_size);
    const bool received_body = timed_transfer(io_service_, *round2_socket_, [&](auto handler) {
        boost::asio::async_read(*round2_socket_, boost::asio::buffer(raw), handler);
    });
    if (!received_body) return fail();
    if (zip_size == unzip_size) return raw;
    std::vector<char> decoded;
    if (!unzip_data(raw.data(), zip_size, unzip_size, decoded)) return fail();
    return decoded;
}

shared_ptr<TdxHqApi> create_tdx_hq_api() { return std::make_shared<TdxHqApiImpl>(); }
}  // namespace cpptdx
