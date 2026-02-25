import sys
import traceback

print("=" * 80)
print("Testing OsgVerse Backend")
print("=" * 80)

try:
    print("\n1. Importing FreeCADGui...")
    import FreeCADGui
    print("   SUCCESS")
    
    print("\n2. Getting current backend...")
    backend = FreeCADGui.getCurrentRenderBackend()
    backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print(f"   Current backend: {backend_names.get(backend, 'Unknown')} (ID: {backend})")
    
    print("\n3. Getting renderer info...")
    info = FreeCADGui.getRendererInfo()
    print(f"   Renderer: {info}")
    
    print("\n4. Checking OsgVerse availability...")
    available = FreeCADGui.isRenderBackendAvailable(2)
    print(f"   OsgVerse available: {available}")
    
    if available:
        print("\n5. Switching to OsgVerse...")
        result = FreeCADGui.switchRenderBackend(2)
        print(f"   Switch result: {result}")
        
        if result:
            backend = FreeCADGui.getCurrentRenderBackend()
            print(f"   New backend: {backend_names.get(backend, 'Unknown')} (ID: {backend})")
            
            info = FreeCADGui.getRendererInfo()
            print(f"   New renderer: {info}")
            
            print("\n   ✓ OsgVerse backend is working!")
        else:
            print("\n   ✗ Failed to switch to OsgVerse")
    else:
        print("\n   ✗ OsgVerse is not available")
    
    print("\n" + "=" * 80)
    print("Test Complete")
    print("=" * 80)
    
except Exception as e:
    print(f"\n✗ Test failed: {e}")
    traceback.print_exc()
    sys.exit(1)
