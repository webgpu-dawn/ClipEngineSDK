/**
 * InputSystem Demo - Comprehensive test of ClipEngine's window-agnostic input system
 *
 * This demo demonstrates:
 * 1. Using InputSystem with GLFW adapter
 * 2. Event listener registration and handling
 * 3. Mouse, keyboard, and scroll events
 * 4. Modifier key handling
 * 5. Using platform-independent KeyCode
 * 6. Dynamic event enabling/disabling
 */

#include <clipengine/input/InputSystem.h>
#include <clipengine/input/InputSystemGLFWAdapter.h>
#include <clipengine/input/KeyCode.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>

using namespace clipengine;

// Demo state
struct DemoState {
    bool isDragging = false;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;
    float zoom = 1.0f;
    int clickCount = 0;
    bool mouseMoveEnabled = true;

    // Event log
    std::vector<std::string> eventLog;
    const size_t MAX_LOG_SIZE = 10;

    void addLog(const std::string& message) {
        eventLog.push_back(message);
        if (eventLog.size() > MAX_LOG_SIZE) {
            eventLog.erase(eventLog.begin());
        }
    }
};

DemoState state;

// Helper to format modifier keys
std::string formatModifiers(const ModifierKeys& mods) {
    std::stringstream ss;
    if (mods.ctrl) ss << "Ctrl+";
    if (mods.shift) ss << "Shift+";
    if (mods.alt) ss << "Alt+";
    if (mods.meta) ss << "Meta+";
    std::string result = ss.str();
    if (!result.empty() && result.back() == '+') {
        result.pop_back();
    }
    return result.empty() ? "None" : result;
}

// Helper to get button name
const char* getButtonName(MouseButton button) {
    switch (button) {
        case MouseButton::Left: return "Left";
        case MouseButton::Right: return "Right";
        case MouseButton::Middle: return "Middle";
        case MouseButton::Button4: return "Button4";
        case MouseButton::Button5: return "Button5";
        default: return "Unknown";
    }
}

// Setup all event listeners
void setupEventListeners(InputSystem& inputSystem) {
    // Pointer Click Event
    inputSystem.addEventListener(InputEventType::PointerClick, [](const InputEvent& event) {
        state.clickCount++;
        std::stringstream ss;
        ss << "[CLICK] Button: " << getButtonName(event.button)
           << " at (" << std::fixed << std::setprecision(0)
           << event.mouseX << ", " << event.mouseY << ")";
        state.addLog(ss.str());

        std::cout << "\n=== POINTER CLICK ===" << std::endl;
        std::cout << "  Button: " << getButtonName(event.button) << std::endl;
        std::cout << "  Position: (" << event.mouseX << ", " << event.mouseY << ")" << std::endl;
        std::cout << "  Normalized: (" << event.normalizedX << ", " << event.normalizedY << ")" << std::endl;
        std::cout << "  Modifiers: " << formatModifiers(event.modifiers) << std::endl;
        std::cout << "  Total clicks: " << state.clickCount << std::endl;
    });

    // Pointer Down Event (for drag start)
    inputSystem.addEventListener(InputEventType::PointerDown, [](const InputEvent& event) {
        if (event.button == MouseButton::Left) {
            state.isDragging = true;
            state.dragStartX = event.mouseX;
            state.dragStartY = event.mouseY;

            std::stringstream ss;
            ss << "[DRAG START] at (" << std::fixed << std::setprecision(0)
               << event.mouseX << ", " << event.mouseY << ")";
            state.addLog(ss.str());

            std::cout << "\n--- Drag Started ---" << std::endl;
        }
    });

    // Pointer Up Event (for drag end)
    inputSystem.addEventListener(InputEventType::PointerUp, [](const InputEvent& event) {
        if (event.button == MouseButton::Left && state.isDragging) {
            state.isDragging = false;

            float dx = event.mouseX - state.dragStartX;
            float dy = event.mouseY - state.dragStartY;

            std::stringstream ss;
            ss << "[DRAG END] Delta: (" << std::fixed << std::setprecision(0)
               << dx << ", " << dy << ")";
            state.addLog(ss.str());

            std::cout << "--- Drag Ended ---" << std::endl;
            std::cout << "  Drag delta: (" << dx << ", " << dy << ")" << std::endl;
        }
    });

    // Pointer Move Event (throttled output)
    static int moveCounter = 0;
    inputSystem.addEventListener(InputEventType::PointerMove, [](const InputEvent& event) {
        // Only log every 30th move event to avoid spam
        moveCounter++;
        if (moveCounter % 30 == 0) {
            if (state.isDragging) {
                float dx = event.mouseX - state.dragStartX;
                float dy = event.mouseY - state.dragStartY;
                std::cout << "\r  Dragging... offset: ("
                         << std::fixed << std::setprecision(0)
                         << dx << ", " << dy << ")          " << std::flush;
            }
        }
    });

    // Scroll Event (for zoom)
    inputSystem.addEventListener(InputEventType::Scroll, [](const InputEvent& event) {
        const float zoomSpeed = 0.1f;
        state.zoom += event.deltaY * zoomSpeed;
        state.zoom = std::max(0.1f, std::min(5.0f, state.zoom));

        std::stringstream ss;
        ss << "[SCROLL] Zoom: " << std::fixed << std::setprecision(2) << state.zoom << "x";
        state.addLog(ss.str());

        std::cout << "\n=== SCROLL ===" << std::endl;
        std::cout << "  Delta: (" << event.deltaX << ", " << event.deltaY << ")" << std::endl;
        std::cout << "  New zoom: " << state.zoom << "x" << std::endl;
    });

    // Key Down Event
    inputSystem.addEventListener(InputEventType::KeyDown, [&inputSystem](const InputEvent& event) {
        std::stringstream ss;
        ss << "[KEY DOWN] Code: " << event.keyCode;
        state.addLog(ss.str());

        std::cout << "\n=== KEY DOWN ===" << std::endl;
        std::cout << "  Key Code: " << event.keyCode << std::endl;
        std::cout << "  Scan Code: " << event.scanCode << std::endl;
        std::cout << "  Modifiers: " << formatModifiers(event.modifiers) << std::endl;

        // Handle specific keys
        if (event.keyCode == KeyCode::Space) {
            std::cout << "  >> SPACE key pressed!" << std::endl;
        }

        if (event.keyCode == KeyCode::Escape) {
            std::cout << "  >> ESC key pressed!" << std::endl;
        }

        // Toggle mouse move events with 'M' key
        if (event.keyCode == KeyCode::M) {
            state.mouseMoveEnabled = !state.mouseMoveEnabled;
            inputSystem.setEventEnabled(InputEventType::PointerMove, state.mouseMoveEnabled);
            std::cout << "  >> Mouse move events: "
                     << (state.mouseMoveEnabled ? "ENABLED" : "DISABLED") << std::endl;
        }

        // Ctrl+S - Save example
        if (event.isCtrlPressed() && event.keyCode == KeyCode::S) {
            std::cout << "  >> Ctrl+S detected - Save action!" << std::endl;
        }

        // Ctrl+Z - Undo example
        if (event.isCtrlPressed() && event.keyCode == KeyCode::Z) {
            std::cout << "  >> Ctrl+Z detected - Undo action!" << std::endl;
        }

        // WASD movement example
        if (event.keyCode == KeyCode::W) {
            std::cout << "  >> W - Move forward" << std::endl;
        }
        if (event.keyCode == KeyCode::A) {
            std::cout << "  >> A - Move left" << std::endl;
        }
        if (event.keyCode == KeyCode::S) {
            std::cout << "  >> S - Move backward" << std::endl;
        }
        if (event.keyCode == KeyCode::D) {
            std::cout << "  >> D - Move right" << std::endl;
        }
    });

    // Key Up Event
    inputSystem.addEventListener(InputEventType::KeyUp, [](const InputEvent& event) {
        // Only log key up for specific keys to reduce spam
        if (event.keyCode == KeyCode::Space || event.keyCode == KeyCode::Escape) {
            std::cout << "--- KEY UP: " << event.keyCode << " ---" << std::endl;
        }
    });
}

// Print help information
void printHelp() {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "  InputSystem Demo - Interactive Test  " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  Mouse Left Click    - Generate click event" << std::endl;
    std::cout << "  Mouse Left Drag     - Drag operation" << std::endl;
    std::cout << "  Mouse Wheel         - Zoom in/out" << std::endl;
    std::cout << "  W/A/S/D            - Movement keys" << std::endl;
    std::cout << "  M                  - Toggle mouse move events" << std::endl;
    std::cout << "  Space              - Test key event" << std::endl;
    std::cout << "  Ctrl+S             - Save action" << std::endl;
    std::cout << "  Ctrl+Z             - Undo action" << std::endl;
    std::cout << "  ESC                - Quit demo" << std::endl;
    std::cout << "\nFeatures being tested:" << std::endl;
    std::cout << "  ✓ Event listener registration" << std::endl;
    std::cout << "  ✓ Mouse click/drag detection" << std::endl;
    std::cout << "  ✓ Keyboard input with modifiers" << std::endl;
    std::cout << "  ✓ Scroll wheel handling" << std::endl;
    std::cout << "  ✓ Platform-independent KeyCode" << std::endl;
    std::cout << "  ✓ Dynamic event enable/disable" << std::endl;
    std::cout << "  ✓ Normalized coordinates [0,1]" << std::endl;
    std::cout << "  ✓ GLFW adapter integration" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

// Print status information
void printStatus(InputSystem& inputSystem) {
    std::cout << "\n--- Status Update ---" << std::endl;

    // Mouse position
    float mouseX, mouseY;
    inputSystem.getMousePosition(mouseX, mouseY);
    std::cout << "Mouse Position: (" << mouseX << ", " << mouseY << ")" << std::endl;

    // Normalized position
    float normX, normY;
    inputSystem.getMousePositionNormalized(normX, normY);
    std::cout << "Normalized: (" << std::fixed << std::setprecision(3)
             << normX << ", " << normY << ")" << std::endl;

    // Modifier keys
    const auto& mods = inputSystem.getModifierKeys();
    std::cout << "Modifiers: " << formatModifiers(mods) << std::endl;

    // Demo state
    std::cout << "Zoom: " << std::setprecision(2) << state.zoom << "x" << std::endl;
    std::cout << "Total Clicks: " << state.clickCount << std::endl;
    std::cout << "Mouse Move Events: " << (state.mouseMoveEnabled ? "ON" : "OFF") << std::endl;
    std::cout << "Dragging: " << (state.isDragging ? "YES" : "NO") << std::endl;

    // Recent events
    if (!state.eventLog.empty()) {
        std::cout << "\nRecent Events:" << std::endl;
        for (const auto& log : state.eventLog) {
            std::cout << "  " << log << std::endl;
        }
    }

    std::cout << "---------------------" << std::endl;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create window
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT,
        "InputSystem Demo - Move mouse, click, type keys, scroll!",
        nullptr, nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize InputSystem
    InputSystem inputSystem;
    if (!inputSystem.initialize(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        std::cerr << "Failed to initialize InputSystem" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Attach GLFW adapter
    InputSystemGLFWAdapter adapter(&inputSystem);
    adapter.attach(window);

    // Setup event listeners
    setupEventListeners(inputSystem);

    // Print help
    printHelp();

    std::cout << "Window created. Start interacting!" << std::endl;
    std::cout << "Press ESC to quit.\n" << std::endl;

    // Track time for status updates
    double lastStatusTime = glfwGetTime();
    double lastFrameTime = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        // Update InputSystem
        inputSystem.update(deltaTime);

        // Print status every 5 seconds
        if (currentTime - lastStatusTime > 5.0) {
            printStatus(inputSystem);
            lastStatusTime = currentTime;
        }

        // Handle window resize
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        // Simple rendering (clear to dark gray)
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Check for ESC key to quit
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    // Cleanup
    std::cout << "\n=== Demo Summary ===" << std::endl;
    std::cout << "Total clicks: " << state.clickCount << std::endl;
    std::cout << "Final zoom: " << state.zoom << "x" << std::endl;
    std::cout << "===================" << std::endl;

    adapter.detach();
    inputSystem.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "\nThank you for testing InputSystem!" << std::endl;

    return 0;
}
