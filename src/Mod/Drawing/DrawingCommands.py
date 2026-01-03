# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Project                                   *
# *                                                                         *
# *   Drawing workbench commands                                            *
# *                                                                         *
# ***************************************************************************

"""Drawing workbench commands."""

import FreeCAD
import FreeCADGui


class DrawLineCommand:
    """Command to draw a line by clicking two points."""
    
    def __init__(self):
        self.points = []
        self.call = None
        self.view = None
        self.viewer = None
    
    def GetResources(self):
        return {
            'MenuText': 'Line',
            'ToolTip': 'Draw a line by clicking two points',
            'Accel': 'L'
        }
    
    def Activated(self):
        if not FreeCAD.ActiveDocument:
            FreeCAD.newDocument("Drawing")
        
        FreeCAD.Console.PrintMessage("=== Draw Line ===\n")
        FreeCAD.Console.PrintMessage("Click first point (ESC to cancel)...\n")
        
        self.points = []
        self.view = FreeCADGui.ActiveDocument.ActiveView
        
        # Get the viewer and enable event redirection to bypass navigation
        self.viewer = self.view.getViewer()
        self.viewer.setRedirectToSceneGraph(True)
        
        self.call = self.view.addEventCallback("SoEvent", self.action)
    
    def action(self, arg):
        """Handle mouse and keyboard events."""
        # Handle ESC key to cancel
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
            return
        
        # Handle mouse click
        if arg["Type"] == "SoMouseButtonEvent":
            if arg["State"] == "DOWN" and arg["Button"] == "BUTTON1":
                # Get 3D point from click position
                pos = arg["Position"]
                point = self.view.getPoint(pos[0], pos[1])
                point.z = 0  # Force 2D
                
                self.points.append(point)
                
                if len(self.points) == 1:
                    FreeCAD.Console.PrintMessage(f"First point: ({point.x:.2f}, {point.y:.2f})\n")
                    FreeCAD.Console.PrintMessage("Click second point...\n")
                elif len(self.points) == 2:
                    FreeCAD.Console.PrintMessage(f"Second point: ({point.x:.2f}, {point.y:.2f})\n")
                    self.createLine()
                    self.finish()
    
    def createLine(self):
        import Part
        
        p1, p2 = self.points
        line_shape = Part.makeLine(p1, p2)
        
        doc = FreeCAD.ActiveDocument
        obj = doc.addObject("Part::Feature", "Line")
        obj.Shape = line_shape
        obj.ViewObject.LineColor = (0.0, 0.0, 0.0)
        obj.ViewObject.LineWidth = 2.0
        
        doc.recompute()
        
        length = p1.distanceToPoint(p2)
        FreeCAD.Console.PrintMessage(f"Line created: length = {length:.2f} mm\n")
    
    def finish(self):
        # Restore normal navigation
        if self.viewer:
            self.viewer.setRedirectToSceneGraph(False)
        if self.call and self.view:
            self.view.removeEventCallback("SoEvent", self.call)
        self.call = None
        self.points = []
        self.viewer = None
        FreeCAD.Console.PrintMessage("Line command finished.\n")
    
    def IsActive(self):
        return True


class PlaceholderCommand:
    """Placeholder command for unimplemented features."""
    
    def __init__(self, name):
        self.name = name
    
    def GetResources(self):
        return {
            'MenuText': self.name.replace('Drawing_', ''),
            'ToolTip': f'{self.name} (not yet implemented)'
        }
    
    def Activated(self):
        FreeCAD.Console.PrintMessage(f"{self.name} is not yet implemented.\n")
    
    def IsActive(self):
        return True


def registerCommands():
    """Register all Drawing commands."""
    FreeCADGui.addCommand("Drawing_Line", DrawLineCommand())
    
    for cmd_name in ["Drawing_Circle", "Drawing_Rectangle", "Drawing_Polygon",
                     "Drawing_Text", "Drawing_Dimension", "Drawing_Move",
                     "Drawing_Copy", "Drawing_Rotate", "Drawing_Scale",
                     "Drawing_Trim", "Drawing_Extend"]:
        FreeCADGui.addCommand(cmd_name, PlaceholderCommand(cmd_name))
