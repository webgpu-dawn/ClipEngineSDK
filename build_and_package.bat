@echo off
REM ClipEngine SDK - One-Click Build and Package Script
REM This script builds Debug and Release configurations and creates a distribution package

echo =====================================================
echo   ClipEngine SDK - Build and Package Script
echo =====================================================
echo.

REM Run PowerShell script with execution policy bypass
powershell -ExecutionPolicy Bypass -File "%~dp0build_and_package.ps1"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Press any key to exit...
    pause >nul
) else (
    echo.
    echo Build failed! Press any key to exit...
    pause >nul
    exit /b 1
)
