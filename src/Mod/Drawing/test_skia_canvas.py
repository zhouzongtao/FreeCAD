#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Test script for Skia-based Drawing canvas.

Run this script from FreeCAD GUI:
    exec(open('/path/to/test_skia_canvas.py').read())
"""

import FreeCAD
import FreeCADGui

def test_skia_canvas():
    """Test the Skia canvas functionality."""
    
    # Import DrawingGui module
    try:
        import DrawingGui
        FreeCAD.Console.PrintMessage("DrawingGui module loaded successfully\n")
    except ImportError as e:
        FreeCAD.Console.PrintError(f"Failed to load DrawingGui: {e}\n")
        return False
    
    # Create a new document if needed
    if not FreeCAD.ActiveDocument:
        FreeCAD.newDocument("DrawingTest")
    
    # Execute the New Canvas command
    try:
        FreeCADGui.runCommand('Drawing_NewCanvas')
        FreeCAD.Console.PrintMessage("Drawing canvas created successfully!\n")
        FreeCAD.Console.PrintMessage("\nUsage:\n")
        FreeCAD.Console.PrintMessage("  - Press 'L' or click Line button to draw lines\n")
        FreeCAD.Console.PrintMessage("  - Press 'C' or click Circle button to draw circles\n")
        FreeCAD.Console.PrintMessage("  - Press 'R' or click Rectangle button to draw rectangles\n")
        FreeCAD.Console.PrintMessage("  - Use mouse wheel to zoom\n")
        FreeCAD.Console.PrintMessage("  - Middle mouse button to pan\n")
        FreeCAD.Console.PrintMessage("  - Press 'Escape' to cancel current drawing\n")
        return True
    except Exception as e:
        FreeCAD.Console.PrintError(f"Failed to create canvas: {e}\n")
        return False

if __name__ == "__main__":
    test_skia_canvas()
