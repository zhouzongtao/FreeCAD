@echo off
echo ========================================
echo FreeCAD 启动并验证菜单
echo ========================================
echo.

echo [1] 检查关键模块文件...
echo.

set MISSING=0

if exist "build\Mod\Part\PartGui.pyd" (
    echo [OK] PartGui.pyd
) else (
    echo [MISSING] PartGui.pyd
    set MISSING=1
)

if exist "build\Mod\PartDesign\PartDesignGui.pyd" (
    echo [OK] PartDesignGui.pyd
) else (
    echo [MISSING] PartDesignGui.pyd
    set MISSING=1
)

if exist "build\Mod\Sketcher\SketcherGui.pyd" (
    echo [OK] SketcherGui.pyd
) else (
    echo [MISSING] SketcherGui.pyd
    set MISSING=1
)

if exist "build\Mod\TechDraw\TechDrawGui.pyd" (
    echo [OK] TechDrawGui.pyd
) else (
    echo [MISSING] TechDrawGui.pyd
    set MISSING=1
)

if exist "build\Mod\Fem\FemGui.pyd" (
    echo [OK] FemGui.pyd
) else (
    echo [MISSING] FemGui.pyd
    set MISSING=1
)

echo.

if %MISSING%==1 (
    echo [警告] 有模块文件缺失！
    echo 请先运行: powershell -ExecutionPolicy Bypass -File rebuild_missing_modules.ps1
    echo.
    pause
    exit /b 1
)

echo [2] 所有关键模块文件存在
echo.

echo [3] 启动FreeCAD...
echo.

start "" "build\bin\FreeCAD.exe"

echo.
echo FreeCAD已启动！
echo.
echo 验证步骤:
echo 1. 检查工作台列表 (右上角下拉菜单)
echo 2. 切换到Part工作台，检查菜单栏
echo 3. 切换到PartDesign工作台，检查菜单栏
echo 4. 在Python控制台运行: exec(open('verify_menus.py').read())
echo.
echo 如果菜单仍然缺失，请查看 菜单恢复_完成报告.md
echo.
pause
