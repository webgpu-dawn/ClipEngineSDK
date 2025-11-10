# ClipEngine SDK - One-Click Build and Package Script
# This script builds Debug and Release configurations and creates a distribution package

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8
chcp 65001 | Out-Null

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host "  ClipEngine SDK - Build and Package Script" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Stop"
$SDK_ROOT = $PSScriptRoot
$PACKAGE_ROOT = "$SDK_ROOT\ClipEngineSDK-Package"
$BUILD_DIR = "$SDK_ROOT\build"

# Step 1: Configure CMake if needed
Write-Host "[1/6] Checking CMake configuration..." -ForegroundColor Yellow
if (-not (Test-Path $BUILD_DIR)) {
    Write-Host "  Build directory not found, running CMake configuration..." -ForegroundColor Gray
    Push-Location $SDK_ROOT
    try {
        cmake -S . -B build
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
        Write-Host "  [OK] CMake configuration completed" -ForegroundColor Green
    } catch {
        Write-Host "  [FAIL] CMake configuration failed: $_" -ForegroundColor Red
        Pop-Location
        exit 1
    }
    Pop-Location
}

# Step 2: Build Debug Configuration
Write-Host ""
Write-Host "[2/6] Building SDK - Debug Configuration..." -ForegroundColor Yellow
Push-Location $SDK_ROOT
try {
    cmake --build build --config Debug
    if ($LASTEXITCODE -ne 0) {
        throw "Debug build failed"
    }
    Write-Host "  [OK] Debug build completed successfully" -ForegroundColor Green
} catch {
    Write-Host "  [FAIL] Debug build failed: $_" -ForegroundColor Red
    Pop-Location
    exit 1
}
Pop-Location

# Step 3: Build Release Configuration
Write-Host ""
Write-Host "[3/6] Building SDK - Release Configuration..." -ForegroundColor Yellow
Push-Location $SDK_ROOT
try {
    cmake --build build --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "Release build failed"
    }
    Write-Host "  [OK] Release build completed successfully" -ForegroundColor Green
} catch {
    Write-Host "  [FAIL] Release build failed: $_" -ForegroundColor Red
    Pop-Location
    exit 1
}
Pop-Location

# Step 4: Clean and Create Package Directory
Write-Host ""
Write-Host "[4/6] Creating package directory..." -ForegroundColor Yellow
if (Test-Path $PACKAGE_ROOT) {
    Write-Host "  Removing old package directory..." -ForegroundColor Gray
    try {
        Remove-Item -Recurse -Force $PACKAGE_ROOT -ErrorAction Stop
    } catch {
        Write-Host "  Warning: Could not remove some files (may be in use). Attempting to overwrite..." -ForegroundColor Yellow
    }
}
New-Item -ItemType Directory -Path $PACKAGE_ROOT -Force | Out-Null
Write-Host "  [OK] Package directory ready: $PACKAGE_ROOT" -ForegroundColor Green

# Step 5: Copy SDK Files
Write-Host ""
Write-Host "[5/6] Copying SDK files..." -ForegroundColor Yellow

# Copy Debug SDK
Write-Host "  Copying Debug SDK..." -ForegroundColor Gray
Copy-Item -Path "$SDK_ROOT\output\ClipEngineSDK-Debug" -Destination "$PACKAGE_ROOT\Debug" -Recurse -Force
Write-Host "    [OK] Debug SDK copied" -ForegroundColor Green

# Copy Release SDK
Write-Host "  Copying Release SDK..." -ForegroundColor Gray
Copy-Item -Path "$SDK_ROOT\output\ClipEngineSDK-Release" -Destination "$PACKAGE_ROOT\Release" -Recurse -Force
Write-Host "    [OK] Release SDK copied" -ForegroundColor Green

# Copy Samples (excluding build directory only)
Write-Host "  Copying samples..." -ForegroundColor Gray
New-Item -ItemType Directory -Path "$PACKAGE_ROOT\samples" -Force | Out-Null
Get-ChildItem -Path "$SDK_ROOT\samples" | Where-Object {
    $_.Name -ne "build"
} | ForEach-Object {
    Copy-Item -Path $_.FullName -Destination "$PACKAGE_ROOT\samples\" -Recurse -Force
}
Write-Host "    [OK] Samples copied (including deps)" -ForegroundColor Green

# Copy Documentation (if exists)
$hasDistribution = Test-Path "$SDK_ROOT\DISTRIBUTION.md"
$hasChecklist = Test-Path "$SDK_ROOT\DISTRIBUTION_CHECKLIST.md"
if ($hasDistribution -or $hasChecklist) {
    Write-Host "  Copying documentation..." -ForegroundColor Gray
    New-Item -ItemType Directory -Path "$PACKAGE_ROOT\docs" -Force | Out-Null
    if ($hasDistribution) {
        Copy-Item -Path "$SDK_ROOT\DISTRIBUTION.md" -Destination "$PACKAGE_ROOT\docs\" -Force
    }
    if ($hasChecklist) {
        Copy-Item -Path "$SDK_ROOT\DISTRIBUTION_CHECKLIST.md" -Destination "$PACKAGE_ROOT\docs\" -Force
    }
    Write-Host "    [OK] Documentation copied" -ForegroundColor Green
}

# Step 6: Create README
Write-Host ""
Write-Host "[6/6] Creating package README..." -ForegroundColor Yellow
$readme = @"
# ClipEngine SDK - Complete Distribution Package

**Version**: 1.0.0
**Build Date**: $(Get-Date -Format "yyyy-MM-dd")
**Platform**: Windows x64

## 📦 Package Contents

``````
ClipEngineSDK-Package/
├── Debug/              # Debug configuration SDK
│   ├── bin/            # Runtime DLLs
│   ├── lib/            # Static libraries
│   ├── include/        # Header files
│   └── share/          # Shaders and resources
│
├── Release/            # Release configuration SDK
│   ├── bin/            # Runtime DLLs (optimized)
│   ├── lib/            # Static libraries (optimized)
│   ├── include/        # Header files
│   └── share/          # Shaders and resources
│
├── samples/            # Sample application code
│   ├── CMakeLists.txt  # Build configuration
│   ├── *.cpp/h         # Source files
│   ├── deps/           # Dependencies (ffmpeg, etc.)
│   └── assets/         # Sample assets
│
└── README.md           # This file
``````

## 🚀 Quick Start

### Option 1: One-Click Build (Recommended)

Navigate to the `samples` folder and double-click:
- **build_debug.bat** - Build with Debug SDK
- **build_release.bat** - Build with Release SDK

The script will automatically configure, build, and ask if you want to run the sample.

### Option 2: Manual Build

#### Build Sample with Debug SDK
``````bash
cd samples
cmake -S . -B build -DCMAKE_PREFIX_PATH="../Debug"
cmake --build build --config Debug
cd build/Debug/samples
./Sample_sdk_demo.exe
``````

#### Build Sample with Release SDK
``````bash
cd samples
cmake -S . -B build -DCMAKE_PREFIX_PATH="../Release"
cmake --build build --config Release
cd build/Release/samples
./Sample_sdk_demo.exe
``````

See `samples/BUILD_INSTRUCTIONS.md` for detailed build instructions.

## 🔧 System Requirements

- **OS**: Windows 10/11 (64-bit)
- **GPU**: DirectX 12 or Vulkan compatible
- **Runtime**: Visual C++ 2022 Redistributable (x64)
- **Development**: Visual Studio 2019/2022 or CMake 3.13+

---

**Copyright © 2025 All rights reserved.**
"@

Set-Content -Path "$PACKAGE_ROOT\README.md" -Value $readme -Encoding UTF8
Write-Host "  [OK] README.md created" -ForegroundColor Green

# Summary
Write-Host ""
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host "  Package Created Successfully!" -ForegroundColor Green
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Package location: $PACKAGE_ROOT" -ForegroundColor White
Write-Host ""

# Verify package contents
Write-Host "Package contents:" -ForegroundColor Yellow
Write-Host "  Debug SDK:" -ForegroundColor Gray
$debugDlls = (Get-ChildItem -Path "$PACKAGE_ROOT\Debug\bin" -Filter "*.dll").Count
$debugLibs = (Get-ChildItem -Path "$PACKAGE_ROOT\Debug\lib" -Filter "*.lib").Count
$debugShaders = (Get-ChildItem -Path "$PACKAGE_ROOT\Debug\share\clipengine\shaders" -Filter "*.wgsl").Count
Write-Host "    - $debugDlls DLLs" -ForegroundColor White
Write-Host "    - $debugLibs LIBs" -ForegroundColor White
Write-Host "    - $debugShaders Shaders" -ForegroundColor White

Write-Host "  Release SDK:" -ForegroundColor Gray
$releaseDlls = (Get-ChildItem -Path "$PACKAGE_ROOT\Release\bin" -Filter "*.dll").Count
$releaseLibs = (Get-ChildItem -Path "$PACKAGE_ROOT\Release\lib" -Filter "*.lib").Count
$releaseShaders = (Get-ChildItem -Path "$PACKAGE_ROOT\Release\share\clipengine\shaders" -Filter "*.wgsl").Count
Write-Host "    - $releaseDlls DLLs" -ForegroundColor White
Write-Host "    - $releaseLibs LIBs" -ForegroundColor White
Write-Host "    - $releaseShaders Shaders" -ForegroundColor White

Write-Host "  Samples:" -ForegroundColor Gray
$sampleFiles = (Get-ChildItem -Path "$PACKAGE_ROOT\samples" -File).Count
Write-Host "    - $sampleFiles files" -ForegroundColor White

if (Test-Path "$PACKAGE_ROOT\docs") {
    Write-Host "  Documentation:" -ForegroundColor Gray
    $docFiles = (Get-ChildItem -Path "$PACKAGE_ROOT\docs").Count
    Write-Host "    - $docFiles files" -ForegroundColor White
}

Write-Host ""
Write-Host "[OK] Ready to distribute!" -ForegroundColor Green
Write-Host ""
