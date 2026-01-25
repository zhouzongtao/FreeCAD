"""
Simple test to import OsgVerseGui module
"""

print("=" * 60)
print("Testing OsgVerseGui Import")
print("=" * 60)

print("\n[Step 1] Importing OsgVerseGui module...")
try:
    import OsgVerseGui
    print("[OK] OsgVerseGui imported successfully")
except Exception as e:
    print(f"[ERROR] Failed to import: {e}")
    import traceback
    traceback.print_exc()
    exit(1)

print("\n[Step 2] Checking if backend is registered...")
try:
    from FreeCADGui import BackendRegistry
    backends = BackendRegistry.getAvailableBackends()
    print(f"Available backends: {backends}")
    
    if "OsgVerse" in backends:
        print("[SUCCESS] OsgVerse backend is registered!")
    else:
        print("[ERROR] OsgVerse backend is NOT registered")
        print("\nTrying to manually register...")
        
        # Try to manually trigger registration
        from OsgVerseGui import OsgVerseBackendFactory
        factory = OsgVerseBackendFactory()
        registered = BackendRegistry.instance().registerBackend(factory)
        
        if registered:
            print("[OK] Manual registration successful")
            backends = BackendRegistry.getAvailableBackends()
            print(f"Available backends now: {backends}")
        else:
            print("[ERROR] Manual registration failed")
            
except Exception as e:
    print(f"[ERROR] {e}")
    import traceback
    traceback.print_exc()

print("\n" + "=" * 60)
