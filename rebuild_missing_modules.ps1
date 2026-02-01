# 重新编译缺失的GUI模块
# 使用单线程编译避免内存不足

Write-Host "=== 检查缺失的GUI模块 ===" -ForegroundColor Cyan

$missingModules = @()
$modules = @(
    @{Name="SketcherGui"; Path="build/Mod/Sketcher/SketcherGui.pyd"},
    @{Name="DraftGui"; Path="build/Mod/Draft/DraftGui.pyd"},  
    @{Name="TechDrawGui"; Path="build/Mod/TechDraw/TechDrawGui.pyd"},
    @{Name="PartDesignGui"; Path="build/Mod/PartDesign/PartDesignGui.pyd"},
    @{Name="FemGui"; Path="build/Mod/Fem/FemGui.pyd"}
)

foreach ($mod in $modules) {
    if (Test-Path $mod.Path) {
        $info = Get-Item $mod.Path
        Write-Host "✓ $($mod.Name) 已存在 ($([math]::Round($info.Length/1MB,2)) MB)" -ForegroundColor Green
    } else {
        Write-Host "✗ $($mod.Name) 缺失" -ForegroundColor Red
        $missingModules += $mod.Name
    }
}

if ($missingModules.Count -eq 0) {
    Write-Host "`n所有模块都已编译！" -ForegroundColor Green
    exit 0
}

Write-Host "`n=== 需要重新编译的模块: $($missingModules -join ', ') ===" -ForegroundColor Yellow
Write-Host "使用单线程编译避免内存不足..." -ForegroundColor Yellow

# 清理预编译头文件缓存
Write-Host "`n清理预编译头文件..." -ForegroundColor Cyan
Get-ChildItem -Path "build" -Recurse -Filter "cmake_pch.pch" -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue

# 单线程编译每个缺失的模块
foreach ($modName in $missingModules) {
    Write-Host "`n=== 编译 $modName ===" -ForegroundColor Cyan
    
    $target = $modName
    if ($modName -eq "DraftGui") {
        # Draft模块比较特殊，可能需要编译Draft
        $target = "Draft"
    }
    
    Write-Host "cmake --build build --config Release --target $target -j 1" -ForegroundColor Gray
    cmake --build build --config Release --target $target -j 1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ $modName 编译失败" -ForegroundColor Red
    } else {
        Write-Host "✓ $modName 编译成功" -ForegroundColor Green
    }
}

Write-Host "`n=== 编译完成，检查结果 ===" -ForegroundColor Cyan
foreach ($mod in $modules) {
    if (Test-Path $mod.Path) {
        $info = Get-Item $mod.Path
        Write-Host "✓ $($mod.Name) : $([math]::Round($info.Length/1MB,2)) MB" -ForegroundColor Green
    } else {
        Write-Host "✗ $($mod.Name) : 仍然缺失" -ForegroundColor Red
    }
}
