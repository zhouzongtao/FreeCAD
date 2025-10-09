# Simple Drawing workbench for direct execution in FreeCAD

import FreeCAD
import FreeCADGui
import Part

# Global drawing state
class DrawingState:
    active_command = None
    view = None
    callback = None
    
    @classmethod
    def start_drawing(cls, command, instruction):
        if not FreeCAD.activeDocument():
            FreeCAD.newDocument()
        
        cls.view = FreeCADGui.activeDocument().activeView()
        if not cls.view:
            return False
        
        # Set crosshair cursor
        try:
            from PySide6 import QtCore, QtGui
            cls.view.setCursor(QtGui.QCursor(QtCore.Qt.CrossCursor))
        except:
            pass
        
        cls.active_command = command
        command.points = []
        
        # Add mouse callback - this is the key fix!
        cls.callback = cls.view.addEventCallback("SoMouseButtonEvent", cls.handle_mouse)
        
        FreeCAD.Console.PrintMessage("DRAWING MODE: " + instruction + "\n")
        FreeCAD.Console.PrintMessage("Left click to place points, Right click to cancel\n")
        return True
    
    @classmethod
    def stop_drawing(cls):
        if cls.view and cls.callback:
            try:
                cls.view.removeEventCallback("SoMouseButtonEvent", cls.callback)
            except:
                pass
        
        # Restore cursor
        try:
            from PySide6 import QtCore, QtGui
            if cls.view:
                cls.view.setCursor(QtGui.QCursor(QtCore.Qt.ArrowCursor))
        except:
            pass
        
        cls.active_command = None
        cls.callback = None
        FreeCAD.Console.PrintMessage("Exited drawing mode\n")
    
    @classmethod
    def handle_mouse(cls, event):
        from pivy import coin
        
        if not cls.active_command:
            return
        
        # IMPORTANT: Mark event as handled to prevent navigation
        event.setHandled()
        
        # Left click to add point
        if (event.getButton() == coin.SoMouseButtonEvent.BUTTON1 and 
            event.getState() == coin.SoMouseButtonEvent.DOWN):
            
            pos = event.getPosition()
            try:
                point = cls.view.getPoint(pos[0], pos[1])
                point.z = 0  # Force to XY plane
                cls.active_command.add_point(point)
            except Exception as e:
                FreeCAD.Console.PrintError("Error: " + str(e) + "\n")
        
        # Right click to cancel
        elif (event.getButton() == coin.SoMouseButtonEvent.BUTTON3 and 
              event.getState() == coin.SoMouseButtonEvent.DOWN):
            cls.stop_drawing()

# Line command
class DrawLineCommand:
    def GetResources(self):
        return {
            'Pixmap': 'Part_Line',
            'MenuText': 'Draw Line',
            'ToolTip': 'Draw line by clicking two points'
        }
    
    def Activated(self):
        DrawingState.start_drawing(self, "Click start point for line")
    
    def add_point(self, point):
        self.points.append(point)
        FreeCAD.Console.PrintMessage("Point {}: ({:.2f}, {:.2f})\n".format(
            len(self.points), point.x, point.y))
        
        if len(self.points) == 1:
            FreeCAD.Console.PrintMessage("Click end point\n")
        elif len(self.points) == 2:
            self.create_line()
            DrawingState.stop_drawing()
    
    def create_line(self):
        try:
            doc = FreeCAD.activeDocument()
            line = Part.makeLine(self.points[0], self.points[1])
            
            obj = doc.addObject("Part::Feature", "Line")
            obj.Shape = line
            obj.Label = "Line_" + str(len([o for o in doc.Objects if 'Line' in o.Label]) + 1)
            
            doc.recompute()
            
            length = self.points[0].distanceToPoint(self.points[1])
            FreeCAD.Console.PrintMessage("SUCCESS: Line created! Length: {:.2f} mm\n".format(length))
            
        except Exception as e:
            FreeCAD.Console.PrintError("Error: " + str(e) + "\n")
    
    def IsActive(self):
        return True

# Circle command
class DrawCircleCommand:
    def GetResources(self):
        return {
            'Pixmap': 'Part_Circle',
            'MenuText': 'Draw Circle',
            'ToolTip': 'Draw circle by clicking center and edge'
        }
    
    def Activated(self):
        DrawingState.start_drawing(self, "Click center point for circle")
    
    def add_point(self, point):
        self.points.append(point)
        
        if len(self.points) == 1:
            FreeCAD.Console.PrintMessage("Center: ({:.2f}, {:.2f})\n".format(point.x, point.y))
            FreeCAD.Console.PrintMessage("Click edge point\n")
        elif len(self.points) == 2:
            self.create_circle()
            DrawingState.stop_drawing()
    
    def create_circle(self):
        try:
            doc = FreeCAD.activeDocument()
            
            center = self.points[0]
            edge = self.points[1]
            radius = center.distanceToPoint(edge)
            
            circle = Part.makeCircle(radius, center)
            
            obj = doc.addObject("Part::Feature", "Circle")
            obj.Shape = circle
            obj.Label = "Circle_" + str(len([o for o in doc.Objects if 'Circle' in o.Label]) + 1)
            
            doc.recompute()
            
            FreeCAD.Console.PrintMessage("SUCCESS: Circle created! Radius: {:.2f} mm\n".format(radius))
            
        except Exception as e:
            FreeCAD.Console.PrintError("Error: " + str(e) + "\n")
    
    def IsActive(self):
        return True

# Cancel command
class CancelDrawingCommand:
    def GetResources(self):
        return {
            'Pixmap': 'Process-stop',
            'MenuText': 'Cancel',
            'ToolTip': 'Cancel drawing operation'
        }
    
    def Activated(self):
        DrawingState.stop_drawing()
    
    def IsActive(self):
        return True

# Register commands
FreeCADGui.addCommand("Drawing_Line", DrawLineCommand())
FreeCADGui.addCommand("Drawing_Circle", DrawCircleCommand())
FreeCADGui.addCommand("Drawing_Cancel", CancelDrawingCommand())

print("Drawing commands registered!")
print("Now you can:")
print("1. Type: FreeCADGui.runCommand('Drawing_Line')")
print("2. Or use the toolbar buttons if workbench is active")

