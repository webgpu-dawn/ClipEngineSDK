#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::json;

// 获取 AppData 下配置目录
inline std::string get_app_data_path() {
#ifdef _WIN32
    char* appdata = nullptr;
    size_t len = 0;
    _dupenv_s(&appdata, &len, "APPDATA");
    if (appdata) {
        std::string path(appdata);
        free(appdata);
        return path + "\\com.clipforge.app\\ClipForge";
    }
    return "";
#elif __APPLE__
    const char* home = getenv("HOME");
    if (home) return std::string(home) + "/Library/Application Support/com.clipforge.app/ClipForge";
#else
    const char* home = getenv("HOME");
    if (home) return std::string(home) + "/.local/share/com.clipforge.app/ClipForge";
#endif
    return "";
}

// 子结构体
struct WindowPos {
    int x = 0;
    int y = 0;
};

struct WindowSize {
    int w = 800;
    int h = 600;
};

// 主配置结构体
struct ConfigData {
    WindowPos render_wnd_pos;
    WindowSize render_wnd_size;
    std::string theme = "light";

    // 从 json 填充结构体
    void from_json(const json& j) {
        if (j.contains("render_wnd_pos")) {
            render_wnd_pos.x = j["render_wnd_pos"].value("x", 0);
            render_wnd_pos.y = j["render_wnd_pos"].value("y", 0);
        }
        if (j.contains("render_wnd_size")) {
            render_wnd_size.w = j["render_wnd_size"].value("w", 800);
            render_wnd_size.h = j["render_wnd_size"].value("h", 600);
        }
        theme = j.value("theme", "light");
    }
};

class Config {
public:
    Config() {
        config_dir = get_app_data_path();
        file_path = config_dir + "/clipforge.json";
    }

    // 读取 JSON 文件并填充 ConfigData
    bool load() {
        if (!fs::exists(file_path)) {
            std::cerr << "配置文件不存在: " << file_path << std::endl;
            return false;
        }

        std::ifstream ifs(file_path);
        if (!ifs.is_open()) {
            std::cerr << "无法打开配置文件: " << file_path << std::endl;
            return false;
        }

        try {
            json j;
            ifs >> j;
            data.from_json(j);
        } catch (const std::exception& e) {
            std::cerr << "解析 JSON 失败: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

    const ConfigData& get() const { return data; }

private:
    std::string config_dir;
    std::string file_path;
    ConfigData data;
};
