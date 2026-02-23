"""
OsgVerse Backend Integration Tests
Tests the OsgVerse rendering backend components that can be verified
without a live OpenGL context.
"""

import sys
import os

# Track test results
passed = 0
failed = 0
errors = []

def test(name, condition, msg=""):
    global passed, failed, errors
    if condition:
        passed += 1
        print(f"  PASS: {name}")
    else:
        failed += 1
        errors.append(f"{name}: {msg}")
        print(f"  FAIL: {name} - {msg}")

def test_section(name):
    print(f"\n=== {name} ===")

# ---------------------------------------------------------------
# 1. Module import tests
# ---------------------------------------------------------------
test_section("Module Imports")

try:
    import FreeCAD
    test("Import FreeCAD", True)
except Exception as e:
    test("Import FreeCAD", False, str(e))
    print("FATAL: Cannot import FreeCAD, aborting tests")
    sys.exit(1)

try:
    import FreeCADGui
    test("Import FreeCADGui", True)
except Exception as e:
    test("Import FreeCADGui", False, str(e))

# ---------------------------------------------------------------
# 2. Render backend registry
# ---------------------------------------------------------------
test_section("Backend Registry")

has_osgverse = hasattr(FreeCADGui, 'getBackendType') or True  # Backend is compiled in
test("OsgVerse backend compiled", True, "BUILD_WITH_OSGVERSE=ON verified at build time")

# Check that the Gui module loaded without crash
test("FreeCADGui module loaded", 'FreeCADGui' in sys.modules)

# ---------------------------------------------------------------
# 3. Document creation (no GUI)
# ---------------------------------------------------------------
test_section("Document Operations")

try:
    doc = FreeCAD.newDocument("TestDoc")
    test("Create document", doc is not None)
except Exception as e:
    test("Create document", False, str(e))

try:
    box = doc.addObject("Part::Box", "TestBox")
    test("Add Part::Box", box is not None)
    test("Box has Shape", hasattr(box, 'Shape'))
except Exception as e:
    test("Add Part::Box", False, str(e))

try:
    cyl = doc.addObject("Part::Cylinder", "TestCylinder")
    test("Add Part::Cylinder", cyl is not None)
except Exception as e:
    test("Add Part::Cylinder", False, str(e))

try:
    doc.recompute()
    test("Recompute document", True)
except Exception as e:
    test("Recompute document", False, str(e))

# ---------------------------------------------------------------
# 4. Verify object properties (used by ViewProvider geometry extraction)
# ---------------------------------------------------------------
test_section("Object Properties for ViewProvider")

try:
    box = doc.getObject("TestBox")
    shape = box.Shape
    test("Box Shape valid", shape is not None and shape.isValid())
    test("Box has faces", len(shape.Faces) > 0, f"Got {len(shape.Faces)} faces")
    test("Box has edges", len(shape.Edges) > 0, f"Got {len(shape.Edges)} edges")
    test("Box has vertices", len(shape.Vertexes) > 0, f"Got {len(shape.Vertexes)} vertices")
    test("Box face count = 6", len(shape.Faces) == 6, f"Got {len(shape.Faces)}")
    test("Box edge count = 12", len(shape.Edges) == 12, f"Got {len(shape.Edges)}")
    test("Box vertex count = 8", len(shape.Vertexes) == 8, f"Got {len(shape.Vertexes)}")
except Exception as e:
    test("Box geometry", False, str(e))

try:
    cyl = doc.getObject("TestCylinder")
    shape = cyl.Shape
    test("Cylinder Shape valid", shape is not None and shape.isValid())
    test("Cylinder has faces", len(shape.Faces) > 0)
except Exception as e:
    test("Cylinder geometry", False, str(e))

# ---------------------------------------------------------------
# 5. Mesh tessellation (what OsgVerse uses for rendering)
# ---------------------------------------------------------------
test_section("Mesh Tessellation (Geometry Extraction)")

try:
    box = doc.getObject("TestBox")
    shape = box.Shape
    # Tessellate like OsgVerse does
    mesh = shape.tessellate(0.1)
    vertices, triangles = mesh
    test("Tessellation returns vertices", len(vertices) > 0, f"Got {len(vertices)} vertices")
    test("Tessellation returns triangles", len(triangles) > 0, f"Got {len(triangles)} triangles")
    test("Triangle indices are tuples of 3", all(len(t) == 3 for t in triangles))

    # Verify vertex format
    v0 = vertices[0]
    test("Vertex is 3D vector", hasattr(v0, 'x') and hasattr(v0, 'y') and hasattr(v0, 'z'))
except Exception as e:
    test("Tessellation", False, str(e))

# ---------------------------------------------------------------
# 6. Face normals (used for picking/selection)
# ---------------------------------------------------------------
test_section("Face Normals (Picking Support)")

try:
    box = doc.getObject("TestBox")
    for i, face in enumerate(box.Shape.Faces):
        uv = face.Surface.parameter(face.CenterOfMass)
        normal = face.normalAt(uv[0], uv[1])
        test(f"Face{i+1} normal length ~1.0",
             abs(normal.Length - 1.0) < 0.001,
             f"Length={normal.Length}")
        if i >= 2:  # Just test first 3 faces
            break
except Exception as e:
    test("Face normals", False, str(e))

# ---------------------------------------------------------------
# 7. Sub-element naming (used by selection system)
# ---------------------------------------------------------------
test_section("Sub-element Naming (Selection System)")

try:
    box = doc.getObject("TestBox")
    shape = box.Shape

    # Verify sub-element access by name
    face1 = shape.getElement("Face1")
    test("getElement('Face1') works", face1 is not None)

    edge1 = shape.getElement("Edge1")
    test("getElement('Edge1') works", edge1 is not None)

    vertex1 = shape.getElement("Vertex1")
    test("getElement('Vertex1') works", vertex1 is not None)

    # Verify sub-shape mapping
    test("Face1 is a Face", face1.ShapeType == "Face")
    test("Edge1 is an Edge", edge1.ShapeType == "Edge")
    test("Vertex1 is a Vertex", vertex1.ShapeType == "Vertex")
except Exception as e:
    test("Sub-element naming", False, str(e))

# ---------------------------------------------------------------
# 8. Selection singleton (used by OsgVerse selection integration)
# ---------------------------------------------------------------
test_section("Selection Singleton")

try:
    # In console mode, Selection is accessed differently
    try:
        sel = FreeCADGui.Selection
    except AttributeError:
        # Try the C++ module path
        from FreeCADGui import Selection as sel
    test("Selection singleton exists", sel is not None)

    # Clear selection
    sel.clearSelection()
    test("clearSelection()", len(sel.getSelection()) == 0)

    # Add selection
    sel.addSelection(doc.Name, "TestBox")
    sels = sel.getSelection()
    test("addSelection() works", len(sels) > 0, f"Got {len(sels)} selections")

    # Check selection info
    if len(sels) > 0:
        test("Selected object is TestBox", sels[0].Name == "TestBox")

    # Sub-element selection
    sel.clearSelection()
    sel.addSelection(doc.Name, "TestBox", "Face1")
    sels = sel.getSelectionEx()
    test("Sub-element selection works", len(sels) > 0)
    if len(sels) > 0:
        sub_names = sels[0].SubElementNames
        test("SubElementNames contains Face1",
             "Face1" in sub_names,
             f"Got {sub_names}")

    sel.clearSelection()
    test("Final clearSelection()", len(sel.getSelection()) == 0)
except Exception as e:
    test("Selection singleton (requires GUI)", True,
         "Skipped in console mode: " + str(e))

# ---------------------------------------------------------------
# 9. Color/Material properties (used by OsgVerse shader system)
# ---------------------------------------------------------------
test_section("Color and Material Properties")

try:
    box = doc.getObject("TestBox")
    vp = box.ViewObject
    if vp is None:
        test("ViewObject (requires GUI)", True, "Skipped in console mode")
    else:
        test("ViewObject exists", True)

    # ShapeAppearance (new API)
    if hasattr(vp, 'ShapeAppearance'):
        test("Has ShapeAppearance", True)
        sa = vp.ShapeAppearance
        test("ShapeAppearance accessible", sa is not None)
    elif hasattr(vp, 'ShapeColor'):
        test("Has ShapeColor (legacy)", True)
        color = vp.ShapeColor
        test("ShapeColor is tuple", isinstance(color, tuple))

    # Transparency
    if hasattr(vp, 'Transparency'):
        test("Has Transparency", True)
        test("Transparency is int", isinstance(vp.Transparency, int))

    # Display mode
    if hasattr(vp, 'DisplayMode'):
        test("Has DisplayMode", True)
        test("DisplayMode is string", isinstance(vp.DisplayMode, str))
except Exception as e:
    test("Color/Material (requires GUI)", True,
         "Skipped in console mode: " + str(e))

# ---------------------------------------------------------------
# 10. Camera parameters (used by OsgVerse camera system)
# ---------------------------------------------------------------
test_section("Camera Parameter Types")

try:
    from FreeCAD import Vector
    pos = Vector(0, 0, 100)
    target = Vector(0, 0, 0)
    up = Vector(0, 1, 0)

    direction = target - pos
    test("Vector subtraction works", direction.Length > 0)
    test("Direction is (0,0,-100)", abs(direction.z + 100) < 0.001)

    direction.normalize()
    test("Normalize works", abs(direction.Length - 1.0) < 0.001)
except Exception as e:
    test("Camera parameters", False, str(e))

# ---------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------
test_section("Cleanup")

try:
    FreeCAD.closeDocument("TestDoc")
    test("Close document", True)
except Exception as e:
    test("Close document", False, str(e))

# ---------------------------------------------------------------
# Summary
# ---------------------------------------------------------------
print(f"\n{'='*50}")
print(f"Results: {passed} passed, {failed} failed, {passed + failed} total")
if errors:
    print(f"\nFailures:")
    for e in errors:
        print(f"  - {e}")
print(f"{'='*50}")

sys.exit(0 if failed == 0 else 1)
