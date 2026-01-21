# -*- coding: utf-8 -*-
"""
Create test object and check if sphere is visible
"""
import FreeCAD
import FreeCADGui

print("\n=== Creating Test Object ===\n")

# Get or create document
doc = FreeCAD.ActiveDocument
if not doc:
    print("Creating new document...")
    doc = FreeCAD.newDocument("TestDoc")
else:
    print(f"Using active document: {doc.Name}")

# Create a Box object
print("\nCreating Box object...")
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

print(f"Box created: {box.Name}")
print(f"Box dimensions: {box.Length} x {box.Width} x {box.Height}")

# Check ViewProvider
vp = FreeCADGui.ActiveDocument.getObject("TestBox")
if vp:
    print(f"\nViewProvider exists: {vp}")
    print(f"ViewProvider visible: {vp.Visibility}")
else:
    print("\nERROR: No ViewProvider created!")

# Fit view
print("\nFitting view to show object...")
FreeCADGui.SendMsgToActiveView("ViewFit")

print("\n=== Test Complete ===")
print("\nCheck Report View for these messages:")
print("- OsgVerseViewerImpl: Adding ViewProvider")
print("- OsgVerseViewerImpl: Created sphere with radius 2.0 at origin")
print("- OsgVerseViewerImpl: Added node to container")
print("- OsgVerseViewerImpl: ViewProvider added successfully")
print("\nYou should see a RED SPHERE (radius 2.0) in the 3D view")
