# 拷贝 OSG 和 OsgVerse 运行时文件到 FreeCAD 目录
# Copy OSG and OsgVerse runtime files to FreeCAD directory

param(
    [string]$OsgPath = "E:\Repository\OSGVerse\osg3.6.5Vs2022X64",
    [string]$OsgVersePath = "E:\Repository\OSGVerse",
    [string]$FreeCADPath = "E:\Repository\FreeCAD\FreeCAD\build\bin",
    [string]$Config = "Release"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "拷贝 OSG 和 OsgVerse 运行时文件" -ForegroundColor Cyan
Write-Host "Copy OSG and OsgVerse Runtime Files" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查源目录
Write-Host "检查源目录 / Checking source directories..." -ForegroundColor Yellow
if (-not (Test-Path $OsgPath)) {
    Write-Host "错误: OSG 路径不存在 / Error: OSG path not found: $OsgPath" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $OsgVersePath)) {
    Write-Host "错误: OsgVerse 路径不存在 / Error: OsgVerse path not found: $OsgVersePath" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $FreeCADPath)) {
    Write-Host "错误: FreeCAD 路径不存在 / Error: FreeCAD path not found: $FreeCADPath" -ForegroundColor Red
    exit 1
}

Write-Host "✓ 所有路径有效 / All paths valid" -ForegroundColor Green
Write-Host ""

# 1. 拷贝 OSG DLL 文件
Write-Host "1. 拷贝 OSG DLL 文件 / Copying OSG DLL files..." -ForegroundColor Yellow
$osgBinPath = Join-Path $OsgPath "bin"
$osgDlls = Get-ChildItem -Path $osgBinPath -Filter "*.dll" -ErrorAction SilentlyContinue

if ($osgDlls) {
    $copiedCount = 0
    foreach ($dll in $osgDlls) {
        $destPath = Join-Path $FreeCADPath $dll.Name
        try {
            Copy-Item -Path $dll.FullName -Destination $destPath -Force
            $copiedCount++
            Write-Host "  ✓ $($dll.Name)" -ForegroundColor Gray
        }
        catch {
            Write-Host "  ✗ 失败 / Failed: $($dll.Name) - $($_.Exception.Message)" -ForegroundColor Red
        }
    }
    Write-Host "  已拷贝 $copiedCount 个 DLL 文件 / Copied $copiedCount DLL files" -ForegroundColor Green
}
else {
    Write-Host "  警告: 未找到 OSG DLL 文件 / Warning: No OSG DLL files found" -ForegroundColor Yellow
}
Write-Host ""

# 2. 拷贝 OSG 插件
Write-Host "2. 拷贝 OSG 插件 / Copying OSG plugins..." -ForegroundColor Yellow
$osgPluginsPath = Join-Path $osgBinPath "osgPlugins-3.6.5"
if (Test-Path $osgPluginsPath) {
    $destPluginsPath = Join-Path $FreeCADPath "osgPlugins-3.6.5"
    
    # 创建插件目录
    if (-not (Test-Path $destPluginsPath)) {
        New-Item -ItemType Directory -Path $destPluginsPath -Force | Out-Null
        Write-Host "  创建目录 / Created directory: osgPlugins-3.6.5" -ForegroundColor Gray
    }
    
    # 拷贝所有插件 DLL
    $plugins = Get-ChildItem -Path $osgPluginsPath -Filter "*.dll" -ErrorAction SilentlyContinue
    if ($plugins) {
        $copiedCount = 0
        foreach ($plugin in $plugins) {
            $destPath = Join-Path $destPluginsPath $plugin.Name
            try {
                Copy-Item -Path $plugin.FullName -Destination $destPath -Force
                $copiedCount++
            }
            catch {
                Write-Host "  ✗ 失败 / Failed: $($plugin.Name)" -ForegroundColor Red
            }
        }
        Write-Host "  ✓ 已拷贝 $copiedCount 个插件 / Copied $copiedCount plugins" -ForegroundColor Green
    }
}
else {
    Write-Host "  警告: 未找到 OSG 插件目录 / Warning: OSG plugins directory not found" -ForegroundColor Yellow
}
Write-Host ""

# 3. 拷贝 OpenThreads DLL
Write-Host "3. 拷贝 OpenThreads DLL / Copying OpenThreads DLL..." -ForegroundColor Yellow
$openThreadsDll = Join-Path $osgBinPath "ot21-OpenThreads.dll"
if (Test-Path $openThreadsDll) {
    $destPath = Join-Path $FreeCADPath "ot21-OpenThreads.dll"
    try {
        Copy-Item -Path $openThreadsDll -Destination $destPath -Force
        Write-Host "  ✓ ot21-OpenThreads.dll" -ForegroundColor Green
    }
    catch {
        Write-Host "  ✗ 失败 / Failed: $($_.Exception.Message)" -ForegroundColor Red
    }
}
else {
    Write-Host "  警告: 未找到 OpenThreads DLL / Warning: OpenThreads DLL not found" -ForegroundColor Yellow
}
Write-Host ""

# 4. 拷贝 OsgVerse 库文件（如果有编译好的）
Write-Host "4. 检查 OsgVerse 库文件 / Checking OsgVerse library files..." -ForegroundColor Yellow
$osgVerseBuildPath = Join-Path $OsgVersePath "build"
if (Test-Path $osgVerseBuildPath) {
    $osgVerseBinPath = Join-Path $osgVerseBuildPath "bin\$Config"
    if (Test-Path $osgVerseBinPath) {
        $osgVerseDlls = Get-ChildItem -Path $osgVerseBinPath -Filter "*.dll" -ErrorAction SilentlyContinue
        if ($osgVerseDlls) {
            $copiedCount = 0
            foreach ($dll in $osgVerseDlls) {
                $destPath = Join-Path $FreeCADPath $dll.Name
                try {
                    Copy-Item -Path $dll.FullName -Destination $destPath -Force
                    $copiedCount++
                    Write-Host "  ✓ $($dll.Name)" -ForegroundColor Gray
                }
                catch {
                    Write-Host "  ✗ 失败 / Failed: $($dll.Name)" -ForegroundColor Red
                }
            }
            Write-Host "  已拷贝 $copiedCount 个 OsgVerse DLL 文件 / Copied $copiedCount OsgVerse DLL files" -ForegroundColor Green
        }
        else {
            Write-Host "  信息: 未找到 OsgVerse DLL 文件 / Info: No OsgVerse DLL files found" -ForegroundColor Gray
        }
    }
    else {
        Write-Host "  信息: OsgVerse 未编译或路径不存在 / Info: OsgVerse not built or path not found" -ForegroundColor Gray
    }
}
else {
    Write-Host "  信息: OsgVerse build 目录不存在 / Info: OsgVerse build directory not found" -ForegroundColor Gray
}
Write-Host ""

# 5. 设置环境变量（可选）
Write-Host "5. 环境变量建议 / Environment variable recommendations..." -ForegroundColor Yellow
Write-Host "  建议设置以下环境变量 / Recommend setting the following environment variables:" -ForegroundColor Gray
Write-Host "  OSG_FILE_PATH=$osgBinPath" -ForegroundColor Gray
Write-Host "  OSG_LIBRARY_PATH=$osgBinPath" -ForegroundColor Gray
Write-Host ""

# 6. 验证拷贝结果
Write-Host "6. 验证拷贝结果 / Verifying copy results..." -ForegroundColor Yellow
$requiredDlls = @(
    "osg161-osg.dll",
    "osg161-osgDB.dll",
    "osg161-osgUtil.dll",
    "osg161-osgViewer.dll",
    "osg161-osgGA.dll",
    "ot21-OpenThreads.dll"
)

$allPresent = $true
foreach ($dll in $requiredDlls) {
    $dllPath = Join-Path $FreeCADPath $dll
    if (Test-Path $dllPath) {
        Write-Host "  ✓ $dll" -ForegroundColor Green
    }
    else {
        Write-Host "  ✗ $dll (缺失 / Missing)" -ForegroundColor Red
        $allPresent = $false
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
if ($allPresent) {
    Write-Host "✓ 所有必需的 DLL 文件已就位" -ForegroundColor Green
    Write-Host "✓ All required DLL files are in place" -ForegroundColor Green
}
else {
    Write-Host "⚠ 部分 DLL 文件缺失，请检查" -ForegroundColor Yellow
    Write-Host "⚠ Some DLL files are missing, please check" -ForegroundColor Yellow
}
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 7. 创建运行脚本
Write-Host "7. 创建运行脚本 / Creating run script..." -ForegroundColor Yellow
$runScriptPath = Join-Path $FreeCADPath "run_freecad_with_osg.bat"
$runScriptContent = @"
@echo off
REM 设置 OSG 环境变量 / Set OSG environment variables
set OSG_FILE_PATH=$osgBinPath
set OSG_LIBRARY_PATH=$osgBinPath
set PATH=%PATH%;$osgBinPath

REM 启动 FreeCAD / Start FreeCAD
echo Starting FreeCAD with OSG support...
start FreeCAD.exe

REM 或者使用 FreeCADCmd.exe 进行命令行测试
REM Or use FreeCADCmd.exe for command line testing
REM FreeCADCmd.exe
"@

try {
    Set-Content -Path $runScriptPath -Value $runScriptContent -Encoding ASCII
    Write-Host "  ✓ 已创建运行脚本 / Created run script: run_freecad_with_osg.bat" -ForegroundColor Green
}
catch {
    Write-Host "  ✗ 创建运行脚本失败 / Failed to create run script: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host ""
Write-Host "完成! / Done!" -ForegroundColor Green
Write-Host ""
Write-Host "使用方法 / Usage:" -ForegroundColor Cyan
Write-Host "  1. 直接运行 FreeCAD.exe" -ForegroundColor Gray
Write-Host "  2. 或使用 run_freecad_with_osg.bat 启动（推荐）" -ForegroundColor Gray
Write-Host ""
