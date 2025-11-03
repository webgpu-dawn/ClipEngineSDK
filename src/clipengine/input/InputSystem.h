#pragma once

#include "InputEvent.h"
#include <vector>
#include <map>
#include <memory>
#include <mutex>

namespace clipengine {

/**
 * @brief Unified input system for handling mouse, touch, and keyboard events
 *
 * Inspired by Orillusion's InputSystem design, this provides a centralized
 * event-driven input handling system that is COMPLETELY WINDOW-SYSTEM AGNOSTIC.
 *
 * Architecture:
 * - Core InputSystem has NO dependencies on GLFW, Win32, SDL, or any window library
 * - Uses event injection pattern (inject*() methods) to receive input from adapters
 * - Window-specific code is isolated in separate adapter classes
 *
 * Features:
 * - Unified event model for mouse and touch
 * - Event listener registration system (thread-safe)
 * - Modifier key tracking
 * - Multi-touch support (via pointerId)
 * - Works with ANY window system through adapters or direct injection API
 *
 * Example usage with GLFW adapter:
 * @code
 * #include <clipengine/input/InputSystem.h>
 * #include <clipengine/input/InputSystemGLFWAdapter.h>
 * #include <clipengine/input/KeyCode.h>
 *
 * InputSystem inputSystem;
 * inputSystem.initialize(1920, 1080);
 *
 * // Use adapter for automatic GLFW integration
 * InputSystemGLFWAdapter adapter(&inputSystem);
 * adapter.attach(glfwWindow);
 *
 * // Register event listener
 * inputSystem.addEventListener(InputEventType::PointerClick, [](const InputEvent& event) {
 *     std::cout << "Click at: " << event.mouseX << ", " << event.mouseY << std::endl;
 * });
 *
 * // In main loop
 * inputSystem.update(deltaTime);
 * @endcode
 *
 * Example usage with manual event injection (no adapter):
 * @code
 * #include <clipengine/input/InputSystem.h>
 * #include <clipengine/input/KeyCode.h>
 *
 * InputSystem inputSystem;
 * inputSystem.initialize(1920, 1080);
 *
 * // In your window system's callbacks, inject events directly:
 * void onMouseMove(float x, float y) {
 *     inputSystem.injectMouseMove(x, y);
 * }
 *
 * void onMouseButton(int button, bool pressed) {
 *     ModifierKeys mods = getCurrentModifierKeys();
 *     inputSystem.injectMouseButton(button, pressed, mods);
 * }
 * @endcode
 *
 * Available adapters:
 * - InputSystemGLFWAdapter - for GLFW windows
 * - InputSystemWin32Adapter - for Win32 windows
 * - Create your own adapter for other window systems (SDL, Qt, etc.)
 */
class InputSystem {
public:
    InputSystem();
    ~InputSystem();

    /**
     * @brief Initialize the input system
     * @param windowWidth Window width in pixels
     * @param windowHeight Window height in pixels
     * @return true if initialization succeeded
     */
    bool initialize(int windowWidth, int windowHeight);

    /**
     * @brief Shutdown the input system
     */
    void shutdown();

    /**
     * @brief Update the input system (call once per frame)
     */
    void update(double deltaTime);

    /**
     * @brief Add an event listener for a specific event type
     * @param type Event type to listen for
     * @param callback Callback function to invoke when event occurs
     * @return Listener ID (can be used to remove listener)
     */
    int addEventListener(InputEventType type, InputEventCallback callback);

    /**
     * @brief Remove an event listener
     * @param listenerId Listener ID returned from addEventListener
     * @return true if listener was found and removed
     */
    bool removeEventListener(int listenerId);

    /**
     * @brief Remove all event listeners for a specific type
     * @param type Event type
     */
    void removeAllListeners(InputEventType type);

    /**
     * @brief Remove all event listeners
     */
    void removeAllListeners();

    /**
     * @brief Get current mouse position in screen coordinates
     */
    void getMousePosition(float& x, float& y) const;

    /**
     * @brief Get current mouse position in normalized coordinates [0, 1]
     */
    void getMousePositionNormalized(float& x, float& y) const;

    /**
     * @brief Get current modifier key state
     */
    const ModifierKeys& getModifierKeys() const { return currentModifiers_; }

    /**
     * @brief Enable/disable specific event type
     */
    void setEventEnabled(InputEventType type, bool enabled);

    /**
     * @brief Check if event type is enabled
     */
    bool isEventEnabled(InputEventType type) const;

    /**
     * @brief Set window dimensions (call when window is resized)
     */
    void setWindowSize(int width, int height);

    // ========================================================================
    // Event Injection API (call from window system callbacks)
    // ========================================================================

    /**
     * @brief Inject mouse movement event
     * @param x Mouse X position in pixels
     * @param y Mouse Y position in pixels
     */
    void injectMouseMove(float x, float y);

    /**
     * @brief Inject mouse button event
     * @param button Mouse button (Left=0, Right=1, Middle=2, Button4=3, Button5=4)
     * @param pressed True if pressed, false if released
     * @param modifiers Modifier key state (shift, ctrl, alt, meta)
     */
    void injectMouseButton(int button, bool pressed, const ModifierKeys& modifiers);

    /**
     * @brief Inject scroll event
     * @param deltaX Horizontal scroll delta
     * @param deltaY Vertical scroll delta
     */
    void injectScroll(float deltaX, float deltaY);

    /**
     * @brief Inject keyboard event
     * @param keyCode Platform key code
     * @param scanCode Platform scan code
     * @param pressed True if pressed, false if released
     * @param modifiers Modifier key state
     */
    void injectKey(int keyCode, int scanCode, bool pressed, const ModifierKeys& modifiers);

private:
    // Event listener structure
    struct EventListener {
        int id;
        InputEventType type;
        InputEventCallback callback;
    };

    // Helper methods
    void dispatchEvent(const InputEvent& event);

    // State
    bool initialized_ = false;

    // Event listeners
    std::vector<EventListener> listeners_;
    int nextListenerId_ = 1;
    std::mutex listenersMutex_;

    // Current input state
    float currentMouseX_ = 0.0f;
    float currentMouseY_ = 0.0f;
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
    ModifierKeys currentModifiers_;

    // Window dimensions (for normalized coordinates)
    int windowWidth_ = 1920;
    int windowHeight_ = 1080;

    // Event enable/disable flags
    std::map<InputEventType, bool> eventEnabled_;

    // Timing
    double lastUpdateTime_ = 0.0;
};

} // namespace clipengine
