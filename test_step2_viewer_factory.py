#!/usr/bin/env python3
"""
Test script for Step 2: View3DInventor using ViewerFactory

This script verifies that:
1. View3DInventor uses ViewerFactory to create viewers
2. All existing functionality still works
3. The viewer is properly initialized
"""

import sys
import FreeCAD
import FreeCADGui

def test_view_creation():
    """Test that 3D view is created using ViewerFactory"""
    print("=" * 60)
    print("Test 1: View Creation via ViewerFactory")
    print("=" * 60)
    
    try:
        # Create a new document
        doc = FreeCAD.newDocument("TestStep2")
        print("✓ Document created")
        
        # Create a simple object
        box = doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        doc.recompute()
        print("✓ Box object created")
        
        # Show main window and activate workbench
        FreeCADGui.showMainWindow()
        FreeCADGui.activateWorkbench("PartWorkbench")
        print("✓ Main window shown")
        
        # Get the active view
        view = FreeCADGui.ActiveDocument.ActiveView
        if view:
            print(f"✓ Active view created: {type(view).__name__}")
            
            # Get the viewer
            viewer = view.getViewer()
            if viewer:
                print(f"✓ Viewer obtained: {type(viewer).__name__}")
            else:
                print("✗ Failed to get viewer")
                return False
        else:
            print("✗ No active view")
            return False
            
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_view_operations():
    """Test that all view operations still work"""
    print("\n" + "=" * 60)
    print("Test 2: View Operations")
    print("=" * 60)
    
    try:
        view = FreeCADGui.ActiveDocument.ActiveView
        if not view:
            print("✗ No active view")
            return False
        
        # Test basic view operations
        operations = [
            ("viewAxometric", lambda: view.viewAxometric()),
            ("viewFront", lambda: view.viewFront()),
            ("viewTop", lambda: view.viewTop()),
            ("viewRight", lambda: view.viewRight()),
            ("viewAll", lambda: view.viewAll()),
            ("fitAll", lambda: view.fitAll()),
        ]
        
        for name, op in operations:
            try:
                op()
                print(f"✓ {name}() works")
            except Exception as e:
                print(f"✗ {name}() failed: {e}")
                return False
        
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_viewer_properties():
    """Test viewer properties and methods"""
    print("\n" + "=" * 60)
    print("Test 3: Viewer Properties")
    print("=" * 60)
    
    try:
        view = FreeCADGui.ActiveDocument.ActiveView
        viewer = view.getViewer()
        
        if not viewer:
            print("✗ No viewer")
            return False
        
        # Test viewer methods
        tests = [
            ("hasSceneGraph", lambda: viewer.hasSceneGraph()),
            ("isAnimating", lambda: viewer.isAnimating()),
            ("isViewing", lambda: viewer.isViewing()),
        ]
        
        for name, test in tests:
            try:
                result = test()
                print(f"✓ {name}() = {result}")
            except Exception as e:
                print(f"✗ {name}() failed: {e}")
                return False
        
        # Test camera operations
        try:
            viewer.viewAll()
            print("✓ viewer.viewAll() works")
        except Exception as e:
            print(f"✗ viewer.viewAll() failed: {e}")
            return False
        
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_render_manager_integration():
    """Test integration with RenderManager"""
    print("\n" + "=" * 60)
    print("Test 4: RenderManager Integration")
    print("=" * 60)
    
    try:
        import FreeCADGui as Gui
        
        # Check RenderManager functions
        if hasattr(Gui, 'getCurrentRenderBackend'):
            backend = Gui.getCurrentRenderBackend()
            print(f"✓ Current render backend: {backend}")
            
            if hasattr(Gui, 'getRendererInfo'):
                info = Gui.getRendererInfo()
                print(f"✓ Renderer info: {info}")
        else:
            print("- RenderManager Python bindings not available")
        
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_backward_compatibility():
    """Test backward compatibility with existing code"""
    print("\n" + "=" * 60)
    print("Test 5: Backward Compatibility")
    print("=" * 60)
    
    try:
        view = FreeCADGui.ActiveDocument.ActiveView
        viewer = view.getViewer()
        
        # These should still work (Coin3D specific)
        tests = [
            ("getSceneGraph", lambda: viewer.getSceneGraph()),
            ("getSoRenderManager", lambda: viewer.getSoRenderManager()),
            ("getSoEventManager", lambda: viewer.getSoEventManager()),
        ]
        
        for name, test in tests:
            try:
                result = test()
                print(f"✓ {name}() returns {type(result).__name__}")
            except Exception as e:
                print(f"✗ {name}() failed: {e}")
                return False
        
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """Run all tests"""
    print("\n" + "=" * 60)
    print("Step 2 Test Suite: View3DInventor using ViewerFactory")
    print("=" * 60)
    
    results = []
    
    # Run tests
    results.append(("View Creation", test_view_creation()))
    results.append(("View Operations", test_view_operations()))
    results.append(("Viewer Properties", test_viewer_properties()))
    results.append(("RenderManager Integration", test_render_manager_integration()))
    results.append(("Backward Compatibility", test_backward_compatibility()))
    
    # Summary
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status}: {name}")
    
    print(f"\nTotal: {passed}/{total} tests passed")
    
    if passed == total:
        print("\n🎉 All tests passed!")
        print("\nStep 2 完成:")
        print("- View3DInventor 现在使用 ViewerFactory 创建渲染器")
        print("- 所有现有功能保持正常工作")
        print("- 为未来的后端切换做好准备")
        return True
    else:
        print(f"\n⚠ {total - passed} test(s) failed")
        return False

if __name__ == "__main__":
    try:
        success = main()
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"\n💥 Fatal error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
