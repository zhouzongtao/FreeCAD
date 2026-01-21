# -*- coding: utf-8 -*-
"""
Create a fresh view to test rendering
"""
import FreeCAD
import FreeCADGui

print("\n=== Creating Fresh View ===\n")

# Close all documents
for doc in FreeCAD.listDocuments().values():
    FreeCAD.closeDocument(doc.Name)
    print(f"Closed document: {doc.Name}")

# Create new document
print("\nCreating new document...")
doc = FreeCAD.newDocument("FreshTest")

# Create a simple object
print("Creating Box...")
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

print("\n=== Check Report View ===")
print("You should see:")
print("1. 'Test sphere added (GREEN, radius 3.0)' - from scene initialization")
print("2. 'Adding ViewProvider (TestBox)' - when box is created")
print("3. 'Created LARGE RED sphere with radius 5.0' - placeholder for box")
print("\nIf you see the green test sphere, OSG rendering works!")
print("If not, there's an OpenGL/OSG configuration issue.")
