# -*- coding: utf-8 -*-
"""
Debug scene graph structure
"""
import FreeCAD
import FreeCADGui

print("\n=== Scene Graph Debug ===\n")

# Create test object
doc = FreeCAD.newDocument("DebugTest")
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10  
box.Height = 10
doc.recompute()

print(f"Created Box: {box.Name}")

# Get view
view = FreeCADGui.activeDocument().activeView()
print(f"View type: {type(view).__name__}")

# Try to get viewer
try:
    viewer = view.getViewer()
    print(f"Viewer: {viewer}")
    print(f"Viewer type: {type(viewer)}")
    
    # Check if viewer has methods to inspect scene
    print(f"\nViewer methods:")
    for attr in dir(viewer):
        if not attr.startswith('_'):
            print(f"  - {attr}")
            
except Exception as e:
    print(f"Error getting viewer: {e}")
    import traceback
    traceback.print_exc()

# Fit view
FreeCADGui.SendMsgToActiveView("ViewFit")

print("\n=== Debug Complete ===")
