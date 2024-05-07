/**
 * Created by chenly on 07/05/24.
 * Description
 **/
#pragma once
#include <filesystem>
#include <string>

namespace cpptdx {
using namespace std;

class Application {
    string m_app_name;
    string m_current_path;

public:
    explicit Application(char *full_path_name) {
        filesystem::path p(full_path_name);
        m_app_name = p.stem();
        m_current_path = filesystem::current_path().string();
        // creat path
        create_dir(m_current_path + "/config/");
        create_dir(m_current_path + "/log/");
    }
    void create_dir(const string &path) {
        if (!filesystem::exists(path)) {
            filesystem::create_directories(path);
        }
    }
    const string &current_path() { return m_current_path; }
    const string &get_app_name() { return m_app_name; }
};
}  // namespace cpptdx