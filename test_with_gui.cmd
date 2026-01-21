@echo off
echo Starting FreeCAD GUI to check console output...
echo Check the Report View for RenderManager messages
echo.

start "" "E:\Repository\FreeCAD\FreeCAD\build\bin\FreeCAD.exe"

echo.
echo FreeCAD GUI started. Check the Report View for:
echo   - "Application: Initializing RenderManager Python bindings..."
echo   - "RenderManager Python bindings initialized"
echo   - "Application: Initializing RenderManager..."
echo.
pause
