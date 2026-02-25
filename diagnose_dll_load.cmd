@echo off
echo ========================================
echo Diagnosing FreeCADGui DLL Load Failure
echo ========================================
echo.

set FREECAD_BIN=E:\Repository\FreeCAD\FreeCAD\build\bin

echo Testing with minimal import...
echo.

REM 设置环境变量以获取更多调试信息
set PYTHONVERBOSE=1

echo Test 1: Import FreeCAD only
"%FREECAD_BIN%\FreeCADCmd.exe" -c "import FreeCAD; print('FreeCAD imported successfully')"

echo.
echo Test 2: Try to import FreeCADGui
"%FREECAD_BIN%\FreeCADCmd.exe" -c "try: import FreeCADGui; print('FreeCADGui imported successfully'); except Exception as e: print('Failed:', e); import traceback; traceback.print_exc()"

echo.
echo ========================================
echo Diagnosis Complete
echo ========================================
pause
