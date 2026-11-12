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
#include <chrono>

namespace cpptdx {

namespace {
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
    // start heart beat thread
    is_running_ = true;
    std::thread th(&TdxHqApiImpl::heart_beat_thread, this);
    th_.swap(th);
    return true;
}

vector<Kline> TdxHqApiImpl::get_security_klines(Category cat, Market market, 
                                                const std::string& code, 
                                                unsigned short start, unsigned short count) {
    auto request = make_get_bars_request((unsigned short)cat, (unsigned short)market, code, start, count);
    std::vector<char> rev_data = send(request.data(), request.length());
    return parse_kline(rev_data.data(), rev_data.size(), (unsigned short)cat);
}

vector<Kline> TdxHqApiImpl::get_index_klines(Category cat, Market market,
                                              const std::string& code,
                                              unsigned short start, unsigned short count) {
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
    vector<SnapShot> ret;
    auto cmd = make_get_snap_request(stock_list);
    auto receive_data = send(cmd.data(), cmd.size());
    return parse_snapshot(receive_data.data(),receive_data.size());
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

shared_ptr<TdxHqApi> create_tdx_hq_api() { return std::make_shared<TdxHqApiImpl>(); }
}  // namespace cpptdx
