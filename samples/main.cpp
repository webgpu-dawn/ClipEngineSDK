#include "Application.h"
#include <iostream>

int main()
{
    Application app;
    app.initialize();

    if (!app.isInitialized()) {
        std::cerr << "Failed to initialize application. Exiting." << std::endl;
        return 1;
    }

    app.run();
    return 0;
}