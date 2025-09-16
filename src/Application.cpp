#include "Application.h"

void Application::initialize(int w, int h)
{
    width_ = w;
    height_= h;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(width_, height_, "Application", nullptr, nullptr);

    engine_.initialize(window_);
}

void Application::run()
{
    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        engine_.render();
    }
}