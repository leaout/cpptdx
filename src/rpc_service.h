#pragma once
#include "include/tdxhq_api.h"
#include "rpc_server.pb.h"

namespace cpptdx{

class HttpServiceImpl : public rpc_server::HttpService {
public:
    HttpServiceImpl();
    virtual ~HttpServiceImpl();
    void GetSecuritySnapshots(google::protobuf::RpcController* cntl_base, const rpc_server::HttpRequest* req,
                              rpc_server::HttpResponse* res, google::protobuf::Closure* done);
    void GetSecurityList(google::protobuf::RpcController* cntl_base, const rpc_server::HttpRequest* req,
                              rpc_server::HttpResponse* res, google::protobuf::Closure* done);
    void GetSecurityKlines(google::protobuf::RpcController* cntl_base, const rpc_server::HttpRequest* req,
                              rpc_server::HttpResponse* res, google::protobuf::Closure* done);

private:
    void connect_to_server();

private:
shared_ptr<TdxHqApi> tdx_hq_api_;
};

int run_server();

}