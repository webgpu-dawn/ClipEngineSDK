#pragma

#include <clipengine/clipengine.h>

#include <GLFW/glfw3.h>
#include <string>

class Application
{
public:
    void initialize();

    void run();

private:
    GLFWwindow* window_;
    uint32_t    width_;
    uint32_t    height_;
    std::string title_;

    ClipEngine  ce_;
    std::unique_ptr<VideoRenderer> video_renderer_;

};