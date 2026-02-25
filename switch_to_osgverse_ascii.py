#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
How to switch to OsgVerse view
This script demonstrates how to switch to OsgVerse rendering backend in FreeCAD

使用方法 / Usage:
./build/debug/bin/FreeCAD --console /Users/zhouzongtao/repository/FreeCAD/switch_to_osgverse_ascii.py
"""

import FreeCAD
import FreeCADGui

print("=" * 70)
print("OsgVerse Backend Switching Guide")
print("=" * 70)

# Backend type constants
BACKEND_NONE = 0
BACKEND_COIN3D = 1
BACKEND_OSGVERSE = 2

print("\nStep 1: Check current backend")
print("-" * 70)
try:
    current = FreeCADGui.getCurrentRenderBackend()
    backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print("Current backend: {} ({})".format(current, backend_names.get(current, "Unknown")))
except Exception as e:
    print("Error: {}".format(e))
    print("Hint: RenderManager may not be initialized")

print("\nStep 2: Check if OsgVerse is available")
print("-" * 70)
try:
    available = FreeCADGui.isRenderBackendAvailable(BACKEND_OSGVERSE)
    if available:
        print("[SUCCESS] OsgVerse backend is available!")
    else:
        print("[WARNING] OsgVerse backend is not available")
        print("          Make sure OsgVerseGui module is loaded")
except Exception as e:
    print("Error: {}".format(e))

print("\nStep 3: Switch to OsgVerse backend")
print("-" * 70)
try:
    print("Switching to OsgVerse...")
    success = FreeCADGui.switchRenderBackend(BACKEND_OSGVERSE)
    if success:
        print("[SUCCESS] Switched to OsgVerse backend!")
        
        # Verify the switch
        current = FreeCADGui.getCurrentRenderBackend()
        if current == BACKEND_OSGVERSE:
            print("[VERIFIED] Current backend is indeed OsgVerse")
        else:
            print("[WARNING] Switch may not have taken full effect")
    else:
        print("[FAILED] Could not switch to OsgVerse backend")
        print("         Possible reasons:")
        print("         - OsgVerse backend not registered")
        print("         - RenderManager not initialized")
except Exception as e:
    print("Error: {}".format(e))

print("\nStep 4: Create test document")
print("-" * 70)
try:
    doc = FreeCAD.newDocument("OsgVerseTest")
    print("Document created: {}".format(doc.Name))
    
    # Add a Box
    box = doc.addObject("Part::Box", "Box")
    box.Length = 10
    box.Width = 10
    box.Height = 10
    doc.recompute()
    print("Box object added")
    
    print("\n[HINT] Newly created 3D views should use OsgVerse backend")
    print("       Check the 3D view in FreeCAD window")
    
except Exception as e:
    print("Error: {}".format(e))

print("\nStep 5: Get renderer information")
print("-" * 70)
try:
    info = FreeCADGui.getRendererInfo()
    print("Renderer info: {}".format(info))
except Exception as e:
    print("Error: {}".format(e))

print("\n" + "=" * 70)
print("Switching completed!")
print("=" * 70)

print("\nOther useful commands:")
print("-" * 70)
print("# Switch back to Coin3D:")
print("FreeCADGui.switchRenderBackend(1)")
print()
print("# Get render statistics:")
print("stats = FreeCADGui.getRenderStats()")
print("print(stats)")
print()
print("# Reset statistics:")
print("FreeCADGui.resetRenderStats()")
