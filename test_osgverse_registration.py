"""
Test OsgVerse Backend Registration
Quick test to verify the backend is registered
"""

print("=" * 60)
print("OsgVerse Backend Registration Test")
print("=" * 60)

print("\n[Step 1] Checking available backends...")
try:
    from FreeCADGui import BackendRegistry
    backends = BackendRegistry.getAvailableBackends()
    print(f"Available backends: {backends}")
    
    if "OsgVerse" in backends:
        print("[SUCCESS] ✅ OsgVerse backend is registered!")
        
        # Get backend info
        print("\n[Step 2] Getting backend info...")
        info = BackendRegistry.getBackendInfo("OsgVerse")
        print(f"  Name: {info['name']}")
        print(f"  Version: {info['version']}")
        print(f"  Available: {info['available']}")
        
        # Try to create viewer
        print("\n[Step 3] Creating viewer...")
        viewer = BackendRegistry.createViewer("OsgVerse")
        if viewer:
            print(f"[SUCCESS] ✅ Viewer created!")
            print(f"  Type: {type(viewer).__name__}")
            print(f"  Backend: {viewer.getBackendName()}")
            
            print("\n" + "=" * 60)
            print("ALL TESTS PASSED! ✅")
            print("=" * 60)
            print("\nOsgVerse backend is working correctly.")
            print("You can now run the Phase 2 event handling tests.")
            print("\nNext step:")
            print("  exec(open(r'E:\\Repository\\FreeCAD\\FreeCAD\\test_phase2_events.py', encoding='utf-8').read())")
        else:
            print("[ERROR] ❌ Failed to create viewer")
    else:
        print("[ERROR] ❌ OsgVerse backend is NOT registered")
        print("\nPossible causes:")
        print("1. Init.py not executed")
        print("2. Module initialization failed")
        print("3. Check console for error messages")
        
        print("\nTrying to manually import...")
        import OsgVerseGui
        print("OsgVerseGui module imported")
        
        # Check again
        backends = BackendRegistry.getAvailableBackends()
        print(f"Available backends after import: {backends}")
        
except Exception as e:
    print(f"[ERROR] ❌ {e}")
    import traceback
    traceback.print_exc()

print("\n" + "=" * 60)
