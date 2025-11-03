#!/bin/bash

# Quick run script for InputSystem Demo (Linux/macOS)

if [ -f "build/input_system_demo" ]; then
    echo "Running InputSystem Demo..."
    ./build/input_system_demo
else
    echo "No executable found! Please build first with ./build.sh"
    exit 1
fi
