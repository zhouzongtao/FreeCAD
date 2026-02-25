@echo off
REM Phase 4.1: FreeCAD 启动测试
REM 直接启动 FreeCAD 并捕获控制台输出

echo ======================================================================
echo Phase 4.1: FreeCAD 启动测试
echo ======================================================================
echo.

cd /d E:\Repository\FreeCAD\FreeCAD\build\bin

echo 检查关键文件...
if exist FreeCAD.exe (
    echo [OK] FreeCAD.exe 存在
) else (
    echo [ERROR] FreeCAD.exe 不存在
    exit /b 1
)

if exist FreeCADGui.dll (
    echo [OK] FreeCADGui.dll 存在
) else (
    echo [ERROR] FreeCADGui.dll 不存在
    exit /b 1
)

echo.
echo 检查 OSG DLL...
if exist osg161-osg.dll (
    echo [OK] osg161-osg.dll 存在
) else (
    echo [WARNING] osg161-osg.dll 不存在
)

if exist osg161-osgViewer.dll (
    echo [OK] osg161-osgViewer.dll 存在
) else (
    echo [WARNING] osg161-osgViewer.dll 不存在
)

echo.
echo ======================================================================
echo 启动 FreeCAD（控制台模式）
echo ======================================================================
echo.
echo 注意：请查看控制台输出中的 OsgVerse 相关日志
echo 按 Ctrl+C 可以停止
echo.

REM 启动 FreeCAD 控制台模式
FreeCAD.exe --console

echo.
echo ======================================================================
echo 测试完成
echo ======================================================================
