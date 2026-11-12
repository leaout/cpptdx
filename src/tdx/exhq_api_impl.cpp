#include "exhq_api_impl.h"
#include "msg/exhq_basic.h"
#include "msg/get_exhq_data.h"
#include "tdx_base.h"
#include "utils.h"
#include <boost/asio.hpp>
#include <iostream>
#include <chrono>

namespace cpptdx {

namespace {
template <typename Initiate>
bool timed_transfer(boost::asio::io_context& io, boost::asio::ip::tcp::socket& socket, Initiate initiate) {
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

TdxExHqApiImpl::~TdxExHqApiImpl() {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = false;
    if (socket_) {
        boost::system::error_code ec;
        socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_->close(ec);
    }
}

bool TdxExHqApiImpl::connect(const std::string& host, const std::string& port) {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::system::error_code ec;
    const auto endpoints = resolver.resolve(host, port, ec);
    if (ec) return false;
    socket_ = std::make_unique<boost::asio::ip::tcp::socket>(io_service_);
    boost::asio::connect(*socket_, endpoints, ec);
    if (ec) {
        socket_.reset();
        return false;
    }
    const auto raw_setup = hexStringToByteArray(
        "0101486500015200520054241f32c6e5d53dfb411f32c6e5d53dfb41"
        "1f32c6e5d53dfb411f32c6e5d53dfb411f32c6e5d53dfb41"
        "1f32c6e5d53dfb411f32c6e5d53dfb411f32c6e5d53dfb41"
        "cce16dffd5ba3fb8cbc57a054f7748ea");
    std::vector<char> setup(raw_setup.begin(), raw_setup.end());
    // ExHq servers require this 92-byte session packet and reply with a
    // framed acknowledgement. Drain it before allowing API calls, otherwise
    // the acknowledgement is mistaken for the first API response.
    const auto acknowledgement = send(setup);
    connected_ = !acknowledgement.empty() && socket_ && socket_->is_open();
    if (!connected_ && socket_) {
        boost::system::error_code close_error;
        socket_->close(close_error);
    }
    return connected_;
}

bool TdxExHqApiImpl::connected() const {
    return connected_.load();
}

std::vector<char> TdxExHqApiImpl::send(const std::vector<char>& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!socket_ || !socket_->is_open()) return {};
    if (!timed_transfer(io_service_, *socket_, [&](auto handler) {
            boost::asio::async_write(*socket_, boost::asio::buffer(request), handler);
        })) {
        connected_ = false;
        boost::system::error_code close_error;
        socket_->close(close_error);
        return {};
    }
    resp_header header{};
    if (!timed_transfer(io_service_, *socket_, [&](auto handler) {
            boost::asio::async_read(*socket_, boost::asio::buffer(&header, kHeaderLen), handler);
        }) || header.zip_size == 0 || header.unzip_size == 0) {
        connected_ = false;
        boost::system::error_code close_error;
        socket_->close(close_error);
        return {};
    }
    std::vector<char> zipped(header.zip_size);
    if (!timed_transfer(io_service_, *socket_, [&](auto handler) {
            boost::asio::async_read(*socket_, boost::asio::buffer(zipped), handler);
        })) {
        connected_ = false;
        boost::system::error_code close_error;
        socket_->close(close_error);
        return {};
    }
    if (header.zip_size == header.unzip_size) return zipped;
    std::vector<char> result;
    if (!unzip_data(zipped.data(), header.zip_size, header.unzip_size, result)) return {};
    return result;
}

std::vector<ExMarket> TdxExHqApiImpl::get_markets() {
    const auto response = send(make_ex_get_markets_request());
    return parse_ex_markets(response.data(), response.size());
}

std::size_t TdxExHqApiImpl::get_instrument_count() {
    const auto response = send(make_ex_get_instrument_count_request());
    return parse_ex_instrument_count(response.data(), response.size());
}

std::vector<ExInstrument> TdxExHqApiImpl::get_instrument_info(unsigned int start, unsigned short count) {
    const auto response = send(make_ex_get_instrument_info_request(start, count));
    return parse_ex_instrument_info(response.data(), response.size());
}

std::vector<ExQuote> TdxExHqApiImpl::get_instrument_quote(unsigned char market, const std::string& code) {
    const auto response = send(exhq_detail::quote_request(market, code));
    return exhq_detail::parse_quote(response.data(), response.size());
}

std::vector<nlohmann::json> TdxExHqApiImpl::get_instrument_quote_list(unsigned char market,
    unsigned char category, unsigned short start, unsigned short count) {
    const auto response = send(exhq_detail::quote_list_request(market, start, count));
    return exhq_detail::parse_quote_list(response.data(), response.size(), category);
}

std::vector<ExBar> TdxExHqApiImpl::get_instrument_bars(unsigned char category, unsigned char market,
    const std::string& code, std::uint32_t start, unsigned short count) {
    const auto response = send(exhq_detail::bars_request(category, market, code, start, count));
    return exhq_detail::parse_bars(response.data(), response.size(), category);
}

std::vector<ExRangeBar> TdxExHqApiImpl::get_history_instrument_bars_range(unsigned char market,
    const std::string& code, std::uint32_t start_date, std::uint32_t end_date) {
    const auto response = send(exhq_detail::history_bars_range_request(market, code, start_date, end_date));
    return exhq_detail::parse_history_bars_range(response.data(), response.size());
}

std::vector<ExMinuteBar> TdxExHqApiImpl::get_minute_time_data(unsigned char market, const std::string& code) {
    const auto response = send(exhq_detail::minute_request(market, code));
    return exhq_detail::parse_minutes(response.data(), response.size(), 12);
}

std::vector<ExMinuteBar> TdxExHqApiImpl::get_history_minute_time_data(unsigned char market,
    const std::string& code, std::uint32_t date) {
    const auto response = send(exhq_detail::history_minute_request(market, code, date));
    return exhq_detail::parse_minutes(response.data(), response.size(), 20);
}

std::vector<ExTransaction> TdxExHqApiImpl::get_transaction_data(unsigned char market,
    const std::string& code, std::int32_t start, unsigned short count) {
    const auto response = send(exhq_detail::transaction_request(market, code, start, count));
    return exhq_detail::parse_transactions(response.data(), response.size(), exhq_detail::today_date(), market);
}

std::vector<ExTransaction> TdxExHqApiImpl::get_history_transaction_data(unsigned char market,
    const std::string& code, std::uint32_t date, std::int32_t start, unsigned short count) {
    const auto response = send(exhq_detail::history_transaction_request(market, code, date, start, count));
    return exhq_detail::parse_transactions(response.data(), response.size(), date, market);
}

std::shared_ptr<TdxExHqApi> create_tdx_ex_hq_api() {
    return std::make_shared<TdxExHqApiImpl>();
}

}  // namespace cpptdx
