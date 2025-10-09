# Add drawing tools to current Drawing workbench
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
            FreeCAD.Console.PrintError("No active 3D view found\n")
            return False
        
        # Set crosshair cursor to indicate drawing mode
        try:
            from PySide6 import QtCore, QtGui
            cls.view.setCursor(QtGui.QCursor(QtCore.Qt.CrossCursor))
        except:
            try:
                from PySide2 import QtCore, QtGui  
                cls.view.setCursor(QtGui.QCursor(QtCore.Qt.CrossCursor))
            except:
                pass
        
        cls.active_command = command
        command.points = []
        
        # Add mouse event callback - this captures mouse clicks for drawing
        cls.callback = cls.view.addEventCallback("SoMouseButtonEvent", cls.handle_mouse)
        
        FreeCAD.Console.PrintMessage(">>> DRAWING MODE ACTIVE <<<\n")
        FreeCAD.Console.PrintMessage(instruction + "\n")
        FreeCAD.Console.PrintMessage("Left click = place point, Right click = cancel\n")
        return True
    
    @classmethod
    def stop_drawing(cls):
        """Exit drawing mode"""
        if cls.view and cls.callback:
            try:
                cls.view.removeEventCallback("SoMouseButtonEvent", cls.callback)
            except:
                pass
        
        # Restore normal cursor
        try:
            from PySide6 import QtCore, QtGui
            if cls.view:
                cls.view.setCursor(QtGui.QCursor(QtCore.Qt.ArrowCursor))
        except:
            try:
                from PySide2 import QtCore, QtGui
                if cls.view:
                    cls.view.setCursor(QtGui.QCursor(QtCore.Qt.ArrowCursor))
            except:
                pass
        
        if cls.active_command:
            FreeCAD.Console.PrintMessage(">>> DRAWING MODE DEACTIVATED <<<\n")
        
        cls.active_command = None
        cls.callback = None
    
    @classmethod
    def handle_mouse(cls, event):
        """Handle mouse events during drawing"""
        from pivy import coin
        
        if not cls.active_command:
            return
        
        # CRITICAL: Mark event as handled to prevent normal navigation
        event.setHandled()
        
        # Left mouse button - place point
        if (event.getButton() == coin.SoMouseButtonEvent.BUTTON1 and 
            event.getState() == coin.SoMouseButtonEvent.DOWN):
            
            pos = event.getPosition()
            try:
                # Get 3D point from mouse position
                point = cls.view.getPoint(pos[0], pos[1])
                # Project to XY plane for 2D drawing
                point.z = 0
                cls.active_command.add_point(point)
            except Exception as e:
                FreeCAD.Console.PrintError("Could not get point: " + str(e) + "\n")
        
        # Right mouse button - cancel drawing
        elif (event.getButton() == coin.SoMouseButtonEvent.BUTTON3 and 
              event.getState() == coin.SoMouseButtonEvent.DOWN):
            cls.stop_drawing()

# Line drawing command
class DrawLineCommand:
    def __init__(self):
        self.points = []
    
    def GetResources(self):
        return {
            'Pixmap': 'Part_Line',
            'MenuText': 'Draw Line',
            'ToolTip': 'Draw a line by clicking two points',
            'Accel': 'L'
        }
    
    def Activated(self):
        FreeCAD.Console.PrintMessage("=== LINE DRAWING TOOL ACTIVATED ===\n")
        DrawingState.start_drawing(self, "Click FIRST point for line")
    
    def add_point(self, point):
        self.points.append(point)
        FreeCAD.Console.PrintMessage("Point {}: X={:.2f}, Y={:.2f}\n".format(
            len(self.points), point.x, point.y))
        
        if len(self.points) == 1:
            FreeCAD.Console.PrintMessage("Now click SECOND point for line\n")
        elif len(self.points) == 2:
            self.create_line()
            DrawingState.stop_drawing()
    
    def create_line(self):
        try:
            doc = FreeCAD.activeDocument()
            
            # Create line geometry
            line = Part.makeLine(self.points[0], self.points[1])
            
            # Add to document
            obj = doc.addObject("Part::Feature", "Line")
            obj.Shape = line
            obj.Label = "Line_" + str(len([o for o in doc.Objects if 'Line' in o.Label]) + 1)
            
            # Update display
            doc.recompute()
            FreeCADGui.SendMsgToActiveView("ViewFit")
            
            # Show result
            length = self.points[0].distanceToPoint(self.points[1])
            FreeCAD.Console.PrintMessage("*** LINE CREATED SUCCESSFULLY ***\n")
            FreeCAD.Console.PrintMessage("Length: {:.2f} mm\n".format(length))
            FreeCAD.Console.PrintMessage("From: ({:.2f}, {:.2f}) To: ({:.2f}, {:.2f})\n".format(
                self.points[0].x, self.points[0].y, self.points[1].x, self.points[1].y))
            
        except Exception as e:
            FreeCAD.Console.PrintError("FAILED to create line: " + str(e) + "\n")
    
    def IsActive(self):
        return True

# Circle drawing command  
class DrawCircleCommand:
    def __init__(self):
        self.points = []
    
    def GetResources(self):
        return {
            'Pixmap': 'Part_Circle',
            'MenuText': 'Draw Circle',
            'ToolTip': 'Draw a circle by clicking center and edge point',
            'Accel': 'C'
        }
    
    def Activated(self):
        FreeCAD.Console.PrintMessage("=== CIRCLE DRAWING TOOL ACTIVATED ===\n")
        DrawingState.start_drawing(self, "Click CENTER point for circle")
    
    def add_point(self, point):
        self.points.append(point)
        
        if len(self.points) == 1:
            FreeCAD.Console.PrintMessage("Center: ({:.2f}, {:.2f})\n".format(point.x, point.y))
            FreeCAD.Console.PrintMessage("Now click EDGE point to set radius\n")
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
            FreeCADGui.SendMsgToActiveView("ViewFit")
            
            FreeCAD.Console.PrintMessage("*** CIRCLE CREATED SUCCESSFULLY ***\n")
            FreeCAD.Console.PrintMessage("Radius: {:.2f} mm\n".format(radius))
            
        except Exception as e:
            FreeCAD.Console.PrintError("FAILED to create circle: " + str(e) + "\n")
    
    def IsActive(self):
        return True

# Cancel command
class CancelDrawingCommand:
    def GetResources(self):
        return {
            'Pixmap': 'Process-stop',
            'MenuText': 'Cancel Drawing',
            'ToolTip': 'Cancel current drawing operation'
        }
    
    def Activated(self):
        FreeCAD.Console.PrintMessage("Drawing operation cancelled\n")
        DrawingState.stop_drawing()
    
    def IsActive(self):
        return True

# Register the drawing commands
try:
    FreeCADGui.addCommand("Drawing_Line", DrawLineCommand())
    FreeCADGui.addCommand("Drawing_Circle", DrawCircleCommand()) 
    FreeCADGui.addCommand("Drawing_Cancel", CancelDrawingCommand())
    
    print("SUCCESS: Drawing commands registered!")
    print("You can now run:")
    print("  FreeCADGui.runCommand('Drawing_Line')")
    print("  FreeCADGui.runCommand('Drawing_Circle')")
    print("Or look for these commands in the Drawing workbench toolbar")
    
except Exception as e:
    print("Error registering commands:", str(e))

