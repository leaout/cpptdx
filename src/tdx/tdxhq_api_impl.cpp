#include "tdxhq_api_impl.h"
#include "msg/get_security_klines.h"
#include "msg/get_security_snapshot.h"
#include "msg/get_security_list.h"
#include "msg/get_finance_info.h"
#include "tdx_base.h"
#include "utils.h"

namespace cpptdx {

TdxHqApiImpl::~TdxHqApiImpl() {
    stop();
}

bool TdxHqApiImpl::connect(const string& host, const string& port) {
    tcp::resolver resolver(io_service_);
    tcp::resolver::query query(tcp::v4(), host, port);
    tcp::resolver::iterator iterator = resolver.resolve(query);
    boost::system::error_code ec;
    socket_.reset(new boost::asio::ip::tcp::socket(io_service_));
    socket_->connect(*iterator, ec);
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

size_t TdxHqApiImpl::get_security_count(Market market) {
    auto cmd = get_stock_count_cmd((unsigned short)market);
    auto receive_data = send(cmd);
    unsigned short stock_count = *(unsigned short*)receive_data.data();
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

std::vector<char> TdxHqApiImpl::send(const char* send_data, size_t send_len) {
    std::lock_guard<std::mutex> lock(mutex_);
    boost::system::error_code ec;
    std::vector<char> recived_data;
    auto tmp_send_len = boost::asio::write(*socket_, boost::asio::buffer(send_data, send_len),ec);
    if (tmp_send_len != send_len) {
        return {};
    }
    if (ec) {
        std::cerr << "boost::asio::write failed: " << ec.message() << std::endl;
        return {};
    }

    resp_header resp_header;
    size_t n = boost::asio::read(*socket_, boost::asio::buffer((char*)&resp_header, kHeaderLen), ec);
    if (ec) {
        std::cerr << "boost::asio::read failed: " << ec.message() << std::endl;
        return {};
    }
    if (n != kHeaderLen) {
        return {};
    }
    size_t offset = 0;
    std::vector<char> tmp_data;
    while (true) {
        tmp_data.resize(resp_header.zip_size);

        size_t read_n = boost::asio::read(*socket_, boost::asio::buffer(tmp_data.data() + offset, resp_header.zip_size - offset), ec);
        offset += read_n;
        if (offset == resp_header.zip_size) {
            break;
        }
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