@echo off
echo Testing Manual RenderManager Initialization
echo ============================================
echo.

set FREECAD_BIN=E:\Repository\FreeCAD\FreeCAD\build\bin
set PYTHON_SCRIPT=%~dp0test_manual_init.py

echo Running FreeCAD with test script...
echo.

"%FREECAD_BIN%\FreeCADCmd.exe" -c "%PYTHON_SCRIPT%"

echo.
echo Test complete. Press any key to exit...
pause > nul
