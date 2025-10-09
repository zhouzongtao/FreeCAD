# Force add Drawing toolbar to current FreeCAD session

import FreeCAD
import FreeCADGui
import Part

print("=== Adding Drawing Toolbar Immediately ===")

# First, let's create the drawing commands
class DrawingState:
    active = False
    view = None
    callback = None
    command = None
    
    @classmethod
    def start(cls, cmd, msg):
        if not FreeCAD.activeDocument():
            FreeCAD.newDocument()
        cls.view = FreeCADGui.activeDocument().activeView()
        if not cls.view:
            return False
        
        # Change cursor
        try:
            from PySide6.QtCore import Qt
            from PySide6.QtGui import QCursor
            cls.view.setCursor(QCursor(Qt.CrossCursor))
        except:
            pass
        
        cls.active = True
        cls.command = cmd
        cmd.points = []
        cls.callback = cls.view.addEventCallback("SoMouseButtonEvent", cls.mouse_event)
        
        FreeCAD.Console.PrintMessage("DRAWING: " + msg + "\n")
        FreeCAD.Console.PrintMessage("Left click to place points, Right click to stop\n")
        return True
    
    @classmethod
    def stop(cls):
        if cls.view and cls.callback:
            cls.view.removeEventCallback("SoMouseButtonEvent", cls.callback)
        try:
            from PySide6.QtCore import Qt
            from PySide6.QtGui import QCursor
            cls.view.setCursor(QCursor(Qt.ArrowCursor))
        except:
            pass
        cls.active = False
        cls.command = None
        FreeCAD.Console.PrintMessage("Drawing stopped\n")
    
    @classmethod
    def mouse_event(cls, event):
        from pivy import coin
        if not cls.active or not cls.command:
            return
        
        event.setHandled()  # Prevent normal navigation
        
        if (event.getButton() == coin.SoMouseButtonEvent.BUTTON1 and 
            event.getState() == coin.SoMouseButtonEvent.DOWN):
            pos = event.getPosition()
            try:
                point = cls.view.getPoint(pos[0], pos[1])
                point.z = 0
                cls.command.add_point(point)
            except:
                pass
        elif (event.getButton() == coin.SoMouseButtonEvent.BUTTON3 and 
              event.getState() == coin.SoMouseButtonEvent.DOWN):
            cls.stop()

class LineCmd:
    def GetResources(self):
        return {
            'Pixmap': 'Part_Line',
            'MenuText': 'Line',
            'ToolTip': 'Draw line - click 2 points'
        }
    
    def Activated(self):
        DrawingState.start(self, "Click start point")
    
    def add_point(self, pt):
        self.points.append(pt)
        FreeCAD.Console.PrintMessage("Point: ({:.1f}, {:.1f})\n".format(pt.x, pt.y))
        
        if len(self.points) == 2:
            doc = FreeCAD.activeDocument()
            line = Part.makeLine(self.points[0], self.points[1])
            obj = doc.addObject("Part::Feature", "Line")
            obj.Shape = line
            doc.recompute()
            FreeCAD.Console.PrintMessage("Line created!\n")
            DrawingState.stop()
    
    def IsActive(self):
        return True

class CircleCmd:
    def GetResources(self):
        return {
            'Pixmap': 'Part_Circle', 
            'MenuText': 'Circle',
            'ToolTip': 'Draw circle - click center + edge'
        }
    
    def Activated(self):
        DrawingState.start(self, "Click center point")
    
    def add_point(self, pt):
        self.points.append(pt)
        
        if len(self.points) == 1:
            FreeCAD.Console.PrintMessage("Center: ({:.1f}, {:.1f})\n".format(pt.x, pt.y))
            FreeCAD.Console.PrintMessage("Click edge point\n")
        elif len(self.points) == 2:
            doc = FreeCAD.activeDocument()
            center = self.points[0]
            radius = center.distanceToPoint(self.points[1])
            circle = Part.makeCircle(radius, center)
            obj = doc.addObject("Part::Feature", "Circle")
            obj.Shape = circle
            doc.recompute()
            FreeCAD.Console.PrintMessage("Circle created! R={:.1f}\n".format(radius))
            DrawingState.stop()
    
    def IsActive(self):
        return True

class StopCmd:
    def GetResources(self):
        return {
            'Pixmap': 'Process-stop',
            'MenuText': 'Stop',
            'ToolTip': 'Stop drawing'
        }
    
    def Activated(self):
        DrawingState.stop()
    
    def IsActive(self):
        return True

# Register commands
FreeCADGui.addCommand("Draw_Line", LineCmd())
FreeCADGui.addCommand("Draw_Circle", CircleCmd())
FreeCADGui.addCommand("Draw_Stop", StopCmd())

print("Commands registered!")

# Method 1: Try to add to current workbench
try:
    wb = FreeCADGui.activeWorkbench()
    if wb:
        wb.appendToolbar("Drawing", ["Draw_Line", "Draw_Circle", "Draw_Stop"])
        print("SUCCESS: Toolbar added to current workbench!")
    else:
        print("No active workbench found")
except Exception as e:
    print("Method 1 failed:", str(e))

# Method 2: Force create toolbar in main window
try:
    mw = FreeCADGui.getMainWindow()
    if mw:
        # Create new toolbar
        toolbar = mw.addToolBar("Drawing Tools")
        
        # Add actions
        action1 = toolbar.addAction("Line")
        action1.triggered.connect(lambda: FreeCADGui.runCommand("Draw_Line"))
        
        action2 = toolbar.addAction("Circle") 
        action2.triggered.connect(lambda: FreeCADGui.runCommand("Draw_Circle"))
        
        action3 = toolbar.addAction("Stop")
        action3.triggered.connect(lambda: FreeCADGui.runCommand("Draw_Stop"))
        
        toolbar.show()
        print("SUCCESS: Drawing toolbar created in main window!")
    
except Exception as e:
    print("Method 2 failed:", str(e))

print("\nYou can also run commands directly:")
print("FreeCADGui.runCommand('Draw_Line')   # Start line drawing")
print("FreeCADGui.runCommand('Draw_Circle') # Start circle drawing")
print("FreeCADGui.runCommand('Draw_Stop')   # Stop drawing")

