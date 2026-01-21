"""
Test manual RenderManager initialization
"""

import FreeCADGui

print("=" * 60)
print("Testing Manual RenderManager Initialization")
print("=" * 60)

# Check if initializeRenderManager function exists
print("\n1. Checking if initializeRenderManager exists...")
if hasattr(FreeCADGui, 'initializeRenderManager'):
    print("   ✓ initializeRenderManager function found")
else:
    print("   ✗ initializeRenderManager function NOT found")
    print("   Available functions:", [x for x in dir(FreeCADGui) if 'render' in x.lower() or 'init' in x.lower()])
    exit(1)

# Check OsgVerse availability BEFORE initialization
print("\n2. Checking OsgVerse availability BEFORE initialization...")
osgverse_before = FreeCADGui.isRenderBackendAvailable(2)
print(f"   OsgVerse available: {osgverse_before}")

# Manually initialize RenderManager
print("\n3. Calling initializeRenderManager()...")
try:
    result = FreeCADGui.initializeRenderManager()
    print(f"   Result: {result}")
    if result:
        print("   ✓ RenderManager initialized successfully")
    else:
        print("   ✗ RenderManager initialization returned False")
except Exception as e:
    print(f"   ✗ Exception: {e}")
    exit(1)

# Check OsgVerse availability AFTER initialization
print("\n4. Checking OsgVerse availability AFTER initialization...")
osgverse_after = FreeCADGui.isRenderBackendAvailable(2)
print(f"   OsgVerse available: {osgverse_after}")

if osgverse_after:
    print("   ✓ OsgVerse is now available!")
else:
    print("   ✗ OsgVerse is still not available")

# Check current backend
print("\n5. Checking current backend...")
current = FreeCADGui.getCurrentRenderBackend()
backends = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
print(f"   Current backend: {current} ({backends.get(current, 'Unknown')})")

# Get renderer info
print("\n6. Getting renderer info...")
info = FreeCADGui.getRendererInfo()
print(f"   Renderer: {info}")

# If OsgVerse is available, try to switch to it
if osgverse_after:
    print("\n7. Attempting to switch to OsgVerse...")
    try:
        switch_result = FreeCADGui.switchRenderBackend(2)
        print(f"   Switch result: {switch_result}")
        if switch_result:
            print("   ✓ Successfully switched to OsgVerse")
            new_backend = FreeCADGui.getCurrentRenderBackend()
            new_info = FreeCADGui.getRendererInfo()
            print(f"   New backend: {new_backend} ({backends.get(new_backend, 'Unknown')})")
            print(f"   New renderer: {new_info}")
        else:
            print("   ✗ Failed to switch to OsgVerse")
    except Exception as e:
        print(f"   ✗ Exception during switch: {e}")

print("\n" + "=" * 60)
print("Test Complete")
print("=" * 60)
