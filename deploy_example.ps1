#!/usr/bin/env pwsh
# ClipEngine Example 部署脚本
# 此脚本将 SDK 和必要的依赖复制到 example/deps 目录

param(
    [string]$Config = "Debug"  # 可选: Debug 或 Release
)

$ErrorActionPreference = "Stop"

Write-Host "======================================" -ForegroundColor Cyan
Write-Host "ClipEngine Example 部署脚本" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

# 获取脚本所在目录（项目根目录）
$ProjectRoot = $PSScriptRoot
$ExampleDir = Join-Path $ProjectRoot "example"
$DepsDir = Join-Path $ExampleDir "deps"

Write-Host "[1/5] 检查目录..." -ForegroundColor Yellow

# 确保 example 目录存在
if (-not (Test-Path $ExampleDir)) {
    Write-Host "错误: example 目录不存在！" -ForegroundColor Red
    exit 1
}

# 创建 deps 目录
if (-not (Test-Path $DepsDir)) {
    Write-Host "创建 deps 目录..." -ForegroundColor Green
    New-Item -ItemType Directory -Path $DepsDir -Force | Out-Null
}

Write-Host "[2/5] 复制 ClipEngine SDK..." -ForegroundColor Yellow

# 复制 ClipEngine SDK
$SDKConfigs = @("Debug", "Release")
foreach ($cfg in $SDKConfigs) {
    $SrcSDK = Join-Path $ProjectRoot "output\ClipEngineSDK-$cfg"
    $DstSDK = Join-Path $DepsDir "ClipEngineSDK-$cfg"

    if (Test-Path $SrcSDK) {
        Write-Host "  复制 ClipEngineSDK-$cfg..." -ForegroundColor Green

        # 删除旧的 SDK
        if (Test-Path $DstSDK) {
            Remove-Item -Path $DstSDK -Recurse -Force
        }

        # 复制新的 SDK
        Copy-Item -Path $SrcSDK -Destination $DstSDK -Recurse -Force

        Write-Host "  ✓ ClipEngineSDK-$cfg 复制完成" -ForegroundColor Green
    } else {
        Write-Host "  ⚠ ClipEngineSDK-$cfg 不存在，跳过" -ForegroundColor Yellow
    }
}

Write-Host "[3/5] 检查 GLFW..." -ForegroundColor Yellow

# 复制 GLFW（如果不存在）
$GLFWSrc = Join-Path $ProjectRoot "deps\glfw"
$GLFWDst = Join-Path $DepsDir "glfw"

if (-not (Test-Path $GLFWDst)) {
    if (Test-Path $GLFWSrc) {
        Write-Host "  复制 GLFW..." -ForegroundColor Green
        Copy-Item -Path $GLFWSrc -Destination $GLFWDst -Recurse -Force
        Write-Host "  ✓ GLFW 复制完成" -ForegroundColor Green
    } else {
        Write-Host "  ⚠ GLFW 源目录不存在" -ForegroundColor Yellow
    }
} else {
    Write-Host "  ✓ GLFW 已存在" -ForegroundColor Green
}

Write-Host "[4/5] 检查 FFmpeg..." -ForegroundColor Yellow

# 检查 FFmpeg
$FFmpegDst = Join-Path $DepsDir "ffmpeg_x64-windows"
if (Test-Path $FFmpegDst) {
    Write-Host "  ✓ FFmpeg 已存在" -ForegroundColor Green
} else {
    Write-Host "  ⚠ 警告: FFmpeg 未找到" -ForegroundColor Yellow
    Write-Host "    请手动将 FFmpeg 复制到: $FFmpegDst" -ForegroundColor Yellow
    Write-Host "    或使用 vcpkg 安装: vcpkg install ffmpeg:x64-windows" -ForegroundColor Yellow
}

Write-Host "[5/5] 验证部署..." -ForegroundColor Yellow

# 验证关键文件
$RequiredFiles = @(
    "ClipEngineSDK-Debug\lib\clipengine.lib",
    "ClipEngineSDK-Debug\include\clipengine\clipengine.h",
    "ClipEngineSDK-Debug\bin\webgpu_dawn.dll"
)

$AllGood = $true
foreach ($file in $RequiredFiles) {
    $FilePath = Join-Path $DepsDir $file
    if (-not (Test-Path $FilePath)) {
        Write-Host "  ✗ 缺少: $file" -ForegroundColor Red
        $AllGood = $false
    }
}

Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan

if ($AllGood) {
    Write-Host "✓ 部署成功！" -ForegroundColor Green
    Write-Host ""
    Write-Host "下一步操作：" -ForegroundColor Cyan
    Write-Host "  1. cd example" -ForegroundColor White
    Write-Host "  2. cmake -B build" -ForegroundColor White
    Write-Host "  3. cmake --build build --config Debug" -ForegroundColor White
    Write-Host ""
} else {
    Write-Host "✗ 部署未完成，请检查错误信息" -ForegroundColor Red
    exit 1
}

Write-Host "======================================" -ForegroundColor Cyan
