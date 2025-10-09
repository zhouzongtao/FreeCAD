# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Project                                   *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""
Drawing workbench GUI initialization with interactive drawing tools

This file provides a complete Drawing workbench with mouse-interactive
drawing commands for lines, circles, and rectangles.
"""

import FreeCAD
import FreeCADGui
import Part

class DrawingWorkbench(FreeCADGui.Workbench):
    """Drawing workbench class with interactive drawing tools"""
    
    def __init__(self):
        self.__class__.MenuText = "Drawing"
        self.__class__.ToolTip = "2D Drawing workbench with interactive tools"
    
    def Initialize(self):
        """Initialize the workbench"""
        # Import Drawing modules
        try:
            import Drawing
            import DrawingGui
            FreeCAD.Console.PrintMessage("Drawing modules loaded successfully\n")
        except ImportError as e:
            FreeCAD.Console.PrintError(f"Failed to load Drawing modules: {e}\n")
        
        # Create drawing commands
        self.createDrawingCommands()
        
        # Set up menus and toolbars
        drawingCommands = [
            "Drawing_Line",
            "Drawing_Circle", 
            "Drawing_Rectangle",
            "Separator",
            "Drawing_Info"
        ]
        
        self.appendMenu("Drawing", drawingCommands)
        self.appendToolbar("Drawing Tools", drawingCommands)
        
        FreeCAD.Console.PrintMessage("Drawing workbench initialized with interactive tools\n")
    
    def createDrawingCommands(self):
        """Create interactive drawing commands"""
        
        # Line drawing command with mouse interaction
        class DrawLineCommand:
            def __init__(self):
                self.points = []
                self.callback = None
            
            def GetResources(self):
                return {
                    'Pixmap': 'Part_Line',
                    'MenuText': 'Draw Line',
                    'ToolTip': 'Draw a line by clicking two points',
                    'Accel': 'L'
                }
            
            def Activated(self):
                import FreeCADGui
                from pivy import coin
                
                FreeCAD.Console.PrintMessage("Click two points to draw a line\n")
                self.points = []
                
                # Get the active 3D view
                view = FreeCADGui.activeDocument().activeView()
                
                # Set up mouse callback for point picking
                self.callback = view.addEventCallback("SoMouseButtonEvent", self.mouseCallback)
            
            def mouseCallback(self, event):
                import FreeCADGui
                from pivy import coin
                
                # Only handle left mouse button press
                if (event.getButton() == coin.SoMouseButtonEvent.BUTTON1 and 
                    event.getState() == coin.SoMouseButtonEvent.DOWN):
                    
                    # Get 3D point from mouse position
                    pos = event.getPosition()
                    view = FreeCADGui.activeDocument().activeView()
                    point = view.getPoint(pos[0], pos[1])
                    
                    self.points.append(point)
                    FreeCAD.Console.PrintMessage(f"Point {len(self.points)}: {point}\n")
                    
                    if len(self.points) == 2:
                        # Create the line
                        self.createLine()
                        # Remove callback and reset
                        view = FreeCADGui.activeDocument().activeView()
                        view.removeEventCallback("SoMouseButtonEvent", self.callback)
                        self.points = []
                        FreeCAD.Console.PrintMessage("Line created! Click 'Draw Line' again for another line.\n")
            
            def createLine(self):
                """Create a line object from two points"""
                try:
                    doc = FreeCAD.activeDocument()
                    if not doc:
                        doc = FreeCAD.newDocument()
                    
                    # Create line using Part module
                    import Part
                    line = Part.makeLine(self.points[0], self.points[1])
                    
                    # Add to document
                    obj = doc.addObject("Part::Feature", "Line")
                    obj.Shape = line
                    obj.Label = f"Line_{len([o for o in doc.Objects if 'Line' in o.Label]) + 1}"
                    
                    doc.recompute()
                    FreeCADGui.SendMsgToActiveView("ViewFit")
                    
                    FreeCAD.Console.PrintMessage(f"Line created from {self.points[0]} to {self.points[1]}\n")
                    
                except Exception as e:
                    FreeCAD.Console.PrintError(f"Failed to create line: {e}\n")
            
            def IsActive(self):
                return True
        
        # Circle drawing command
        class DrawCircleCommand:
            def __init__(self):
                self.points = []
                self.callback = None
            
            def GetResources(self):
                return {
                    'Pixmap': 'Part_Circle',
                    'MenuText': 'Draw Circle',
                    'ToolTip': 'Draw a circle by clicking center and edge point',
                    'Accel': 'C'
                }
            
            def Activated(self):
                import FreeCADGui
                from pivy import coin
                
                FreeCAD.Console.PrintMessage("Click center point, then edge point to draw a circle\n")
                self.points = []
                
                view = FreeCADGui.activeDocument().activeView()
                self.callback = view.addEventCallback("SoMouseButtonEvent", self.mouseCallback)
            
            def mouseCallback(self, event):
                import FreeCADGui
                from pivy import coin
                
                if (event.getButton() == coin.SoMouseButtonEvent.BUTTON1 and 
                    event.getState() == coin.SoMouseButtonEvent.DOWN):
                    
                    pos = event.getPosition()
                    view = FreeCADGui.activeDocument().activeView()
                    point = view.getPoint(pos[0], pos[1])
                    
                    self.points.append(point)
                    
                    if len(self.points) == 1:
                        FreeCAD.Console.PrintMessage(f"Center: {point}. Now click edge point.\n")
                    elif len(self.points) == 2:
                        self.createCircle()
                        view.removeEventCallback("SoMouseButtonEvent", self.callback)
                        self.points = []
                        FreeCAD.Console.PrintMessage("Circle created!\n")
            
            def createCircle(self):
                """Create a circle from center and edge points"""
                try:
                    doc = FreeCAD.activeDocument()
                    if not doc:
                        doc = FreeCAD.newDocument()
                    
                    center = self.points[0]
                    edge = self.points[1]
                    radius = center.distanceToPoint(edge)
                    
                    import Part
                    circle = Part.makeCircle(radius, center)
                    
                    obj = doc.addObject("Part::Feature", "Circle")
                    obj.Shape = circle
                    obj.Label = f"Circle_{len([o for o in doc.Objects if 'Circle' in o.Label]) + 1}"
                    
                    doc.recompute()
                    FreeCADGui.SendMsgToActiveView("ViewFit")
                    
                    FreeCAD.Console.PrintMessage(f"Circle created: center={center}, radius={radius:.2f}\n")
                    
                except Exception as e:
                    FreeCAD.Console.PrintError(f"Failed to create circle: {e}\n")
            
            def IsActive(self):
                return True
        
        # Rectangle drawing command
        class DrawRectangleCommand:
            def __init__(self):
                self.points = []
                self.callback = None
            
            def GetResources(self):
                return {
                    'Pixmap': 'Part_Box',
                    'MenuText': 'Draw Rectangle',
                    'ToolTip': 'Draw a rectangle by clicking two opposite corners',
                    'Accel': 'R'
                }
            
            def Activated(self):
                import FreeCADGui
                
                FreeCAD.Console.PrintMessage("Click two opposite corners to draw a rectangle\n")
                self.points = []
                
                view = FreeCADGui.activeDocument().activeView()
                self.callback = view.addEventCallback("SoMouseButtonEvent", self.mouseCallback)
            
            def mouseCallback(self, event):
                import FreeCADGui
                from pivy import coin
                
                if (event.getButton() == coin.SoMouseButtonEvent.BUTTON1 and 
                    event.getState() == coin.SoMouseButtonEvent.DOWN):
                    
                    pos = event.getPosition()
                    view = FreeCADGui.activeDocument().activeView()
                    point = view.getPoint(pos[0], pos[1])
                    
                    self.points.append(point)
                    
                    if len(self.points) == 1:
                        FreeCAD.Console.PrintMessage(f"First corner: {point}. Click opposite corner.\n")
                    elif len(self.points) == 2:
                        self.createRectangle()
                        view.removeEventCallback("SoMouseButtonEvent", self.callback)
                        self.points = []
                        FreeCAD.Console.PrintMessage("Rectangle created!\n")
            
            def createRectangle(self):
                """Create a rectangle from two points"""
                try:
                    doc = FreeCAD.activeDocument()
                    if not doc:
                        doc = FreeCAD.newDocument()
                    
                    p1 = self.points[0]
                    p2 = self.points[1]
                    
                    # Create rectangle as a wire
                    import Part
                    v1 = FreeCAD.Vector(p1.x, p1.y, 0)
                    v2 = FreeCAD.Vector(p2.x, p1.y, 0)
                    v3 = FreeCAD.Vector(p2.x, p2.y, 0)
                    v4 = FreeCAD.Vector(p1.x, p2.y, 0)
                    
                    edges = [
                        Part.makeLine(v1, v2),
                        Part.makeLine(v2, v3),
                        Part.makeLine(v3, v4),
                        Part.makeLine(v4, v1)
                    ]
                    
                    wire = Part.Wire(edges)
                    
                    obj = doc.addObject("Part::Feature", "Rectangle")
                    obj.Shape = wire
                    obj.Label = f"Rectangle_{len([o for o in doc.Objects if 'Rectangle' in o.Label]) + 1}"
                    
                    doc.recompute()
                    FreeCADGui.SendMsgToActiveView("ViewFit")
                    
                    FreeCAD.Console.PrintMessage(f"Rectangle created from {p1} to {p2}\n")
                    
                except Exception as e:
                    FreeCAD.Console.PrintError(f"Failed to create rectangle: {e}\n")
            
            def IsActive(self):
                return True
        
        # Info command
        class DrawingInfoCommand:
            def GetResources(self):
                return {
                    'Pixmap': 'Std_About',
                    'MenuText': 'Drawing Info',
                    'ToolTip': 'Show Drawing workbench information'
                }
            
            def Activated(self):
                import Drawing
                FreeCAD.Console.PrintMessage("=== Drawing Workbench ===\n")
                FreeCAD.Console.PrintMessage("Available functions: " + str([f for f in dir(Drawing) if not f.startswith('_')]) + "\n")
                FreeCAD.Console.PrintMessage("Interactive tools:\n")
                FreeCAD.Console.PrintMessage("- Draw Line: Click two points\n")
                FreeCAD.Console.PrintMessage("- Draw Circle: Click center, then edge\n")
                FreeCAD.Console.PrintMessage("- Draw Rectangle: Click two opposite corners\n")
            
            def IsActive(self):
                return True
        
        # Register all commands
        FreeCADGui.addCommand("Drawing_Line", DrawLineCommand())
        FreeCADGui.addCommand("Drawing_Circle", DrawCircleCommand())
        FreeCADGui.addCommand("Drawing_Rectangle", DrawRectangleCommand())
        FreeCADGui.addCommand("Drawing_Info", DrawingInfoCommand())
    
    def Activated(self):
        """Called when workbench is activated"""
        FreeCAD.Console.PrintMessage("Drawing workbench activated - Ready for interactive drawing!\n")
        FreeCAD.Console.PrintMessage("Use the toolbar buttons or menu to start drawing.\n")
    
    def Deactivated(self):
        """Called when workbench is deactivated"""
        FreeCAD.Console.PrintMessage("Drawing workbench deactivated\n")
    
    def GetClassName(self):
        """Return the workbench class name"""
        return "Gui::PythonWorkbench"

# Register the workbench
FreeCADGui.addWorkbench(DrawingWorkbench())