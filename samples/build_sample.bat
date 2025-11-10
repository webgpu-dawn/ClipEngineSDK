@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo =====================================================
echo   ClipEngine SDK - Sample Build Script
echo =====================================================
echo.

REM Check if we're in the packaged SDK or source tree
set "SCRIPT_DIR=%~dp0"
set "CONFIG=Debug"

REM Check for command line argument
if not "%1"=="" (
    if /i "%1"=="Debug" set "CONFIG=Debug"
    if /i "%1"=="Release" set "CONFIG=Release"
    if /i "%1"=="debug" set "CONFIG=Debug"
    if /i "%1"=="release" set "CONFIG=Release"
    goto :build_start
)

REM Interactive menu if no argument provided
echo Please select SDK configuration:
echo   1. Debug   (with debug symbols)
echo   2. Release (optimized)
echo.
set /p "choice=Enter choice (1 or 2, default=1): "

if "%choice%"=="2" (
    set "CONFIG=Release"
    echo Selected: Release
) else (
    set "CONFIG=Debug"
    echo Selected: Debug
)
echo.

:build_start
REM Determine SDK path
if exist "%SCRIPT_DIR%..\Debug\" (
    REM We're in packaged SDK
    set "SDK_PATH=%SCRIPT_DIR%..\%CONFIG%"
    echo [INFO] Using packaged SDK: %CONFIG%
) else if exist "%SCRIPT_DIR%..\output\ClipEngineSDK-%CONFIG%\" (
    REM We're in source tree
    set "SDK_PATH=%SCRIPT_DIR%..\output\ClipEngineSDK-%CONFIG%"
    echo [INFO] Using source tree SDK: %CONFIG%
) else (
    echo [FAIL] SDK not found. Please build the SDK first.
    echo Expected location: %SCRIPT_DIR%..\output\ClipEngineSDK-%CONFIG%
    pause
    exit /b 1
)

echo SDK Location: !SDK_PATH!
echo.

REM Clean old build if exists
if exist "%SCRIPT_DIR%build\" (
    echo [1/3] Cleaning old build directory...
    rmdir /s /q "%SCRIPT_DIR%build" 2>nul
    echo [OK] Old build cleaned
    echo.
)

REM Configure CMake
echo [1/3] Configuring CMake...
pushd "%SCRIPT_DIR%"
cmake -S . -B build -DCMAKE_PREFIX_PATH="!SDK_PATH!" -DCMAKE_BUILD_TYPE=%CONFIG%
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] CMake configuration failed
    popd
    pause
    exit /b 1
)
echo [OK] CMake configuration completed
popd
echo.

REM Build
echo [2/3] Building sample (%CONFIG%)...
pushd "%SCRIPT_DIR%"
cmake --build build --config %CONFIG%
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Build failed
    popd
    pause
    exit /b 1
)
echo [OK] Build completed successfully
popd
echo.

REM Show output location
echo [3/3] Build Summary
echo =====================================================
echo Configuration: %CONFIG%
echo Executable: %SCRIPT_DIR%build\%CONFIG%\samples\Sample_sdk_demo.exe
echo.

REM Ask if user wants to run
set /p "run_choice=Run the sample now? (Y/N, default=N): "
if /i "%run_choice%"=="Y" (
    echo.
    echo [RUN] Starting Sample_sdk_demo.exe...
    echo =====================================================
    echo.
    pushd "%SCRIPT_DIR%build\%CONFIG%\samples"
    Sample_sdk_demo.exe
    popd
) else (
    echo.
    echo [INFO] To run the sample manually:
    echo   cd %SCRIPT_DIR%build\%CONFIG%\samples
    echo   Sample_sdk_demo.exe
)

echo.
echo =====================================================
echo [OK] Done!
echo =====================================================
pause
