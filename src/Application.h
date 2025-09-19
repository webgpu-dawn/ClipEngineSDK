#pragma once

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "core/ClipEngine.h"
#include "EventChannel.h"

class Application {

public:
    void initialize();

    void run();

private:
    void init_config();

private:
    int width_;
    int height_;
    int x_;
    int y_;
    
    ClipEngine engine_;
    GLFWwindow* window_;

    EventChannel channel_;

};