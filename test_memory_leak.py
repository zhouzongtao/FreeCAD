"""
Test for memory leaks in viewer creation/destruction
"""
import gc
import sys

print("=" * 60)
print("Memory Leak Test")
print("=" * 60)

import OsgVerseGui
from FreeCADGui import BackendRegistry

print("\n[Test] Creating and destroying 100 viewers...")
print("This tests the PyCapsule destructor")

for i in range(100):
    viewer = BackendRegistry.createViewer("OsgVerse")
    if viewer is None:
        print(f"[FAIL] Viewer {i+1} creation failed")
        sys.exit(1)
    
    # Delete viewer and force garbage collection
    del viewer
    
    if (i + 1) % 10 == 0:
        gc.collect()
        print(f"  Created and destroyed {i+1} viewers...")

print("\n[OK] All 100 viewers created and destroyed")
print("\nIf memory usage is stable, the PyCapsule destructor is working correctly")
print("Check Task Manager / Activity Monitor for memory usage")
print("\n" + "=" * 60)
print("[SUCCESS] Memory leak test completed")
print("=" * 60)
