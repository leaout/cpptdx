#include <iostream>
// #include "lolog/Lolog-Inl.h"
#include "config.h"
#include "app.h"
#include "include/tdxhq_api.h"
#include "rpc_service.h"

using namespace cpptdx;

int main(int argc, char* argv[]) {
    Application app(argv[0]);
    do {
        // Log init
        // string log_path = app.current_path() + "/log/" + app.get_app_name();
        // lolog::init_logging(log_path.c_str(), 5, 200 << 20);
        // if (argc > 1){
        //     lolog::set_log_level(argv[1]);
        // }else{
        //     lolog::set_log_level("Debug");
        // }

        // load config
        auto config = Config::get_instance();
        string config_path = app.current_path() + "/config/" + "application.yml";
        if (!config->load_config(config_path)){
            break;
        }
        run_server();

    } while(false);

    return 0;
}