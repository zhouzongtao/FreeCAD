@echo off
echo Testing FreeCAD startup...
echo.

REM Start FreeCAD GUI mode
start "" "build\bin\FreeCAD.exe"

echo FreeCAD started in GUI mode
echo Please check if it starts successfully
echo.
echo If it starts, run this in Python console:
echo   exec(open('diagnose_startup.py', encoding='utf-8').read())
echo.
pause
