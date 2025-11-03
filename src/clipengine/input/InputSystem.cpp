#include "InputSystem.h"
#include <algorithm>
#include <iostream>

namespace clipengine {

InputSystem::InputSystem() {
    // Enable all event types by default
    eventEnabled_[InputEventType::PointerClick] = true;
    eventEnabled_[InputEventType::PointerDown] = true;
    eventEnabled_[InputEventType::PointerUp] = true;
    eventEnabled_[InputEventType::PointerMove] = true;
    eventEnabled_[InputEventType::PointerOut] = true;
    eventEnabled_[InputEventType::PointerEnter] = true;
    eventEnabled_[InputEventType::KeyDown] = true;
    eventEnabled_[InputEventType::KeyUp] = true;
    eventEnabled_[InputEventType::Scroll] = true;
}

InputSystem::~InputSystem() {
    shutdown();
}

bool InputSystem::initialize(int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0) {
        return false;
    }

    windowWidth_ = windowWidth;
    windowHeight_ = windowHeight;

    initialized_ = true;
    return true;
}

void InputSystem::shutdown() {
    if (!initialized_) {
        return;
    }

    removeAllListeners();
    initialized_ = false;
}

void InputSystem::update(double deltaTime) {
    if (!initialized_) {
        return;
    }

    // Reset movement deltas for next frame
    lastMouseX_ = currentMouseX_;
    lastMouseY_ = currentMouseY_;

    lastUpdateTime_ += deltaTime;
}

void InputSystem::setWindowSize(int width, int height) {
    windowWidth_ = width;
    windowHeight_ = height;
}

int InputSystem::addEventListener(InputEventType type, InputEventCallback callback) {
    std::lock_guard<std::mutex> lock(listenersMutex_);

    EventListener listener;
    listener.id = nextListenerId_++;
    listener.type = type;
    listener.callback = callback;

    listeners_.push_back(listener);

    return listener.id;
}

bool InputSystem::removeEventListener(int listenerId) {
    std::lock_guard<std::mutex> lock(listenersMutex_);

    auto it = std::find_if(listeners_.begin(), listeners_.end(),
        [listenerId](const EventListener& listener) {
            return listener.id == listenerId;
        });

    if (it != listeners_.end()) {
        listeners_.erase(it);
        return true;
    }

    return false;
}

void InputSystem::removeAllListeners(InputEventType type) {
    std::lock_guard<std::mutex> lock(listenersMutex_);

    listeners_.erase(
        std::remove_if(listeners_.begin(), listeners_.end(),
            [type](const EventListener& listener) {
                return listener.type == type;
            }),
        listeners_.end()
    );
}

void InputSystem::removeAllListeners() {
    std::lock_guard<std::mutex> lock(listenersMutex_);
    listeners_.clear();
}

void InputSystem::getMousePosition(float& x, float& y) const {
    x = currentMouseX_;
    y = currentMouseY_;
}

void InputSystem::getMousePositionNormalized(float& x, float& y) const {
    x = currentMouseX_ / static_cast<float>(windowWidth_);
    y = currentMouseY_ / static_cast<float>(windowHeight_);
}

void InputSystem::setEventEnabled(InputEventType type, bool enabled) {
    eventEnabled_[type] = enabled;
}

bool InputSystem::isEventEnabled(InputEventType type) const {
    auto it = eventEnabled_.find(type);
    return it != eventEnabled_.end() && it->second;
}

// ============================================================================
// Event Injection API
// ============================================================================

void InputSystem::injectMouseMove(float x, float y) {
    lastMouseX_ = currentMouseX_;
    lastMouseY_ = currentMouseY_;
    currentMouseX_ = x;
    currentMouseY_ = y;

    if (!isEventEnabled(InputEventType::PointerMove)) {
        return;
    }

    InputEvent event;
    event.type = InputEventType::PointerMove;
    event.deviceType = InputDeviceType::Mouse;
    event.mouseX = currentMouseX_;
    event.mouseY = currentMouseY_;
    event.normalizedX = currentMouseX_ / static_cast<float>(windowWidth_);
    event.normalizedY = currentMouseY_ / static_cast<float>(windowHeight_);
    event.movementX = currentMouseX_ - lastMouseX_;
    event.movementY = currentMouseY_ - lastMouseY_;
    event.modifiers = currentModifiers_;
    event.timestamp = lastUpdateTime_;

    dispatchEvent(event);
}

void InputSystem::injectMouseButton(int button, bool pressed, const ModifierKeys& modifiers) {
    currentModifiers_ = modifiers;

    InputEventType eventType = pressed ? InputEventType::PointerDown : InputEventType::PointerUp;
    if (!isEventEnabled(eventType)) {
        return;
    }

    InputEvent event;
    event.type = eventType;
    event.deviceType = InputDeviceType::Mouse;
    event.button = static_cast<MouseButton>(button);
    event.buttonPressed = pressed;
    event.mouseX = currentMouseX_;
    event.mouseY = currentMouseY_;
    event.normalizedX = currentMouseX_ / static_cast<float>(windowWidth_);
    event.normalizedY = currentMouseY_ / static_cast<float>(windowHeight_);
    event.modifiers = currentModifiers_;
    event.timestamp = lastUpdateTime_;

    dispatchEvent(event);

    // Also dispatch click event on button release
    if (!pressed && isEventEnabled(InputEventType::PointerClick)) {
        InputEvent clickEvent = event;
        clickEvent.type = InputEventType::PointerClick;
        dispatchEvent(clickEvent);
    }
}

void InputSystem::injectScroll(float deltaX, float deltaY) {
    if (!isEventEnabled(InputEventType::Scroll)) {
        return;
    }

    InputEvent event;
    event.type = InputEventType::Scroll;
    event.deviceType = InputDeviceType::Mouse;
    event.deltaX = deltaX;
    event.deltaY = deltaY;
    event.mouseX = currentMouseX_;
    event.mouseY = currentMouseY_;
    event.normalizedX = currentMouseX_ / static_cast<float>(windowWidth_);
    event.normalizedY = currentMouseY_ / static_cast<float>(windowHeight_);
    event.modifiers = currentModifiers_;
    event.timestamp = lastUpdateTime_;

    dispatchEvent(event);
}

void InputSystem::injectKey(int keyCode, int scanCode, bool pressed, const ModifierKeys& modifiers) {
    currentModifiers_ = modifiers;

    InputEventType eventType = pressed ? InputEventType::KeyDown : InputEventType::KeyUp;
    if (!isEventEnabled(eventType)) {
        return;
    }

    InputEvent event;
    event.type = eventType;
    event.deviceType = InputDeviceType::Mouse;
    event.keyCode = keyCode;
    event.scanCode = scanCode;
    event.modifiers = currentModifiers_;
    event.timestamp = lastUpdateTime_;

    dispatchEvent(event);
}

// ============================================================================
// Helper Methods
// ============================================================================

void InputSystem::dispatchEvent(const InputEvent& event) {
    std::lock_guard<std::mutex> lock(listenersMutex_);

    for (const auto& listener : listeners_) {
        if (listener.type == event.type && listener.callback) {
            listener.callback(event);
        }
    }
}

} // namespace clipengine
