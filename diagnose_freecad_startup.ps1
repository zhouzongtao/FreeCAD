# FreeCAD 启动失败诊断脚本
# FreeCAD Startup Failure Diagnosis Script

param(
    [string]$FreeCADPath = "E:\Repository\FreeCAD\FreeCAD\build\bin"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FreeCAD 启动失败诊断" -ForegroundColor Cyan
Write-Host "FreeCAD Startup Failure Diagnosis" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. 检查 FreeCAD 可执行文件
Write-Host "1. 检查 FreeCAD 可执行文件 / Checking FreeCAD executables..." -ForegroundColor Yellow
$freecadExe = Join-Path $FreeCADPath "FreeCAD.exe"
$freecadCmd = Join-Path $FreeCADPath "FreeCADCmd.exe"
$freecadGui = Join-Path $FreeCADPath "FreeCADGui.dll"

if (Test-Path $freecadExe) {
    $exeInfo = Get-Item $freecadExe
    Write-Host "  ✓ FreeCAD.exe 存在 / exists" -ForegroundColor Green
    Write-Host "    大小 / Size: $($exeInfo.Length) bytes" -ForegroundColor Gray
    Write-Host "    修改时间 / Modified: $($exeInfo.LastWriteTime)" -ForegroundColor Gray
} else {
    Write-Host "  ✗ FreeCAD.exe 不存在 / not found" -ForegroundColor Red
}

if (Test-Path $freecadGui) {
    $guiInfo = Get-Item $freecadGui
    Write-Host "  ✓ FreeCADGui.dll 存在 / exists" -ForegroundColor Green
    Write-Host "    大小 / Size: $($guiInfo.Length) bytes" -ForegroundColor Gray
    Write-Host "    修改时间 / Modified: $($guiInfo.LastWriteTime)" -ForegroundColor Gray
} else {
    Write-Host "  ✗ FreeCADGui.dll 不存在 / not found" -ForegroundColor Red
}
Write-Host ""

# 2. 检查关键 DLL 依赖
Write-Host "2. 检查关键 DLL 依赖 / Checking critical DLL dependencies..." -ForegroundColor Yellow
$criticalDlls = @(
    "FreeCADBase.dll",
    "FreeCADApp.dll",
    "FreeCADGui.dll",
    "python312.dll",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll"
)

$missingDlls = @()
foreach ($dll in $criticalDlls) {
    $dllPath = Join-Path $FreeCADPath $dll
    if (Test-Path $dllPath) {
        Write-Host "  ✓ $dll" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $dll (缺失 / Missing)" -ForegroundColor Red
        $missingDlls += $dll
    }
}
Write-Host ""

# 3. 检查 OSG DLL
Write-Host "3. 检查 OSG DLL / Checking OSG DLLs..." -ForegroundColor Yellow
$osgDlls = @(
    "osg161-osg.dll",
    "osg161-osgDB.dll",
    "osg161-osgUtil.dll",
    "osg161-osgViewer.dll",
    "osg161-osgGA.dll",
    "ot21-OpenThreads.dll"
)

$missingOsgDlls = @()
foreach ($dll in $osgDlls) {
    $dllPath = Join-Path $FreeCADPath $dll
    if (Test-Path $dllPath) {
        Write-Host "  ✓ $dll" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $dll (缺失 / Missing)" -ForegroundColor Red
        $missingOsgDlls += $dll
    }
}
Write-Host ""

# 4. 检查 OSG 插件目录
Write-Host "4. 检查 OSG 插件目录 / Checking OSG plugins directory..." -ForegroundColor Yellow
$osgPluginsPath = Join-Path $FreeCADPath "osgPlugins-3.6.5"
if (Test-Path $osgPluginsPath) {
    $pluginCount = (Get-ChildItem -Path $osgPluginsPath -Filter "*.dll" -ErrorAction SilentlyContinue).Count
    Write-Host "  ✓ osgPlugins-3.6.5 目录存在 / directory exists" -ForegroundColor Green
    Write-Host "    插件数量 / Plugin count: $pluginCount" -ForegroundColor Gray
} else {
    Write-Host "  ✗ osgPlugins-3.6.5 目录不存在 / directory not found" -ForegroundColor Red
}
Write-Host ""

# 5. 使用 Dependencies.exe 或 dumpbin 检查 FreeCADGui.dll 的依赖
Write-Host "5. 检查 FreeCADGui.dll 的导入依赖 / Checking FreeCADGui.dll imports..." -ForegroundColor Yellow
if (Test-Path $freecadGui) {
    try {
        # 尝试使用 dumpbin（如果可用）
        $dumpbin = "dumpbin"
        $dumpbinOutput = & $dumpbin /DEPENDENTS $freecadGui 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  FreeCADGui.dll 依赖的 DLL:" -ForegroundColor Gray
            $dumpbinOutput | Select-String "\.dll" | ForEach-Object {
                $line = $_.Line.Trim()
                if ($line -match "osg|OSG") {
                    Write-Host "    $line" -ForegroundColor Cyan
                }
            }
        } else {
            Write-Host "  信息: dumpbin 不可用，跳过详细依赖检查 / Info: dumpbin not available" -ForegroundColor Gray
        }
    } catch {
        Write-Host "  信息: 无法运行 dumpbin / Info: Cannot run dumpbin" -ForegroundColor Gray
    }
} else {
    Write-Host "  ✗ FreeCADGui.dll 不存在，无法检查 / not found, cannot check" -ForegroundColor Red
}
Write-Host ""

# 6. 尝试运行 FreeCADCmd 获取错误信息
Write-Host "6. 尝试运行 FreeCADCmd 获取错误信息 / Trying to run FreeCADCmd for error messages..." -ForegroundColor Yellow
if (Test-Path $freecadCmd) {
    Write-Host "  正在运行 FreeCADCmd.exe --version..." -ForegroundColor Gray
    try {
        $output = & $freecadCmd --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  ✓ FreeCADCmd 运行成功 / ran successfully" -ForegroundColor Green
            Write-Host "  输出 / Output:" -ForegroundColor Gray
            $output | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
        } else {
            Write-Host "  ✗ FreeCADCmd 运行失败 / failed to run" -ForegroundColor Red
            Write-Host "  错误输出 / Error output:" -ForegroundColor Red
            $output | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
        }
    } catch {
        Write-Host "  ✗ 无法运行 FreeCADCmd / Cannot run FreeCADCmd" -ForegroundColor Red
        Write-Host "  错误 / Error: $($_.Exception.Message)" -ForegroundColor Red
    }
} else {
    Write-Host "  ✗ FreeCADCmd.exe 不存在 / not found" -ForegroundColor Red
}
Write-Host ""

# 7. 检查事件查看器中的应用程序错误
Write-Host "7. 检查最近的应用程序错误日志 / Checking recent application error logs..." -ForegroundColor Yellow
try {
    $recentErrors = Get-EventLog -LogName Application -Source "Application Error" -Newest 5 -ErrorAction SilentlyContinue | 
        Where-Object { $_.Message -like "*FreeCAD*" }
    
    if ($recentErrors) {
        Write-Host "  找到 FreeCAD 相关错误 / Found FreeCAD-related errors:" -ForegroundColor Yellow
        foreach ($error in $recentErrors) {
            Write-Host "  时间 / Time: $($error.TimeGenerated)" -ForegroundColor Gray
            Write-Host "  消息 / Message: $($error.Message.Substring(0, [Math]::Min(200, $error.Message.Length)))..." -ForegroundColor Gray
            Write-Host ""
        }
    } else {
        Write-Host "  未找到最近的 FreeCAD 错误 / No recent FreeCAD errors found" -ForegroundColor Green
    }
} catch {
    Write-Host "  信息: 无法访问事件日志 / Info: Cannot access event log" -ForegroundColor Gray
}
Write-Host ""

# 8. 检查 Python 环境
Write-Host "8. 检查 Python 环境 / Checking Python environment..." -ForegroundColor Yellow
$pythonDll = Join-Path $FreeCADPath "python312.dll"
if (Test-Path $pythonDll) {
    Write-Host "  ✓ python312.dll 存在 / exists" -ForegroundColor Green
    
    # 检查 Python 库目录
    $pythonLibPath = Join-Path $FreeCADPath "Lib"
    if (Test-Path $pythonLibPath) {
        Write-Host "  ✓ Lib 目录存在 / directory exists" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Lib 目录不存在 / directory not found" -ForegroundColor Red
    }
} else {
    Write-Host "  ✗ python312.dll 不存在 / not found" -ForegroundColor Red
}
Write-Host ""

# 9. 生成诊断报告
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "诊断总结 / Diagnosis Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if ($missingDlls.Count -gt 0) {
    Write-Host "⚠ 缺失关键 DLL / Missing critical DLLs:" -ForegroundColor Red
    $missingDlls | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
}

if ($missingOsgDlls.Count -gt 0) {
    Write-Host "⚠ 缺失 OSG DLL / Missing OSG DLLs:" -ForegroundColor Red
    $missingOsgDlls | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
}

Write-Host ""
Write-Host "建议的解决步骤 / Recommended troubleshooting steps:" -ForegroundColor Yellow
Write-Host "1. 检查上述缺失的 DLL 文件 / Check missing DLL files above" -ForegroundColor Gray
Write-Host "2. 确保所有 OSG DLL 使用相同的运行时库 (/MD) / Ensure all OSG DLLs use same runtime (/MD)" -ForegroundColor Gray
Write-Host "3. 检查 FreeCADCmd.exe 的错误输出 / Check FreeCADCmd.exe error output" -ForegroundColor Gray
Write-Host "4. 使用 Dependencies.exe 工具检查 DLL 依赖链 / Use Dependencies.exe to check DLL dependency chain" -ForegroundColor Gray
Write-Host "5. 查看 Windows 事件查看器中的详细错误 / Check Windows Event Viewer for detailed errors" -ForegroundColor Gray
Write-Host ""

# 10. 创建测试脚本
Write-Host "10. 创建测试脚本 / Creating test script..." -ForegroundColor Yellow
$testScriptPath = Join-Path $FreeCADPath "test_freecad.bat"
$testScriptContent = @"
@echo off
echo ========================================
echo FreeCAD 启动测试 / FreeCAD Startup Test
echo ========================================
echo.

echo 测试 1: 运行 FreeCADCmd --version
echo Test 1: Running FreeCADCmd --version
echo.
FreeCADCmd.exe --version
if %ERRORLEVEL% NEQ 0 (
    echo 错误: FreeCADCmd 运行失败 / Error: FreeCADCmd failed
    echo 错误代码 / Error code: %ERRORLEVEL%
) else (
    echo 成功: FreeCADCmd 运行正常 / Success: FreeCADCmd runs normally
)
echo.

echo 测试 2: 运行 FreeCADCmd -c "print('Hello')"
echo Test 2: Running FreeCADCmd -c "print('Hello')"
echo.
FreeCADCmd.exe -c "print('Hello from FreeCAD')"
if %ERRORLEVEL% NEQ 0 (
    echo 错误: Python 执行失败 / Error: Python execution failed
    echo 错误代码 / Error code: %ERRORLEVEL%
) else (
    echo 成功: Python 执行正常 / Success: Python execution normal
)
echo.

echo 测试 3: 检查 OsgVerse 后端
echo Test 3: Checking OsgVerse backend
echo.
FreeCADCmd.exe -c "import FreeCADGui as Gui; print('Available backends:', Gui.listRenderBackends() if hasattr(Gui, 'listRenderBackends') else 'API not available')"
echo.

pause
"@

try {
    Set-Content -Path $testScriptPath -Value $testScriptContent -Encoding ASCII
    Write-Host "  ✓ 已创建测试脚本 / Created test script: test_freecad.bat" -ForegroundColor Green
    Write-Host "  运行此脚本以获取更多诊断信息 / Run this script for more diagnostic info" -ForegroundColor Gray
} catch {
    Write-Host "  ✗ 创建测试脚本失败 / Failed to create test script" -ForegroundColor Red
}

Write-Host ""
Write-Host "完成! / Done!" -ForegroundColor Green
