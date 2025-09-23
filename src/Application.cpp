#include "Application.h"
#include "Config.h"

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void Application::init_config()
{
    Config cfg;
    if (!cfg.load()) {
        std::cerr << "load failed" << std::endl;
    }

    const auto& data = cfg.get();
    std::cout << "Window Size: " << data.render_wnd_size.w << "x" << data.render_wnd_size.h << "\n";
    std::cout << "Window Pos: (" << data.render_wnd_pos.x << "," << data.render_wnd_pos.y << ")\n";
    std::cout << "Theme: " << data.theme << "\n";

    // width_  = data.render_wnd_size.w;
    // height_ = data.render_wnd_size.h;
    width_ = 800;
    height_= 800;
    x_ = data.render_wnd_pos.x;
    y_ = data.render_wnd_pos.y;
}

void Application::initialize()
{
    fs::path exe_dir = fs::current_path();
    fs::current_path(exe_dir);

    init_config();

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(width_, height_, "Application", nullptr, nullptr);
    glfwSetWindowPos(window_, x_, y_);

    engine_.initialize(window_);

    channel_.init();
    channel_.subscribe("set_size", [](nlohmann::json j){
        std::cout << "set_size" << std::endl;
    });
    channel_.subscribe("set_pos", [=](nlohmann::json j){
        x_ = j["data"]["x"];
        y_ = j["data"]["y"];
        std::cout << "@@@ set_pos =  @@@" << std::endl;
        glfwSetWindowPos(window_, x_, y_);
    });
}

void Application::run()
{
    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        engine_.render();
    }
}