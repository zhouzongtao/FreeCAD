# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Project                                   *
# *                                                                         *
# *   Drawing workbench GUI initialization                                  *
# *                                                                         *
# ***************************************************************************

"""Drawing workbench GUI initialization."""


class DrawingWorkbench(Workbench):
    """Drawing workbench object"""
    
    MenuText = "Drawing"
    ToolTip = "2D Drawing workbench"
    
    def Initialize(self):
        import FreeCAD
        import FreeCADGui
        
        FreeCAD.Console.PrintMessage("Initializing Drawing workbench...\n")
        
        # Load C++ modules
        import DrawingGui
        import Drawing
        
        # Register commands
        import DrawingCommands
        DrawingCommands.registerCommands()
        
        # Create toolbar and menu
        self.cmdList = ["Drawing_Line", "Drawing_Circle", "Drawing_Rectangle", 
                        "Drawing_Polygon", "Drawing_Text", "Drawing_Dimension"]
        self.modifyList = ["Drawing_Move", "Drawing_Copy", "Drawing_Rotate",
                           "Drawing_Scale", "Drawing_Trim", "Drawing_Extend"]
        
        self.appendToolbar("Drawing Creation", self.cmdList)
        self.appendToolbar("Drawing Modification", self.modifyList)
        self.appendMenu("Drawing", self.cmdList + self.modifyList)
        
        FreeCAD.Console.PrintMessage("Drawing workbench initialized.\n")
    
    def Activated(self):
        import FreeCAD
        FreeCAD.Console.PrintMessage("Drawing workbench activated.\n")
    
    def Deactivated(self):
        pass
    
    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(DrawingWorkbench())
