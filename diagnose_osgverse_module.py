"""
Diagnose OsgVerse Module Loading
"""

print("=" * 60)
print("OsgVerse Module Diagnostic")
print("=" * 60)

# Test 1: Check if module file exists
print("\n[Test 1] Checking module file...")
import os
import sys

module_path = r"E:\Repository\FreeCAD\FreeCAD\build\Mod\OsgVerseGui\OsgVerseGui.pyd"
if os.path.exists(module_path):
    print(f"[OK] Module file exists: {module_path}")
    file_size = os.path.getsize(module_path)
    print(f"     File size: {file_size:,} bytes")
else:
    print(f"[ERROR] Module file not found: {module_path}")
    print("\nPlease check:")
    print("1. Module was compiled successfully")
    print("2. File is in the correct location")
    exit(1)

# Test 2: Check Python path
print("\n[Test 2] Checking Python path...")
mod_dir = r"E:\Repository\FreeCAD\FreeCAD\build\Mod\OsgVerseGui"
if mod_dir in sys.path:
    print(f"[OK] Module directory in sys.path")
else:
    print(f"[WARNING] Module directory not in sys.path")
    print(f"Adding: {mod_dir}")
    sys.path.insert(0, mod_dir)

# Test 3: Try to import module
print("\n[Test 3] Attempting to import OsgVerseGui...")
try:
    import OsgVerseGui
    print("[OK] OsgVerseGui imported successfully")
except ImportError as e:
    print(f"[ERROR] Failed to import OsgVerseGui: {e}")
    print("\nPossible causes:")
    print("1. Missing dependencies (OSG DLLs)")
    print("2. Module not compiled correctly")
    print("3. Python version mismatch")
    exit(1)
except Exception as e:
    print(f"[ERROR] Unexpected error: {e}")
    exit(1)

# Test 4: Check backend registration
print("\n[Test 4] Checking backend registration...")
try:
    from FreeCADGui import BackendRegistry
    
    backends = BackendRegistry.getAvailableBackends()
    print(f"Available backends: {backends}")
    
    if "OsgVerse" in backends:
        print("[OK] OsgVerse backend is registered")
        
        # Get backend info
        info = BackendRegistry.getBackendInfo("OsgVerse")
        print(f"\nBackend Info:")
        print(f"  Name: {info['name']}")
        print(f"  Version: {info['version']}")
        print(f"  Available: {info['available']}")
    else:
        print("[ERROR] OsgVerse backend is NOT registered")
        print("\nThis means the module loaded but registration failed.")
        print("Check the console output for error messages during module init.")
        
except Exception as e:
    print(f"[ERROR] Failed to check backend: {e}")
    import traceback
    traceback.print_exc()

# Test 5: Check default backend
print("\n[Test 5] Checking default backend...")
try:
    default = BackendRegistry.getDefaultBackend()
    print(f"Default backend: {default}")
    
    if default == "Coin3D":
        print("[INFO] Coin3D is the default (expected)")
        print("      OsgVerse can be used by calling:")
        print("      BackendRegistry.setDefaultBackend('OsgVerse')")
    elif default == "OsgVerse":
        print("[INFO] OsgVerse is already the default")
    
except Exception as e:
    print(f"[ERROR] Failed to get default backend: {e}")

# Test 6: Try to create viewer
print("\n[Test 6] Attempting to create OsgVerse viewer...")
try:
    viewer = BackendRegistry.createViewer("OsgVerse")
    if viewer:
        print("[OK] Viewer created successfully")
        print(f"     Type: {type(viewer)}")
        print(f"     Backend: {viewer.getBackendName()}")
    else:
        print("[ERROR] createViewer returned None")
        
except Exception as e:
    print(f"[ERROR] Failed to create viewer: {e}")
    import traceback
    traceback.print_exc()

print("\n" + "=" * 60)
print("Diagnostic Complete")
print("=" * 60)

print("\n[SUMMARY]")
print("If OsgVerse backend is not registered:")
print("1. Check FreeCAD console for error messages")
print("2. Verify all OSG DLLs are in the correct location")
print("3. Try manually importing: import OsgVerseGui")
print("4. Check if module initialization ran successfully")
