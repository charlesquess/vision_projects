# defect_detection 一键编译脚本
# 用法: .\build.ps1
# 或者: powershell -ExecutionPolicy Bypass -File build.ps1

param(
    [string]$Config = "Release",         # 编译配置: Release / Debug
    [string]$OpenCVDir = "C:\Opencv2\opencv\build"  # OpenCV 安装路径
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir    = Join-Path $ProjectRoot "build"

# ── 1. 检查 VS 2019 BuildTools 环境 ──────────────────────────
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$opencv_bin = "$OpenCVDir\x64\vc16\bin"
if (-not (Test-Path $vcvars)) {
    Write-Error "找不到 vcvars64.bat，请确认 VS 2019 BuildTools 已安装。"
    exit 1
}

# ── 2. 检查 cmake ───────────────────────────────────────────
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmake)) {
    Write-Error "找不到 cmake"
    exit 1
}

Write-Host "=== 编译: defect_detection ($Config) ===" -ForegroundColor Cyan
Write-Host "  项目目录 : $ProjectRoot"
Write-Host "  构建目录 : $BuildDir"
Write-Host "  OpenCV   : $OpenCVDir"
Write-Host ""

# ── 3. CMake 配置 ──────────────────────────────────────────
& cmd.exe /c "`"$vcvars`" && `"$cmake`" -B `"$BuildDir`" -S `"$ProjectRoot`" -G `"Visual Studio 16 2019`" -A x64 -DOpenCV_DIR=`"$OpenCVDir`" -DCMAKE_BUILD_TYPE=$Config"
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake 配置失败"
    exit 1
}

# ── 4. 编译 ────────────────────────────────────────────────
& cmd.exe /c "`"$vcvars`" && `"$cmake`" --build `"$BuildDir`" --config $Config"
if ($LASTEXITCODE -ne 0) {
    Write-Error "编译失败"
    exit 1
}

Write-Host ""
Write-Host "=== 编译成功 ===" -ForegroundColor Green
Write-Host "  输出: $BuildDir\$Config\defect_detection.exe"

# 自动复制 OpenCV DLL 到 exe 目录
$opencv_dlls = Get-ChildItem -Path "$opencv_bin" -Filter "opencv_world*.dll" -ErrorAction SilentlyContinue
foreach ($dll in $opencv_dlls) {
    Copy-Item -Path $dll.FullName -Destination "$BuildDir\$Config\" -Force
    Write-Host "  已复制 DLL: $($dll.Name)" -ForegroundColor Yellow
}
