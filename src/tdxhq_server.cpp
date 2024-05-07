#include <iostream>
#include "lolog/Lolog-Inl.h"
#include "config.h"
#include "app.h"
using namespace cpptdx;

int main(int argc, char* argv[]) {
    Application app(argv[0]);
    do {
        const char* host = "119.147.212.81";
        const char* port = "7709";
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

    } while(false);

    return 0;
}