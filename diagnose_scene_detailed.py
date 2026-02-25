# -*- coding: utf-8 -*-
"""
Detailed scene diagnostics
"""
import FreeCAD
import FreeCADGui

print("\n=== Detailed Scene Diagnostics ===\n")

# Get view and viewer
view = FreeCADGui.activeDocument().activeView()
print(f"View: {view}")

if view:
    viewer = view.getViewer()
    print(f"Viewer: {viewer}")
    
    # Check ViewProviders
    try:
        vps = viewer.getViewProviders()
        print(f"\nNumber of ViewProviders: {len(vps)}")
        for i, vp in enumerate(vps):
            print(f"  VP {i}: {vp}")
    except Exception as e:
        print(f"Cannot get ViewProviders: {e}")
    
    # Check widget
    widget = viewer.getWidget()
    print(f"\nWidget: {widget}")
    print(f"Widget visible: {widget.isVisible()}")
    print(f"Widget size: {widget.width()}x{widget.height()}")
    print(f"Widget has focus: {widget.hasFocus()}")
    
    # Force multiple updates
    print("\nForcing updates...")
    for i in range(3):
        widget.update()
        print(f"  Update {i+1} sent")

print("\n=== Diagnostics Complete ===")
print("\nWhat do you see in the 3D view?")
print("- Gray/blue background = Good")
print("- Black areas = OpenGL issue")
print("- Nothing = Rendering not working")
