@echo off
REM Quick run script for InputSystem Demo (Windows)

if exist build\Debug\input_system_demo.exe (
    echo Running InputSystem Demo (Debug)...
    build\Debug\input_system_demo.exe
) else if exist build\Release\input_system_demo.exe (
    echo Running InputSystem Demo (Release)...
    build\Release\input_system_demo.exe
) else (
    echo No executable found! Please build first with build.bat
    pause
    exit /b 1
)
