#pragma once

#include <string>
#include <functional>

namespace clipengine {

/**
 * @brief Input event types
 */
enum class InputEventType {
    PointerClick,    // Mouse click or touch tap
    PointerDown,     // Mouse button down or touch start
    PointerUp,       // Mouse button up or touch end
    PointerMove,     // Mouse move or touch move
    PointerOut,      // Mouse leaves area or touch cancelled
    PointerEnter,    // Mouse enters area
    KeyDown,         // Keyboard key pressed
    KeyUp,           // Keyboard key released
    Scroll           // Mouse wheel or trackpad scroll
};

/**
 * @brief Mouse button identifiers
 */
enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4
};

/**
 * @brief Input device type
 */
enum class InputDeviceType {
    Mouse,
    Touch,
    Pen,
    Unknown
};

/**
 * @brief Modifier key flags
 */
struct ModifierKeys {
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool meta = false;  // Windows key / Command key
};

/**
 * @brief Input event data
 *
 * Unified event structure for mouse, touch, and keyboard input
 * Similar to Orillusion's PointerEvent3D
 */
class InputEvent {
public:
    InputEvent() = default;

    // Event type and device
    InputEventType type = InputEventType::PointerMove;
    InputDeviceType deviceType = InputDeviceType::Mouse;

    // Pointer identification
    int pointerId = 0;          // Unique pointer ID (for multi-touch)

    // Screen coordinates (pixels)
    float mouseX = 0.0f;        // Current X position
    float mouseY = 0.0f;        // Current Y position

    // Normalized coordinates [0, 1]
    float normalizedX = 0.0f;   // X in normalized coordinates
    float normalizedY = 0.0f;   // Y in normalized coordinates

    // Movement deltas
    float movementX = 0.0f;     // Movement since last event
    float movementY = 0.0f;     // Movement since last event

    // Scroll deltas
    float deltaX = 0.0f;        // Horizontal scroll amount
    float deltaY = 0.0f;        // Vertical scroll amount

    // Mouse button state
    MouseButton button = MouseButton::Left;
    bool buttonPressed = false;

    // Keyboard state
    int keyCode = 0;            // Platform key code (GLFW, Win32 VK_*, SDL, etc.)
    int scanCode = 0;           // Platform-specific scan code

    // Modifier keys
    ModifierKeys modifiers;

    // Touch-specific
    float pressure = 1.0f;      // Touch pressure (0.0 - 1.0)

    // Timing
    double timestamp = 0.0;     // Event timestamp

    // Target information (can be extended)
    void* userData = nullptr;   // Custom user data

    /**
     * @brief Check if a specific modifier key is pressed
     */
    bool hasModifier(bool ModifierKeys::*key) const {
        return modifiers.*key;
    }

    /**
     * @brief Check if Shift is pressed
     */
    bool isShiftPressed() const { return modifiers.shift; }

    /**
     * @brief Check if Ctrl is pressed
     */
    bool isCtrlPressed() const { return modifiers.ctrl; }

    /**
     * @brief Check if Alt is pressed
     */
    bool isAltPressed() const { return modifiers.alt; }

    /**
     * @brief Check if Meta (Win/Cmd) is pressed
     */
    bool isMetaPressed() const { return modifiers.meta; }
};

/**
 * @brief Event listener callback type
 */
using InputEventCallback = std::function<void(const InputEvent&)>;

} // namespace clipengine
