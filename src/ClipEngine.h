#pragma once

/**
 * ClipEngine 指责
 * 1、对外提供接口，让用户可以方便地传入视频帧或图片等数据
 * 2、支持一系列处理操作：滤镜、特效、曝光、增益调节等
 * 3、多轨道、多图层合成
 * 4、输出到窗口或文件
 */
#include "core/ClipContext.h"

// 前置声明核心模块
class ClipContext;  // GPU 设备管理类：管理 GPU 上下文、命令队列、资源分配、纹理/缓冲区操作等
class Track;        // 媒体轨道类：表示一条媒体流，可以是视频轨、音频轨或图片序列
class EffectNode;   // 特效/滤镜节点：对输入帧进行处理，例如色彩、模糊、曝光、变换等
class EffectChain;  // 特效链/处理管线：由多个 EffectNode 串联而成，形成可复用的处理流程
class RenderTarget; // 输出目标类：负责将最终渲染帧输出到窗口、屏幕或文件

class Track;

struct GLFWwindow;

class ClipEngine {

public:
    void initialize(GLFWwindow* window);

    void shutdown();

    void render();

private:
    ClipContext context_;

    wgpu::Buffer vertex_buffer;
};