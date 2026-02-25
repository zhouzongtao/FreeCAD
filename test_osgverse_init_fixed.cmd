@echo off
echo ========================================
echo Testing OsgVerse Initialization Fix
echo ========================================
echo.

set FREECAD_BIN=E:\Repository\FreeCAD\FreeCAD\build\bin

echo Starting FreeCAD with console output...
echo.

"%FREECAD_BIN%\FreeCADCmd.exe" -c "import FreeCADGui; print('=== RenderManager Status ==='); print('Current Backend:', FreeCADGui.getCurrentRenderBackend()); print('Renderer Info:', FreeCADGui.getRendererInfo()); print('OsgVerse Available:', FreeCADGui.isRenderBackendAvailable(2)); print('=== Test Complete ===')"

echo.
echo ========================================
echo Test Complete
echo ========================================
pause
