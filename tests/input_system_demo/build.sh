#!/bin/bash

# Build script for InputSystem Demo (Linux/macOS)

echo "========================================"
echo "  Building InputSystem Demo"
echo "========================================"
echo ""

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    cd ..
    exit 1
fi

echo ""
echo "Building..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo "Build failed!"
    cd ..
    exit 1
fi

cd ..

echo ""
echo "========================================"
echo "  Build completed successfully!"
echo "========================================"
echo ""
echo "Executable: build/input_system_demo"
echo ""
echo "Run the demo with:"
echo "  ./build/input_system_demo"
echo ""
