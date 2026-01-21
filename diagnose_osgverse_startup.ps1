# OsgVerse 启动诊断脚本
# Diagnostic script for OsgVerse startup issues

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "OsgVerse 启动诊断" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$freecadPath = "E:\Repository\FreeCAD\FreeCAD\build\bin"
$freecadExe = Join-Path $freecadPath "FreeCAD.exe"
$logFile = Join-Path $PSScriptRoot "osgverse_startup_debug.log"

# 检查 FreeCAD.exe 是否存在
if (-not (Test-Path $freecadExe)) {
    Write-Host "错误: 找不到 FreeCAD.exe" -ForegroundColor Red
    Write-Host "路径: $freecadExe" -ForegroundColor Red
    exit 1
}

Write-Host "1. 检查 DLL 依赖..." -ForegroundColor Yellow
Write-Host ""

# 检查关键 DLL
$requiredDlls = @(
    "FreeCADGui.dll",
    "FreeCADBase.dll",
    "FreeCADApp.dll",
    "osg161-osg.dll",
    "osg161-osgViewer.dll",
    "osg161-osgGA.dll",
    "osg161-osgDB.dll",
    "ot21-OpenThreads.dll"
)

$missingDlls = @()
foreach ($dll in $requiredDlls) {
    $dllPath = Join-Path $freecadPath $dll
    if (Test-Path $dllPath) {
        Write-Host "  ✓ $dll" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $dll (缺失)" -ForegroundColor Red
        $missingDlls += $dll
    }
}

if ($missingDlls.Count -gt 0) {
    Write-Host ""
    Write-Host "警告: 缺少 $($missingDlls.Count) 个 DLL 文件" -ForegroundColor Red
}

Write-Host ""
Write-Host "2. 尝试启动 FreeCAD (带详细日志)..." -ForegroundColor Yellow
Write-Host ""

# 设置环境变量以获取更多调试信息
$env:OSG_NOTIFY_LEVEL = "DEBUG"
$env:FREECAD_DEBUG = "1"

# 创建启动脚本
$startScript = @"
`$ErrorActionPreference = 'Continue'
`$env:OSG_NOTIFY_LEVEL = 'DEBUG'
`$env:FREECAD_DEBUG = '1'

Write-Host "启动 FreeCAD..." -ForegroundColor Cyan
Write-Host "工作目录: $freecadPath" -ForegroundColor Gray
Write-Host "日志文件: $logFile" -ForegroundColor Gray
Write-Host ""

try {
    # 启动 FreeCAD 并捕获输出
    `$process = Start-Process -FilePath '$freecadExe' ``
        -ArgumentList '--console', '--log-file', '$logFile' ``
        -WorkingDirectory '$freecadPath' ``
        -PassThru ``
        -NoNewWindow ``
        -RedirectStandardOutput '$logFile.stdout' ``
        -RedirectStandardError '$logFile.stderr'
    
    Write-Host "进程已启动 (PID: `$(`$process.Id))" -ForegroundColor Green
    Write-Host "等待 5 秒..." -ForegroundColor Yellow
    
    Start-Sleep -Seconds 5
    
    if (`$process.HasExited) {
        Write-Host ""
        Write-Host "进程已退出 (退出码: `$(`$process.ExitCode))" -ForegroundColor Red
        
        # 读取日志
        if (Test-Path '$logFile.stderr') {
            Write-Host ""
            Write-Host "=== 错误输出 ===" -ForegroundColor Red
            Get-Content '$logFile.stderr' | Select-Object -Last 50
        }
        
        if (Test-Path '$logFile.stdout') {
            Write-Host ""
            Write-Host "=== 标准输出 ===" -ForegroundColor Yellow
            Get-Content '$logFile.stdout' | Select-Object -Last 50
        }
    } else {
        Write-Host ""
        Write-Host "进程仍在运行" -ForegroundColor Green
        Write-Host "请手动测试 FreeCAD 功能" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "按任意键停止进程..." -ForegroundColor Yellow
        `$null = `$Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
        
        Write-Host "停止进程..." -ForegroundColor Yellow
        Stop-Process -Id `$process.Id -Force
    }
} catch {
    Write-Host ""
    Write-Host "启动失败: `$_" -ForegroundColor Red
    Write-Host ""
    Write-Host "异常详情:" -ForegroundColor Red
    Write-Host `$_.Exception.Message -ForegroundColor Red
    Write-Host ""
    Write-Host `$_.ScriptStackTrace -ForegroundColor Gray
}
"@

# 执行启动脚本
Invoke-Expression $startScript

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "诊断完成" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查日志文件
if (Test-Path $logFile) {
    Write-Host "日志文件已创建: $logFile" -ForegroundColor Green
    Write-Host ""
    Write-Host "=== 最后 30 行日志 ===" -ForegroundColor Yellow
    Get-Content $logFile -Tail 30
}

if (Test-Path "$logFile.stderr") {
    Write-Host ""
    Write-Host "=== 错误日志 ===" -ForegroundColor Red
    Get-Content "$logFile.stderr" -Tail 30
}
