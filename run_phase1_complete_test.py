"""
Complete Phase 1 Test Suite
Tests all Phase 1 requirements from the spec
"""
print("=" * 60)
print("Phase 1: Qt Widget Integration - Complete Test")
print("=" * 60)

import OsgVerseGui
from FreeCADGui import BackendRegistry
from PySide6.QtGui import QColor
import FreeCAD
import Part

# Test 1: Module Import
print("\n[Test 1] Module Import...")
print("[OK] OsgVerseGui imported")

# Test 2: Backend Registration
print("\n[Test 2] Backend Registration...")
backends = BackendRegistry.getAvailableBackends()
print(f"Available backends: {backends}")
assert "OsgVerse" in backends
print("[OK] OsgVerse backend registered")

# Test 3: Backend Info
print("\n[Test 3] Backend Info...")
info = BackendRegistry.getBackendInfo("OsgVerse")
print(f"Name: {info['name']}")
print(f"Version: {info['version']}")
assert info['name'] == "OsgVerse"
print("[OK] Backend info retrieved")

# Test 4: Create Viewer
print("\n[Test 4] Create Viewer...")
viewer = BackendRegistry.createViewer("OsgVerse")
assert viewer is not None
print(f"[OK] Viewer created: {type(viewer).__name__}")

# Test 5: Get Widget
print("\n[Test 5] Get Widget...")
widget = viewer.getWidget()
assert widget is not None
print(f"[OK] Widget obtained: {widget}")

# Test 6: Basic Operations
print("\n[Test 6] Basic Operations...")
viewer.setBackgroundColor(QColor(50, 50, 80))
viewer.render()
viewer.viewAll()
print("[OK] All basic operations successful")

# Test 7: Geometry Creation
print("\n[Test 7] Geometry Creation...")
doc = FreeCAD.newDocument("Phase1Test")
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()
print(f"[OK] Box created: {box.Name}")

print("\n" + "=" * 60)
print("[SUCCESS] Phase 1 Complete Test PASSED!")
print("=" * 60)
print("\nPhase 1 Status: COMPLETE")
print("- Qt Widget Integration: OK")
print("- Python Bindings: OK")
print("- Viewer Creation: OK")
print("- Basic Operations: OK")
print("\nReady for Phase 2: Event Handling")
