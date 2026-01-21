# -*- coding: utf-8 -*-
"""
Diagnose sphere visibility issues
"""
import FreeCAD
import FreeCADGui

def diagnose_sphere():
    print("\n=== Sphere Visibility Diagnostics ===\n")
    
    # 1. Check current rendering backend
    try:
        backend = FreeCADGui.getMainWindow().activeWindow().getViewer().getBackendName()
        print(f"Current rendering backend: {backend}")
    except Exception as e:
        print(f"Cannot get rendering backend: {e}")
    
    # 2. Create a simple object
    print("\nCreating test object...")
    doc = FreeCAD.newDocument("SphereTest")
    box = doc.addObject("Part::Box", "TestBox")
    doc.recompute()
    
    # 3. Check ViewProvider
    print("\nChecking ViewProvider...")
    vp = FreeCADGui.ActiveDocument.getObject("TestBox")
    if vp:
        print(f"ViewProvider exists: {vp}")
        print(f"ViewProvider type: {type(vp)}")
        print(f"ViewProvider visibility: {vp.Visibility}")
    else:
        print("ViewProvider does not exist!")
    
    # 4. Check view
    print("\nChecking view...")
    view = FreeCADGui.activeDocument().activeView()
    if view:
        print(f"Active view: {view}")
        print(f"View type: {type(view)}")
        
        # Try different methods to get viewer info
        try:
            if hasattr(view, 'getViewer'):
                viewer = view.getViewer()
                print(f"Viewer: {viewer}")
        except Exception as e:
            print(f"Cannot get viewer: {e}")
        
        try:
            if hasattr(view, 'getSceneGraph'):
                sg = view.getSceneGraph()
                print(f"Scene graph: {sg}")
        except Exception as e:
            print(f"Cannot get scene graph: {e}")
    
    # 5. Fit view
    print("\nFitting view...")
    FreeCADGui.SendMsgToActiveView("ViewFit")
    
    print("\n=== Diagnostics Complete ===")
    print("Please check if you can see the sphere in 3D view")
    print("If not, check:")
    print("1. Camera position is correct")
    print("2. Sphere is within view range")
    print("3. Lighting is correctly set")
    print("4. Sphere size is appropriate (current radius 2.0)")

if __name__ == "__main__":
    diagnose_sphere()
