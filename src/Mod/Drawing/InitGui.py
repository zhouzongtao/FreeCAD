# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Project                                   *
# *                                                                         *
# *   Drawing workbench GUI initialization (Skia-based)                    *
# *                                                                         *
# ***************************************************************************

"""Drawing workbench GUI initialization.

This module registers the Drawing workbench which uses Skia for 2D rendering.
"""

import FreeCAD as App
import FreeCADGui as Gui


class DrawingWorkbench(Gui.Workbench):
    """Drawing workbench with Skia-based 2D canvas"""
    
    MenuText = "Drawing"
    ToolTip = "2D Drawing workbench (Skia)"
    
    def Initialize(self):
        import FreeCAD
        FreeCAD.Console.PrintMessage("Initializing Drawing workbench (Skia)...\n")
        
        # Load C++ GUI module - this registers the C++ commands
        import DrawingGui
        
        # Command list - registered by C++ code in DrawingGui
        self.canvasTools = [
            "Drawing_NewCanvas",      # Platform-optimal GPU (Metal on macOS)
            "Drawing_NewCanvasCPU",   # CPU software rendering
        ]
        
        self.drawingTools = [
            "Drawing_Line", 
            "Drawing_Circle",
            "Drawing_Rectangle",
        ]
        
        self.exportTools = [
            "Drawing_ExportSVG",
        ]
        
        # Create toolbar and menu
        self.appendToolbar("Canvas", self.canvasTools)
        self.appendToolbar("Drawing Tools", self.drawingTools)
        self.appendToolbar("Export", self.exportTools)
        
        self.appendMenu("Drawing", self.canvasTools + ["Separator"] + self.drawingTools + ["Separator"] + self.exportTools)
        
        FreeCAD.Console.PrintMessage("Drawing workbench (Skia) initialized.\n")
    
    def Activated(self):
        import FreeCAD
        FreeCAD.Console.PrintMessage("Drawing workbench activated.\n")
    
    def Deactivated(self):
        pass
    
    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(DrawingWorkbench())
