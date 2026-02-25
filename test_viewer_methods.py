"""
Test all viewer methods
"""
print("=" * 60)
print("Testing Viewer Methods")
print("=" * 60)

import OsgVerseGui
from FreeCADGui import BackendRegistry
from PySide6.QtGui import QColor

# Create viewer
print("\n[Test 1] Creating viewer...")
viewer = BackendRegistry.createViewer("OsgVerse")
assert viewer is not None
print(f"[OK] Viewer created: {viewer}")

# Test getBackendName
print("\n[Test 2] getBackendName()...")
backend = viewer.getBackendName()
print(f"Backend: {backend}")
assert backend == "OsgVerse"
print("[OK] getBackendName works")

# Test getWidget
print("\n[Test 3] getWidget()...")
widget = viewer.getWidget()
print(f"Widget pointer: {widget}")
assert widget is not None
print("[OK] getWidget works")

# Test render
print("\n[Test 4] render()...")
viewer.render()
print("[OK] render works")

# Test viewAll
print("\n[Test 5] viewAll()...")
viewer.viewAll()
print("[OK] viewAll works")

# Test setBackgroundColor
print("\n[Test 6] setBackgroundColor()...")
color = QColor(50, 50, 80)
viewer.setBackgroundColor(color)
print("[OK] setBackgroundColor works")

# Test clearScene
print("\n[Test 7] clearScene()...")
viewer.clearScene()
print("[OK] clearScene works")

# Test getVersion
print("\n[Test 8] getVersion()...")
version = viewer.getVersion()
print(f"Version: {version}")
assert "OsgVerse" in version
print("[OK] getVersion works")

print("\n" + "=" * 60)
print("[SUCCESS] All viewer methods work!")
print("=" * 60)
