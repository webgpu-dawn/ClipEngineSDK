@echo off
REM ClipEngine Example 部署脚本（批处理版本）
REM 此脚本将 SDK 和必要的依赖复制到 example/deps 目录

setlocal enabledelayedexpansion

echo ======================================
echo ClipEngine Example 部署脚本
echo ======================================
echo.

set PROJECT_ROOT=%~dp0
set EXAMPLE_DIR=%PROJECT_ROOT%example
set DEPS_DIR=%EXAMPLE_DIR%\deps

echo [1/5] 检查目录...

if not exist "%EXAMPLE_DIR%" (
    echo 错误: example 目录不存在！
    exit /b 1
)

if not exist "%DEPS_DIR%" (
    echo 创建 deps 目录...
    mkdir "%DEPS_DIR%"
)

echo [2/5] 复制 ClipEngine SDK...

REM 复制 Debug SDK
if exist "%PROJECT_ROOT%output\ClipEngineSDK-Debug" (
    echo   复制 ClipEngineSDK-Debug...
    if exist "%DEPS_DIR%\ClipEngineSDK-Debug" (
        rmdir /s /q "%DEPS_DIR%\ClipEngineSDK-Debug"
    )
    xcopy /E /I /Y /Q "%PROJECT_ROOT%output\ClipEngineSDK-Debug" "%DEPS_DIR%\ClipEngineSDK-Debug" >nul
    echo   ✓ ClipEngineSDK-Debug 复制完成
) else (
    echo   ⚠ ClipEngineSDK-Debug 不存在，跳过
)

REM 复制 Release SDK
if exist "%PROJECT_ROOT%output\ClipEngineSDK-Release" (
    echo   复制 ClipEngineSDK-Release...
    if exist "%DEPS_DIR%\ClipEngineSDK-Release" (
        rmdir /s /q "%DEPS_DIR%\ClipEngineSDK-Release"
    )
    xcopy /E /I /Y /Q "%PROJECT_ROOT%output\ClipEngineSDK-Release" "%DEPS_DIR%\ClipEngineSDK-Release" >nul
    echo   ✓ ClipEngineSDK-Release 复制完成
) else (
    echo   ⚠ ClipEngineSDK-Release 不存在，跳过
)

echo [3/5] 检查 GLFW...

if not exist "%DEPS_DIR%\glfw" (
    if exist "%PROJECT_ROOT%deps\glfw" (
        echo   复制 GLFW...
        xcopy /E /I /Y /Q "%PROJECT_ROOT%deps\glfw" "%DEPS_DIR%\glfw" >nul
        echo   ✓ GLFW 复制完成
    ) else (
        echo   ⚠ GLFW 源目录不存在
    )
) else (
    echo   ✓ GLFW 已存在
)

echo [4/5] 检查 FFmpeg...

if exist "%DEPS_DIR%\ffmpeg_x64-windows" (
    echo   ✓ FFmpeg 已存在
) else (
    echo   ⚠ 警告: FFmpeg 未找到
    echo     请手动将 FFmpeg 复制到: %DEPS_DIR%\ffmpeg_x64-windows
    echo     或使用 vcpkg 安装: vcpkg install ffmpeg:x64-windows
)

echo [5/5] 验证部署...

set ALL_GOOD=1
if not exist "%DEPS_DIR%\ClipEngineSDK-Debug\lib\clipengine.lib" (
    echo   ✗ 缺少: ClipEngineSDK-Debug\lib\clipengine.lib
    set ALL_GOOD=0
)
if not exist "%DEPS_DIR%\ClipEngineSDK-Debug\include\clipengine\clipengine.h" (
    echo   ✗ 缺少: ClipEngineSDK-Debug\include\clipengine\clipengine.h
    set ALL_GOOD=0
)
if not exist "%DEPS_DIR%\ClipEngineSDK-Debug\bin\webgpu_dawn.dll" (
    echo   ✗ 缺少: ClipEngineSDK-Debug\bin\webgpu_dawn.dll
    set ALL_GOOD=0
)

echo.
echo ======================================

if %ALL_GOOD%==1 (
    echo ✓ 部署成功！
    echo.
    echo 下一步操作：
    echo   1. cd example
    echo   2. cmake -B build
    echo   3. cmake --build build --config Debug
    echo.
) else (
    echo ✗ 部署未完成，请检查错误信息
    exit /b 1
)

echo ======================================
