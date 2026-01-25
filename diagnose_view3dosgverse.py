#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
诊断 View3DOsgVerse 黑屏问题
"""

import FreeCAD
import FreeCADGui

print("=" * 60)
print("诊断 View3DOsgVerse")
print("=" * 60)

# 1. 检查后端
print("\n1. 当前后端:")
backend = FreeCADGui.getCurrentRenderBackend()
renderer = FreeCADGui.getRendererInfo()
print(f"   Backend: {backend}")
print(f"   Renderer: {renderer}")

# 2. 检查活动视图
print("\n2. 活动视图:")
try:
    view = FreeCADGui.activeDocument().activeView()
    if view:
        print(f"   Type: {type(view).__name__}")
        print(f"   Widget: {view.getWidget()}")
        
        # 尝试获取 viewer
        if hasattr(view, 'getViewerInterface'):
            viewer = view.getViewerInterface()
            if viewer:
                print(f"   Viewer: {viewer}")
                print(f"   Backend: {viewer.getBackendName()}")
                
                # 获取 widget
                widget = viewer.getWidget()
                print(f"   GL Widget: {widget}")
                print(f"   Widget size: {widget.width()}x{widget.height()}")
                print(f"   Widget visible: {widget.isVisible()}")
                
                # 检查场景
                scene = viewer.getSceneGraph()
                print(f"   Scene: {scene}")
                
                # 检查相机
                camera = viewer.getCamera()
                print(f"   Camera position: {camera.position}")
                print(f"   Camera target: {camera.target}")
                
    else:
        print("   No active view")
except Exception as e:
    print(f"   Error: {e}")
    import traceback
    traceback.print_exc()

# 3. 检查文档对象
print("\n3. 文档对象:")
doc = FreeCAD.ActiveDocument
if doc:
    print(f"   Document: {doc.Name}")
    print(f"   Objects: {len(doc.Objects)}")
    for obj in doc.Objects:
        print(f"     - {obj.Name} ({obj.TypeId})")
else:
    print("   No active document")

# 4. 创建测试对象
print("\n4. 创建测试对象...")
if not doc:
    doc = FreeCAD.newDocument("DiagnoseTest")
    print(f"   Created document: {doc.Name}")

try:
    import Part
    
    # 创建一个大的红色球体
    sphere = doc.addObject("Part::Sphere", "TestSphere")
    sphere.Radius = 50
    sphere.Placement.Base = FreeCAD.Vector(0, 0, 0)
    
    doc.recompute()
    print(f"   ✓ Created sphere: radius={sphere.Radius}")
    
    # 适应视图
    FreeCADGui.SendMsgToActiveView("ViewFit")
    print("   ✓ ViewFit sent")
    
    # 再次检查视图
    view = FreeCADGui.activeDocument().activeView()
    if view and hasattr(view, 'getViewerInterface'):
        viewer = view.getViewerInterface()
        if viewer:
            camera = viewer.getCamera()
            print(f"   Camera after ViewFit:")
            print(f"     Position: {camera.position}")
            print(f"     Target: {camera.target}")
            
            # 检查 ViewProviders
            vps = viewer.getViewProviders()
            print(f"   ViewProviders: {len(vps)}")
            
except Exception as e:
    print(f"   ✗ Error: {e}")
    import traceback
    traceback.print_exc()

print("\n" + "=" * 60)
print("诊断完成")
print("=" * 60)
print("\n如果看到黑屏，可能的原因:")
print("1. 场景中没有对象")
print("2. 相机位置不正确")
print("3. OpenGL 上下文问题")
print("4. ViewProvider 没有正确添加到场景")
print("\n请检查 Report View 中的详细日志")
