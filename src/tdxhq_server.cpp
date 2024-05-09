#include <iostream>
#include "lolog/Lolog-Inl.h"
#include "config.h"
#include "app.h"
#include "include/tdxhq_api.h"

using namespace cpptdx;

int main(int argc, char* argv[]) {
    Application app(argv[0]);
    do {
        // Log init
        string log_path = app.current_path() + "/log/" + app.get_app_name();
        lolog::init_logging(log_path.c_str(), 5, 200 << 20);
        if (argc > 1){
            lolog::set_log_level(argv[1]);
        }else{
            lolog::set_log_level("Debug");
        }

        // load config
        auto config = Config::get_instance();
        string config_path = app.current_path() + "/config/" + "application.yml";
        if (!config->load_config(config_path)){
            break;
        }
        auto tdx_api = create_tdx_hq_api();
        if(tdx_api->connect(config->tdx_host,config->tdx_port)){
            //TODO
            auto f = tdx_api->get_finance_info(Market::kSZ,"000001");
            int a = 0;
        }

    } while(false);

    return 0;
}