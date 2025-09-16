#pragma once

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "ClipEngine.h"

class Application {

public:
    void initialize(int w, int h);

    void run();

private:
    int width_;
    int height_;
    
    ClipEngine engine_;

    GLFWwindow* window_;

};