#include <zlib.h>
#include <boost/asio.hpp>
#include <cmath>
#include <cstring>
#include <deque>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include "utils.h"

using namespace cpptdx;
using boost::asio::ip::tcp;

class ResponseMsg {
 public:
  char *header_data() { return (char *)&resp_header_; }
  void set_header(const resp_header &header) {
    std::memcpy(&resp_header_, &header, sizeof(resp_header));
  }
  char *body_data() {
    body_len_ = resp_header_.zip_size;
    body_.resize(body_len_);
    // std::cout << "boddy_len:" << body_len_ << std::endl;
    return body_.data();
  }
  unsigned long unzip_size() { return resp_header_.unzip_size; }
  std::size_t body_length() { return body_len_; }

  void decode_body() {}

 private:
  int body_len_ = 0;
  std::vector<char> body_;
  resp_header resp_header_;
};
class RequestMsg {
 public:
  static constexpr std::size_t max_length = 64;
  void append(const char *data, int len) {
    if ((len + msg_length_) > max_length) {
      std::cerr << "RequestMsg too long";
      return;
    }
    std::memcpy(data_ + msg_length_, data, len);
    msg_length_ += len;
  }
  char *data() { return data_; }
  const char *data() const { return data_; }
  int length() { return msg_length_; }

  void set_data(const std::vector<uint8_t> &data) {
    std::memcpy(data_, data.data(), data.size());
    msg_length_ = data.size();
  }

 private:
  char data_[max_length] = {};
  unsigned int msg_length_ = 0;
  resp_header resp_header_;
};
class TdxRequestMaker {
 private:
//   category->
// K线种类
// 0 5分钟K线 1 15分钟K线 2 30分钟K线 3 1小时K线 4 日K线
// 5 周K线
// 6 月K线
// 7 1分钟
// 8 1分钟K线 9 日K线
// 10 季K线
// 11 年K线
// market -> 市场代码 0:深圳，1:上海

// stockcode -> 证券代码;

// start -> 指定的范围开始位置;

// count -> 用户要请求的 K 线数目，最大值为 800
  unsigned short category_ = 0;

 public:
  RequestMsg make_get_bars_request(unsigned short category,
                                   unsigned short market,
                                   const std::string &code,
                                   unsigned short start, unsigned short count) {
    //                                values = (
    //     0x10c,
    //     0x01016408,
    //     0x1c,
    //     0x1c,
    //     0x052d,
    //     market,
    //     code,
    //     category,
    //     1,
    //     start,
    //     count,
    //     0, 0, 0  # I + I +  H total 10 zero
    // )

    // pkg = struct.pack("<HIHHHH6sHHHHIIH", *values)
    //         字符	字节顺序	大小表示	对齐方式
    // @	本地1	本地	本地
    // =	本地	标准2	无
    // <	小端	标准	无
    // >	大端	标准	无
    // !	网络（总是大端）	标准	无
    category_ = category;
    unsigned short h1 = 0x10c;
    unsigned int h2 = 0x01016408;
    unsigned short h3 = 0x1c;
    unsigned short h4 = 0x1c;
    unsigned short h5 = 0x052d;
    unsigned int paddle = 0;
    unsigned short paddle1 = 1;
    RequestMsg req_msg;
    req_msg.append((char *)&h1, 2);
    req_msg.append((char *)&h2, 4);
    req_msg.append((char *)&h3, 2);
    req_msg.append((char *)&h4, 2);
    req_msg.append((char *)&h5, 2);
    req_msg.append((char *)&market, 2);
    req_msg.append(code.c_str(), code.size());
    req_msg.append((char *)&category, 2);
    req_msg.append((char *)&paddle1, 2);
    req_msg.append((char *)&start, 2);
    req_msg.append((char *)&count, 2);
    req_msg.append((char *)&paddle, 4);
    req_msg.append((char *)&paddle, 4);
    req_msg.append((char *)&paddle, 2);

    return req_msg;
  }

 private:
};



std::vector<uint8_t> setup_cmd1(){
    const std::string cmd1 = "0c0218930001030003000d0001";
    return hexStringToByteArray(cmd1);
}
std::vector<uint8_t> setup_cmd2(){
    const std::string cmd2 = "0c0218940001030003000d0002";
    return hexStringToByteArray(cmd2);
}

std::vector<uint8_t> setup_cmd3(){
  const std::string cmd3 =
      "0c031899000120002000db0fd5d0c9ccd6a4a8af0000008fc22540130000d500c9ccbdf0"
      "d7ea00000002";
    return hexStringToByteArray(cmd3);
}


class tdx_blocking_client {
  boost::asio::io_service io_service_;
  tcp::socket socket_;

 public:
  tdx_blocking_client(const char *host, const char *port)
      : socket_(io_service_) {
    tcp::resolver resolver(io_service_);
    tcp::resolver::query query(tcp::v4(), host, port);
    tcp::resolver::iterator iterator = resolver.resolve(query);
    socket_.connect(*iterator);
  }
  void req_security_klines() {
    TdxRequestMaker req_maker;
    auto request = req_maker.make_get_bars_request(9, 0, "000001", 0, 10);
    std::vector<char> rev_data= send(request.data(), request.length());
    auto klines = parse_kline(rev_data.data(), rev_data.size(), 9);
    for (auto kline : klines) {
      std::cout << kline.datetime << " " << kline.open << " " << kline.close
                << " " << kline.high << " " << kline.low << " " << kline.vol<< " " << kline.amount
                << std::endl;
    }
  }
 public:
 std::vector<char> send(const std::vector<uint8_t>& send_data){
  return send((char*)send_data.data(), send_data.size());
 }
  std::vector<char> send(const char *send_data, size_t send_len) {
    std::vector<char> recived_data;
    auto tmp_send_len =
        boost::asio::write(socket_, boost::asio::buffer(send_data, send_len));
    if (tmp_send_len != send_len) {
      return {};
    }

    resp_header resp_header;
    boost::system::error_code ec;
    size_t n = boost::asio::read(
        socket_, boost::asio::buffer((char *)&resp_header, kHeaderLen), ec);
    if (n != kHeaderLen) {
      return {};
    }
    size_t offset = 0;
    std::vector<char> tmp_data;
    while (true) {
      ResponseMsg response;
      tmp_data.resize(resp_header.zip_size);

      size_t read_n =
          boost::asio::read(socket_,
                            boost::asio::buffer(tmp_data.data() + offset,
                                                resp_header.zip_size - offset),
                            ec);
      offset += read_n;
      if (offset == resp_header.zip_size) {
        break;
      }
    }
    if (resp_header.zip_size != resp_header.unzip_size) {
      unzip_data(tmp_data.data(), resp_header.zip_size, resp_header.unzip_size,
                 recived_data);
    } else {
      tmp_data.swap(recived_data);
    }
    return recived_data;
  }
};
int main(int argc, char *argv[]) {
  try {
    const char *host = "119.147.212.81";
    const char *port = "7709";

    tdx_blocking_client client(host, port);
    //registry
    client.send(setup_cmd1());
    client.send(setup_cmd2());
    client.send(setup_cmd3());
    //req klines
    client.req_security_klines();

  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}