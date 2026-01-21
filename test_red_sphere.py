# -*- coding: utf-8 -*-
"""
Test if red ViewProvider spheres are visible
"""
import FreeCAD
import FreeCADGui

print("\n=== Testing Red ViewProvider Spheres ===\n")

# Get or create document
doc = FreeCAD.ActiveDocument
if not doc:
    doc = FreeCAD.newDocument("RedSphereTest")

# Create a box
print("Creating Box...")
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

print("\n=== What you should see ===")
print("1. GREEN sphere (radius 3.0) - test sphere, always visible")
print("2. RED sphere (radius 5.0) - placeholder for TestBox")
print("\nIf you only see green but not red:")
print("- The ViewProvider nodes are not being rendered")
print("- Possible issue with _vpContainerNode or node hierarchy")
