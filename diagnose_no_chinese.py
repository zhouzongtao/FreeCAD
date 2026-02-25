# -*- coding: utf-8 -*-
"""
Detailed diagnosis of viewer creation failure
NO CHINESE CHARACTERS - ASCII only
"""

print("=" * 60)
print("Detailed Viewer Creation Diagnosis")
print("=" * 60)
print("\nIMPORTANT: Check the FreeCAD console output above")
print("for any error messages from OsgVerseWidget or OsgVerseViewer")
print("=" * 60)

# Step 1: Import
print("\n[Step 1] Importing OsgVerseGui...")
try:
    import OsgVerseGui
    print("[OK] Module imported")
except Exception as e:
    print("[FAIL] Import failed: %s" % str(e))
    import sys
    sys.exit(1)

# Step 2: Check factory
print("\n[Step 2] Checking backend factory...")
try:
    from FreeCADGui import BackendRegistry
    
    # Get backend info
    info = BackendRegistry.getBackendInfo("OsgVerse")
    print("Backend name: %s" % info.get('name'))
    print("Backend available: %s" % info.get('available'))
    print("Backend version: %s" % info.get('version'))
    
    if info.get('available') != 'true':
        print("[FAIL] Backend reports as not available!")
        import sys
        sys.exit(1)
    
    print("[OK] Backend is available")
    
except Exception as e:
    print("[FAIL] Backend check failed: %s" % str(e))
    import traceback
    traceback.print_exc()
    import sys
    sys.exit(1)

# Step 3: Try to create viewer with error catching
print("\n[Step 3] Attempting to create viewer...")
print("Watch the console output above for error messages!")
print()

try:
    print("Calling BackendRegistry.createViewer('OsgVerse')...")
    viewer = BackendRegistry.createViewer("OsgVerse")
    
    print("\nResult: %s" % viewer)
    print("Type: %s" % type(viewer))
    
    if viewer is None:
        print("\n" + "=" * 60)
        print("[FAIL] createViewer returned None")
        print("=" * 60)
        print("\nThis means an exception was caught in:")
        print("  OsgVerseBackendFactory::createViewer()")
        print("\nThe exception was likely thrown by:")
        print("  1. OsgVerseViewer constructor")
        print("  2. OsgVerseWidget constructor")
        print("  3. OSG viewer initialization")
        print("\nPlease scroll up in the FreeCAD console and look for:")
        print("  - 'OsgVerseWidget: Creating widget'")
        print("  - 'OsgVerseWidget: Widget and viewer created'")
        print("  - 'OsgVerseViewer: Creating viewer'")
        print("  - 'OsgVerseViewer: Viewer created successfully'")
        print("  - Any error messages from OSG or Qt")
        print("\nIf you don't see these messages, the constructor failed.")
        print("=" * 60)
    else:
        print("\n[OK] Viewer created successfully!")
        print("Backend: %s" % viewer.getBackendName())
        
except Exception as e:
    print("\n[FAIL] Exception: %s" % str(e))
    import traceback
    traceback.print_exc()

print("\n" + "=" * 60)
print("Diagnosis complete - check console output above")
print("=" * 60)
