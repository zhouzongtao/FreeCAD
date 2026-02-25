# -*- coding: utf-8 -*-
"""
Test if rendering is working
"""
import FreeCAD
import FreeCADGui

print("\n=== Rendering Test ===\n")

# Get active view
view = FreeCADGui.activeDocument().activeView()
print(f"Active view: {view}")
print(f"View type: {type(view).__name__}")

# Try to get viewer
try:
    viewer = view.getViewer()
    print(f"Viewer: {viewer}")
    print(f"Viewer type: {type(viewer)}")
    
    # Check widget
    widget = viewer.getWidget()
    print(f"Widget: {widget}")
    print(f"Widget visible: {widget.isVisible()}")
    print(f"Widget size: {widget.width()}x{widget.height()}")
    
    # Force update
    print("\nForcing widget update...")
    widget.update()
    
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()

print("\n=== Test Complete ===")
print("\nIf the widget is visible and has a size > 0, rendering should work.")
print("The 3D view should show a gray background.")
print("If you see a black screen, there might be an OpenGL issue.")
