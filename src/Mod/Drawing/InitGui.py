# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Project                                   *
# *                                                                         *
# *   Drawing workbench GUI initialization                                  *
# *                                                                         *
# ***************************************************************************

"""Drawing workbench GUI initialization.

This module registers the Drawing workbench which uses C++ commands
defined in DrawingGui::CreateDrawingCommands().
"""


class DrawingWorkbench(Workbench):
    """Drawing workbench object"""
    
    MenuText = "Drawing"
    ToolTip = "2D Drawing workbench"
    
    def Initialize(self):
        import FreeCAD
        FreeCAD.Console.PrintMessage("Initializing Drawing workbench...\n")
        
        # Load C++ modules - this registers the C++ commands
        import DrawingGui
        import Drawing
        
        # Command list - these are registered by C++ code in DrawingGui
        self.cmdList = ["Drawing_Line", "Drawing_Circle"]
        
        # Create toolbar and menu
        self.appendToolbar("Drawing Tools", self.cmdList)
        self.appendMenu("Drawing", self.cmdList)
        
        FreeCAD.Console.PrintMessage("Drawing workbench initialized.\n")
    
    def Activated(self):
        import FreeCAD
        FreeCAD.Console.PrintMessage("Drawing workbench activated.\n")
    
    def Deactivated(self):
        pass
    
    def GetClassName(self):
        # Use Python workbench for simplicity
        return "Gui::PythonWorkbench"


Gui.addWorkbench(DrawingWorkbench())
