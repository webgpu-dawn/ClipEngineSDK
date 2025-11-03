@echo off
REM Build script for ClipEngine and all samples

echo ========================================
echo Building ClipEngine and Samples
echo ========================================
echo.

REM Step 1: Build main ClipEngine library
echo [1/3] Building ClipEngine library...
cd build
cmake --build . --config Debug
if %ERRORLEVEL% NEQ 0 (
    echo Failed to build ClipEngine library!
    exit /b 1
)
cd ..

echo.
echo [2/3] Building input_system_demo...
cd samples\input_system_demo
if not exist build (mkdir build)
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug
if %ERRORLEVEL% NEQ 0 (
    echo Failed to build input_system_demo!
    cd ..\..\..
    exit /b 1
)
cd ..\..\..

echo.
echo [3/3] Building sdk_demo...
cd samples\sdk_demo
if not exist build (mkdir build)
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug
if %ERRORLEVEL% NEQ 0 (
    echo Failed to build sdk_demo!
    cd ..\..\..
    exit /b 1
)
cd ..\..\..

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Executables:
echo   - samples\input_system_demo\build\bin\Debug\input_system_demo.exe
echo   - samples\sdk_demo\build\Debug\sdk_demo.exe
echo.
pause
