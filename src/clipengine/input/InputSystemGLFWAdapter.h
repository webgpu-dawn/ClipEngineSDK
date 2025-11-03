#pragma once

#include "InputSystem.h"
#include <GLFW/glfw3.h>

namespace clipengine {

/**
 * @brief Helper class to integrate InputSystem with GLFW windows
 *
 * This adapter automatically sets up GLFW callbacks to inject events into InputSystem.
 *
 * Example usage:
 * @code
 * InputSystem inputSystem;
 * inputSystem.initialize(1920, 1080);
 *
 * InputSystemGLFWAdapter adapter(&inputSystem);
 * adapter.attach(glfwWindow);
 *
 * // GLFW events will now automatically be forwarded to InputSystem
 * @endcode
 */
class InputSystemGLFWAdapter {
public:
    /**
     * @brief Constructor
     * @param inputSystem Pointer to the InputSystem to inject events into
     */
    explicit InputSystemGLFWAdapter(InputSystem* inputSystem)
        : inputSystem_(inputSystem) {}

    /**
     * @brief Attach the adapter to a GLFW window
     * @param window GLFW window to attach callbacks to
     *
     * This will register GLFW callbacks to forward events to InputSystem.
     * The window's user pointer will be used to store a reference to this adapter.
     */
    void attach(GLFWwindow* window) {
        window_ = window;

        // Store this adapter instance so callbacks can access it
        glfwSetWindowUserPointer(window_, this);

        // Register GLFW callbacks
        glfwSetCursorPosCallback(window_, glfwCursorPosCallback);
        glfwSetMouseButtonCallback(window_, glfwMouseButtonCallback);
        glfwSetScrollCallback(window_, glfwScrollCallback);
        glfwSetKeyCallback(window_, glfwKeyCallback);

        // Update InputSystem with window size
        int width, height;
        glfwGetWindowSize(window_, &width, &height);
        inputSystem_->setWindowSize(width, height);
    }

    /**
     * @brief Detach the adapter from the GLFW window
     *
     * This will clear the GLFW callbacks.
     */
    void detach() {
        if (!window_) return;

        glfwSetCursorPosCallback(window_, nullptr);
        glfwSetMouseButtonCallback(window_, nullptr);
        glfwSetScrollCallback(window_, nullptr);
        glfwSetKeyCallback(window_, nullptr);
        glfwSetWindowUserPointer(window_, nullptr);

        window_ = nullptr;
    }

private:
    // GLFW callback handlers (static)
    static void glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        auto* adapter = static_cast<InputSystemGLFWAdapter*>(glfwGetWindowUserPointer(window));
        if (adapter && adapter->inputSystem_) {
            adapter->inputSystem_->injectMouseMove(static_cast<float>(xpos), static_cast<float>(ypos));
        }
    }

    static void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        auto* adapter = static_cast<InputSystemGLFWAdapter*>(glfwGetWindowUserPointer(window));
        if (adapter && adapter->inputSystem_) {
            ModifierKeys modifiers = convertGLFWModifiers(mods);
            bool pressed = (action == GLFW_PRESS);
            int buttonIndex = convertGLFWButton(button);
            adapter->inputSystem_->injectMouseButton(buttonIndex, pressed, modifiers);
        }
    }

    static void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        auto* adapter = static_cast<InputSystemGLFWAdapter*>(glfwGetWindowUserPointer(window));
        if (adapter && adapter->inputSystem_) {
            adapter->inputSystem_->injectScroll(static_cast<float>(xoffset), static_cast<float>(yoffset));
        }
    }

    static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* adapter = static_cast<InputSystemGLFWAdapter*>(glfwGetWindowUserPointer(window));
        if (adapter && adapter->inputSystem_) {
            ModifierKeys modifiers = convertGLFWModifiers(mods);
            bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
            adapter->inputSystem_->injectKey(key, scancode, pressed, modifiers);
        }
    }

    // Helper to convert GLFW modifiers to InputSystem ModifierKeys
    static ModifierKeys convertGLFWModifiers(int mods) {
        ModifierKeys modifiers;
        modifiers.shift = (mods & GLFW_MOD_SHIFT) != 0;
        modifiers.ctrl = (mods & GLFW_MOD_CONTROL) != 0;
        modifiers.alt = (mods & GLFW_MOD_ALT) != 0;
        modifiers.meta = (mods & GLFW_MOD_SUPER) != 0;
        return modifiers;
    }

    // Helper to convert GLFW button to InputSystem button index
    static int convertGLFWButton(int button) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:   return 0; // MouseButton::Left
            case GLFW_MOUSE_BUTTON_RIGHT:  return 1; // MouseButton::Right
            case GLFW_MOUSE_BUTTON_MIDDLE: return 2; // MouseButton::Middle
            case GLFW_MOUSE_BUTTON_4:      return 3; // MouseButton::Button4
            case GLFW_MOUSE_BUTTON_5:      return 4; // MouseButton::Button5
            default:                        return 0;
        }
    }

    InputSystem* inputSystem_ = nullptr;
    GLFWwindow* window_ = nullptr;
};

} // namespace clipengine
