@echo off
REM Build script for InputSystem Demo (Windows)

echo ========================================
echo   Building InputSystem Demo (Windows)
echo ========================================
echo.

REM Create build directory
if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Building Debug configuration...
cmake --build . --config Debug
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Building Release configuration...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo   Build completed successfully!
echo ========================================
echo.
echo Debug executable:   build\Debug\input_system_demo.exe
echo Release executable: build\Release\input_system_demo.exe
echo.
echo Run the demo with:
echo   build\Debug\input_system_demo.exe
echo   or
echo   build\Release\input_system_demo.exe
echo.

pause
