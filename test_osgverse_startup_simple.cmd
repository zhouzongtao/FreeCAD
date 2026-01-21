@echo off
echo ========================================
echo OsgVerse 启动测试
echo ========================================
echo.

cd /d E:\Repository\FreeCAD\FreeCAD\build\bin

echo 设置调试环境变量...
set OSG_NOTIFY_LEVEL=DEBUG
set FREECAD_DEBUG=1
set OSG_FILE_PATH=E:\Repository\OSGVerse\osg3.6.5Vs2022X64\bin

echo.
echo 启动 FreeCAD...
echo 如果程序崩溃，请记录错误信息
echo.

FreeCAD.exe --console 2>&1 | tee osgverse_startup.log

echo.
echo 程序已退出
pause
