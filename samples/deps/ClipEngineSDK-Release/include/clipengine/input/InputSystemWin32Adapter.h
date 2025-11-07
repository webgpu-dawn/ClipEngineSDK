#pragma once

#include "InputSystem.h"
#include "KeyCode.h"
#include <windows.h>

namespace clipengine {

/**
 * @brief Helper class to integrate InputSystem with Win32 windows
 *
 * This adapter provides Win32 message handlers to inject events into InputSystem.
 *
 * Example usage:
 * @code
 * InputSystem inputSystem;
 * inputSystem.initialize(1920, 1080);
 *
 * InputSystemWin32Adapter adapter(&inputSystem);
 *
 * // In your Win32 window procedure (WndProc):
 * LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
 *     // Forward input messages to InputSystem
 *     if (adapter.handleMessage(msg, wParam, lParam)) {
 *         return 0; // Message was handled
 *     }
 *
 *     // Handle other messages...
 *     return DefWindowProc(hwnd, msg, wParam, lParam);
 * }
 * @endcode
 */
class InputSystemWin32Adapter {
public:
    /**
     * @brief Constructor
     * @param inputSystem Pointer to the InputSystem to inject events into
     */
    explicit InputSystemWin32Adapter(InputSystem* inputSystem)
        : inputSystem_(inputSystem) {}

    /**
     * @brief Handle a Win32 window message
     * @param msg Message identifier
     * @param wParam First message parameter
     * @param lParam Second message parameter
     * @return true if the message was handled, false otherwise
     */
    bool handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        if (!inputSystem_) return false;

        switch (msg) {
            case WM_MOUSEMOVE:
                handleMouseMove(lParam);
                return true;

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
                handleMouseButton(msg, wParam, lParam, true);
                return true;

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
                handleMouseButton(msg, wParam, lParam, false);
                return true;

            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                handleMouseWheel(msg, wParam);
                return true;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                handleKeyEvent(wParam, lParam, true);
                return true;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                handleKeyEvent(wParam, lParam, false);
                return true;

            case WM_SIZE:
                handleResize(lParam);
                return true;

            default:
                return false;
        }
    }

private:
    void handleMouseMove(LPARAM lParam) {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        inputSystem_->injectMouseMove(static_cast<float>(x), static_cast<float>(y));
    }

    void handleMouseButton(UINT msg, WPARAM wParam, LPARAM lParam, bool pressed) {
        int button = 0;

        switch (msg) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                button = 0; // MouseButton::Left
                break;
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                button = 1; // MouseButton::Right
                break;
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
                button = 2; // MouseButton::Middle
                break;
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
                button = (HIWORD(wParam) == XBUTTON1) ? 3 : 4;
                break;
        }

        ModifierKeys modifiers = getModifierKeys();
        inputSystem_->injectMouseButton(button, pressed, modifiers);
    }

    void handleMouseWheel(UINT msg, WPARAM wParam) {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        float normalizedDelta = static_cast<float>(delta) / WHEEL_DELTA;

        if (msg == WM_MOUSEWHEEL) {
            inputSystem_->injectScroll(0.0f, normalizedDelta);
        } else {
            inputSystem_->injectScroll(normalizedDelta, 0.0f);
        }
    }

    void handleKeyEvent(WPARAM wParam, LPARAM lParam, bool pressed) {
        int vkCode = static_cast<int>(wParam);
        int scanCode = (lParam >> 16) & 0xFF;

        // Convert Win32 VK_* codes to our common key codes
        int keyCode = convertVKToKeyCode(vkCode);

        ModifierKeys modifiers = getModifierKeys();
        inputSystem_->injectKey(keyCode, scanCode, pressed, modifiers);
    }

    void handleResize(LPARAM lParam) {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        inputSystem_->setWindowSize(width, height);
    }

    ModifierKeys getModifierKeys() {
        ModifierKeys modifiers;
        modifiers.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        modifiers.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        modifiers.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        modifiers.meta = (GetKeyState(VK_LWIN) & 0x8000) != 0 ||
                        (GetKeyState(VK_RWIN) & 0x8000) != 0;
        return modifiers;
    }

    /**
     * @brief Convert Win32 VK_* virtual key codes to our common key codes
     */
    int convertVKToKeyCode(int vkCode) {
        // Printable characters (A-Z, 0-9, etc.)
        if (vkCode >= 'A' && vkCode <= 'Z') {
            return KeyCode::A + (vkCode - 'A');
        }
        if (vkCode >= '0' && vkCode <= '9') {
            return KeyCode::Key0 + (vkCode - '0');
        }

        // Function keys
        if (vkCode >= VK_F1 && vkCode <= VK_F24) {
            return KeyCode::F1 + (vkCode - VK_F1);
        }

        // Keypad numbers
        if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
            return KeyCode::KP0 + (vkCode - VK_NUMPAD0);
        }

        // Special keys mapping
        switch (vkCode) {
            case VK_SPACE:      return KeyCode::Space;
            case VK_OEM_COMMA:  return KeyCode::Comma;
            case VK_OEM_MINUS:  return KeyCode::Minus;
            case VK_OEM_PERIOD: return KeyCode::Period;
            case VK_OEM_2:      return KeyCode::Slash;
            case VK_OEM_1:      return KeyCode::Semicolon;
            case VK_OEM_PLUS:   return KeyCode::Equal;
            case VK_OEM_4:      return KeyCode::LeftBracket;
            case VK_OEM_5:      return KeyCode::Backslash;
            case VK_OEM_6:      return KeyCode::RightBracket;
            case VK_OEM_7:      return KeyCode::Apostrophe;
            case VK_OEM_3:      return KeyCode::GraveAccent;

            case VK_ESCAPE:     return KeyCode::Escape;
            case VK_RETURN:     return KeyCode::Enter;
            case VK_TAB:        return KeyCode::Tab;
            case VK_BACK:       return KeyCode::Backspace;
            case VK_INSERT:     return KeyCode::Insert;
            case VK_DELETE:     return KeyCode::Delete;
            case VK_RIGHT:      return KeyCode::Right;
            case VK_LEFT:       return KeyCode::Left;
            case VK_DOWN:       return KeyCode::Down;
            case VK_UP:         return KeyCode::Up;
            case VK_PRIOR:      return KeyCode::PageUp;
            case VK_NEXT:       return KeyCode::PageDown;
            case VK_HOME:       return KeyCode::Home;
            case VK_END:        return KeyCode::End;
            case VK_CAPITAL:    return KeyCode::CapsLock;
            case VK_SCROLL:     return KeyCode::ScrollLock;
            case VK_NUMLOCK:    return KeyCode::NumLock;
            case VK_SNAPSHOT:   return KeyCode::PrintScreen;
            case VK_PAUSE:      return KeyCode::Pause;

            case VK_DECIMAL:    return KeyCode::KPDecimal;
            case VK_DIVIDE:     return KeyCode::KPDivide;
            case VK_MULTIPLY:   return KeyCode::KPMultiply;
            case VK_SUBTRACT:   return KeyCode::KPSubtract;
            case VK_ADD:        return KeyCode::KPAdd;

            case VK_LSHIFT:     return KeyCode::LeftShift;
            case VK_LCONTROL:   return KeyCode::LeftControl;
            case VK_LMENU:      return KeyCode::LeftAlt;
            case VK_LWIN:       return KeyCode::LeftSuper;
            case VK_RSHIFT:     return KeyCode::RightShift;
            case VK_RCONTROL:   return KeyCode::RightControl;
            case VK_RMENU:      return KeyCode::RightAlt;
            case VK_RWIN:       return KeyCode::RightSuper;
            case VK_APPS:       return KeyCode::Menu;

            default:
                return vkCode; // Return original code if no mapping
        }
    }

    InputSystem* inputSystem_ = nullptr;
};

} // namespace clipengine
