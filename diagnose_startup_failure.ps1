# 诊断 FreeCAD 启动失败
# Diagnose FreeCAD startup failure

Write-Host "=" -NoNewline; Write-Host ("=" * 69)
Write-Host "FreeCAD 启动失败诊断"
Write-Host "=" -NoNewline; Write-Host ("=" * 69)
Write-Host ""

$freecadBin = "E:\Repository\FreeCAD\FreeCAD\build\bin"

# 检查关键文件
Write-Host "步骤 1: 检查关键文件"
Write-Host ("-" * 70)

$files = @(
    "FreeCAD.exe",
    "FreeCADGui.dll",
    "FreeCADBase.dll",
    "FreeCADApp.dll",
    "osg161-osg.dll",
    "osg161-osgViewer.dll",
    "osg161-osgGA.dll",
    "osg161-osgDB.dll",
    "osg161-osgUtil.dll"
)

foreach ($file in $files) {
    $path = Join-Path $freecadBin $file
    if (Test-Path $path) {
        Write-Host "[OK] $file"
    } else {
        Write-Host "[MISSING] $file" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "步骤 2: 尝试启动 FreeCAD（控制台模式）"
Write-Host ("-" * 70)
Write-Host "正在启动 FreeCAD，请等待..."
Write-Host ""

# 尝试启动 FreeCAD 并捕获输出
$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = Join-Path $freecadBin "FreeCAD.exe"
$startInfo.Arguments = "--console --log-file startup.log"
$startInfo.WorkingDirectory = $freecadBin
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $false

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $startInfo

try {
    $process.Start() | Out-Null
    
    # 等待 5 秒
    Write-Host "等待 5 秒..."
    Start-Sleep -Seconds 5
    
    if (!$process.HasExited) {
        Write-Host "[INFO] FreeCAD 进程正在运行" -ForegroundColor Green
        Write-Host "进程 ID: $($process.Id)"
        
        # 终止进程
        Write-Host "终止进程..."
        $process.Kill()
        $process.WaitForExit()
        Write-Host "[OK] 进程已终止"
    } else {
        Write-Host "[ERROR] FreeCAD 进程已退出" -ForegroundColor Red
        Write-Host "退出代码: $($process.ExitCode)"
    }
    
    # 读取输出
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    
    if ($stdout) {
        Write-Host ""
        Write-Host "标准输出:"
        Write-Host $stdout
    }
    
    if ($stderr) {
        Write-Host ""
        Write-Host "标准错误:" -ForegroundColor Red
        Write-Host $stderr
    }
    
} catch {
    Write-Host "[ERROR] 启动失败: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "步骤 3: 检查日志文件"
Write-Host ("-" * 70)

$logFile = Join-Path $freecadBin "startup.log"
if (Test-Path $logFile) {
    Write-Host "[OK] 找到日志文件: startup.log"
    Write-Host ""
    Write-Host "最后 50 行日志:"
    Get-Content $logFile -Tail 50
} else {
    Write-Host "[INFO] 未找到日志文件"
}

Write-Host ""
Write-Host "=" -NoNewline; Write-Host ("=" * 69)
Write-Host "诊断完成"
Write-Host "=" -NoNewline; Write-Host ("=" * 69)
