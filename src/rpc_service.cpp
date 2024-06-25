#include "rpc_service.h"
// #include "lolog/Lolog-Inl.h"
#include <brpc/restful.h>
#include <brpc/server.h>
#include <json2pb/pb_to_json.h>

#include "config.h"


namespace cpptdx {

static void CallAfterRpc(brpc::Controller* cntl, const google::protobuf::Message* req, const google::protobuf::Message* res) {
    // at this time res is already sent to client, but cntl/req/res is not destructed
    std::string req_str;
    std::string res_str;
    json2pb::ProtoMessageToJson(*req, &req_str, NULL);
    json2pb::ProtoMessageToJson(*res, &res_str, NULL);
    // LOG(INFO) << "req:" << req_str
    //             << " res:" << res_str;
}
HttpServiceImpl::HttpServiceImpl() { 
    tdx_hq_api_ = create_tdx_hq_api(); 
    connect_to_server();
}
HttpServiceImpl::~HttpServiceImpl() {}

void HttpServiceImpl::connect_to_server() {
    auto config = Config::get_instance();
    for (auto& host : config->tdx_hosts) {
        if (tdx_hq_api_->connect(host.first, host.second)) {
            // LOINFO() << " connect success!" << "ip:" << host.first << " port:" << host.second;
            break;
        }else{
        //    LOINFO() << " connect failed!" << "ip:" << host.first << " port:" << host.second; 
        }
    }
}

void HttpServiceImpl::GetSecuritySnapshots(google::protobuf::RpcController* cntl_base, const rpc_server::HttpRequest* req, 
                                            rpc_server::HttpResponse* res, google::protobuf::Closure* done) {
    // This object helps you to call done->Run() in RAII style. If you need
    // to process the request asynchronously, pass done_guard.release().
    brpc::ClosureGuard done_guard(done);

    brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);
    vector<pair<string, Market>> stock_list;
    
    auto array = nlohmann::json::parse(cntl->request_attachment().to_string());
    if(array.is_array()){
        for(const auto& item : array.items()){
            string code_str = item.value()["code"];
            string market_str = item.value()["market"];
            Market market = (Market)atoi(market_str.c_str());
            stock_list.emplace_back(std::make_pair(code_str,market));
        }
    }
    auto snapshots = tdx_hq_api_->get_security_snapshots(stock_list);

    cntl->http_response().set_content_type("text/plain");
    nlohmann::json ret_array = nlohmann::json::array();
    try{
        for(const auto& snap : snapshots){
            nlohmann::json j;
            to_json(j, snap);
            ret_array.emplace_back(j);
        }
    } catch (nlohmann::json::type_error& e) {

    };

    butil::IOBufBuilder os;
    os << ret_array.dump();
    os.move_to(cntl->response_attachment());
}

void HttpServiceImpl::GetSecurityList(google::protobuf::RpcController* cntl_base, const rpc_server::HttpRequest* req, 
                                        rpc_server::HttpResponse* res, google::protobuf::Closure* done) {

    brpc::ClosureGuard done_guard(done);

    brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);
    Market market = Market::kSH;
    int start = 0;
    for (brpc::URI::QueryIterator it = cntl->http_request().uri().QueryBegin(); it != cntl->http_request().uri().QueryEnd(); ++it) {
        if(it->first == "market"){
            market = (Market)atoi(it->second.c_str());
        }else if(it->first == "start"){
            start = atoi(it->second.c_str());
        }
    }
    auto security_infos = tdx_hq_api_->get_security_list(market,start);
    // Fill response.
    cntl->http_response().set_content_type("text/plain");
    
    nlohmann::json array = nlohmann::json::array();
    try{

        for(const auto& info : security_infos){
            nlohmann::json j;
            to_json(j, info);
            array.emplace_back(j);
        }
    } catch (nlohmann::json::type_error& e) {

    };
    butil::IOBufBuilder os;
    os << array.dump();
    os.move_to(cntl->response_attachment());
}

void HttpServiceImpl::GetSecurityKlines(google::protobuf::RpcController* cntl_base, const rpc_server::HttpRequest* req, 
                                        rpc_server::HttpResponse* res, google::protobuf::Closure* done) {

    brpc::ClosureGuard done_guard(done);

    brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);
    int cat = 0;
    int market = 0;
    std::string code;
    int start = 0;
    int count = 0;
    for (brpc::URI::QueryIterator it = cntl->http_request().uri().QueryBegin(); it != cntl->http_request().uri().QueryEnd(); ++it) {

        if(it->first == "cat"){
            cat = atoi(it->second.c_str());
        }else if(it->first == "market"){
            market = atoi(it->second.c_str());
        }else if(it->first == "code"){
            code = it->second;
        }else if(it->first == "start"){
            start = atoi(it->second.c_str());
        }else if(it->first == "count"){
            count = atoi(it->second.c_str());
        }
    }
    auto res_klines = tdx_hq_api_->get_security_klines((Category)cat,(Market)market, code, start, count);
    // Fill response.
    cntl->http_response().set_content_type("text/plain");

    nlohmann::json array = nlohmann::json::array();
    try{

        for(const auto& k : res_klines){
            nlohmann::json j;
            to_json(j, k);
            array.emplace_back(j);
        }
    } catch (nlohmann::json::type_error& e) {

    };
    butil::IOBufBuilder os;
    os << array.dump();
    os.move_to(cntl->response_attachment());
}

int run_server(){
    brpc::Server server;
    HttpServiceImpl http_svc;
    if (server.AddService(&http_svc,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add http_svc";
        return -1;
    }
    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = 3;

    if (server.Start(5051, &options) != 0) {
        LOG(ERROR) << "Fail to start HttpServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
}  // namespace cpptdx