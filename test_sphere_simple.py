# -*- coding: utf-8 -*-
"""
Simple sphere visibility test
"""
import FreeCAD
import FreeCADGui

print("\n=== Simple Sphere Test ===\n")

# Create test document and object
doc = FreeCAD.newDocument("SphereTest")
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

print(f"Created object: {box.Name}")
print(f"Object position: {box.Placement.Base}")

# Get ViewProvider
vp = FreeCADGui.ActiveDocument.getObject("TestBox")
if vp:
    print(f"ViewProvider exists: {vp}")
    print(f"ViewProvider visible: {vp.Visibility}")
else:
    print("ERROR: No ViewProvider!")

# Get view
view = FreeCADGui.activeDocument().activeView()
print(f"\nView type: {type(view).__name__}")

# Fit view to show all objects
FreeCADGui.SendMsgToActiveView("ViewFit")
print("\nView fitted to objects")

print("\n=== Test Complete ===")
print("You should see a RED SPHERE (radius 2.0) in the 3D view")
print("The sphere is a placeholder for the Box object")
print("\nIf you don't see it, the sphere might be:")
print("- Behind the camera")
print("- Too small/large")
print("- Not being rendered")
