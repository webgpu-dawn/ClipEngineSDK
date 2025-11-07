#include "Application.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[])
{
    Application app;
    app.initialize();

    if (!app.isInitialized()) {
        std::cerr << "Failed to initialize application. Exiting." << std::endl;
        return 1;
    }

    // Check for --test-export flag to automatically trigger export
    bool testExport = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-export") {
            testExport = true;
            break;
        }
    }

    if (testExport) {
        std::cout << "\n=== AUTO-TEST MODE: Will trigger export in 2 seconds ===\n" << std::endl;
        // Give video time to load before starting export
        std::this_thread::sleep_for(std::chrono::seconds(2));
        app.testExport();
        std::cout << "\n=== Export test completed ===\n" << std::endl;
        return 0;
    }

    app.run();
    return 0;
}