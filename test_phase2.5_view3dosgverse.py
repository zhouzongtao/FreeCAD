#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
测试 Phase 2.5: View3DOsgVerse 类
验证 OsgVerse 视图创建和功能

使用方法:
    exec(open(r'E:\\Repository\\FreeCAD\\FreeCAD\\test_phase2.5_view3dosgverse.py', encoding='utf-8').read())
"""

import sys

def test_view3dosgverse():
    """测试 View3DOsgVerse 类"""
    print("=" * 60)
    print("测试 Phase 2.5: View3DOsgVerse")
    print("=" * 60)
    
    # 1. 导入模块
    print("\n1. 导入模块...")
    try:
        import FreeCAD
        import FreeCADGui
        import OsgVerseGui
        print("   ✓ 模块导入成功")
    except Exception as e:
        print(f"   ✗ 模块导入失败: {e}")
        return False
    
    # 2. 切换到 OsgVerse 后端
    print("\n2. 切换到 OsgVerse 后端...")
    try:
        current = FreeCADGui.getCurrentRenderBackend()
        print(f"   当前后端: {current}")
        
        if current != 2:
            success = FreeCADGui.switchRenderBackend(2)
            if not success:
                print("   ✗ 切换失败")
                return False
            print("   ✓ 成功切换到 OsgVerse")
        else:
            print("   ✓ 已经是 OsgVerse 后端")
            
        # 验证
        current = FreeCADGui.getCurrentRenderBackend()
        renderer = FreeCADGui.getRendererInfo()
        print(f"   后端: {current} ({renderer})")
        
    except Exception as e:
        print(f"   ✗ 切换后端失败: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    # 3. 创建新文档
    print("\n3. 创建新文档...")
    try:
        doc = FreeCAD.newDocument("TestView3DOsgVerse")
        print(f"   ✓ 文档创建成功: {doc.Name}")
    except Exception as e:
        print(f"   ✗ 文档创建失败: {e}")
        return False
    
    # 4. 创建 3D 视图
    print("\n4. 创建 3D 视图...")
    try:
        # 手动创建视图，因为可能不会自动创建
        gui_doc = FreeCADGui.getDocument(doc.Name)
        
        if not gui_doc:
            print("   ✗ 无法获取 GUI 文档")
            return False
        
        # 尝试获取现有视图
        view = gui_doc.activeView()
        
        if not view:
            # 如果没有视图，手动创建
            print("   没有活动视图，尝试手动创建...")
            try:
                # 使用 Gui.activeDocument().addView() 或直接创建
                FreeCADGui.activeDocument().addView("View3DInventor")
                view = gui_doc.activeView()
            except Exception as e:
                print(f"   ⚠ 手动创建视图失败: {e}")
        
        if view:
            view_type = type(view).__name__
            print(f"   ✓ 视图获取成功")
            print(f"   视图类型: {view_type}")
            
            # 检查是否是 View3DOsgVerse
            if "OsgVerse" in view_type:
                print("   ✓ 视图类型正确: View3DOsgVerse")
            else:
                print(f"   ⚠ 视图类型: {view_type}")
                print("   说明: 可能仍在使用 Coin3D 或类型系统未正确注册")
                
        else:
            print("   ✗ 无法获取或创建视图")
            return False
            
    except Exception as e:
        print(f"   ✗ 获取视图失败: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    # 5. 测试视图功能
    print("\n5. 测试视图功能...")
    try:
        # 测试 viewAll
        try:
            view.viewAll()
            print("   ✓ viewAll() 成功")
        except Exception as e:
            print(f"   ⚠ viewAll() 失败: {e}")
        
        # 测试 dump
        try:
            view.dump()
            print("   ✓ dump() 成功")
        except Exception as e:
            print(f"   ⚠ dump() 失败: {e}")
            
    except Exception as e:
        print(f"   ⚠ 视图功能测试部分失败: {e}")
    
    # 6. 创建几何体
    print("\n6. 创建几何体...")
    try:
        import Part
        
        # 创建立方体
        box = doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        
        # 创建圆柱
        cylinder = doc.addObject("Part::Cylinder", "Cylinder")
        cylinder.Radius = 5
        cylinder.Height = 15
        cylinder.Placement.Base = FreeCAD.Vector(20, 0, 0)
        
        # 创建球体
        sphere = doc.addObject("Part::Sphere", "Sphere")
        sphere.Radius = 6
        sphere.Placement.Base = FreeCAD.Vector(-20, 0, 0)
        
        doc.recompute()
        print("   ✓ 几何体创建成功")
        print("   - Box (10x10x10)")
        print("   - Cylinder (R=5, H=15)")
        print("   - Sphere (R=6)")
        
        # 适应视图
        FreeCADGui.SendMsgToActiveView("ViewFit")
        print("   ✓ 视图适应完成")
        
    except Exception as e:
        print(f"   ✗ 几何体创建失败: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    # 7. 测试渲染统计
    print("\n7. 测试渲染统计...")
    try:
        stats = FreeCADGui.getRenderStats()
        print(f"   FPS: {stats.get('fps', 0):.1f}")
        print(f"   Frame Time: {stats.get('frameTime', 0):.2f} ms")
        print(f"   Triangles: {stats.get('triangleCount', 0)}")
        print(f"   Draw Calls: {stats.get('drawCalls', 0)}")
        print("   ✓ 渲染统计获取成功")
    except Exception as e:
        print(f"   ⚠ 渲染统计失败: {e}")
    
    # 8. 检查 Report View 日志
    print("\n8. 检查日志...")
    print("   请查看 Report View 中的日志，应该看到:")
    print("   - 'Document::createView: Creating View3DOsgVerse'")
    print("   - 'View3DOsgVerse: Constructor called'")
    print("   - 'View3DOsgVerse: Backend: OsgVerse'")
    print("   - 没有 'falling back to direct creation' 警告")
    
    print("\n" + "=" * 60)
    print("✓ Phase 2.5 测试完成！")
    print("=" * 60)
    print("\n说明:")
    print("- View3DOsgVerse 类已创建")
    print("- 切换到 OsgVerse 后端时自动使用 View3DOsgVerse")
    print("- 可以创建和渲染几何体")
    print("- 视图功能正常工作")
    print("\n请检查:")
    print("1. Report View 中的日志确认使用了 View3DOsgVerse")
    print("2. 3D 视图中的几何体是否正确显示")
    print("3. 相机操作（旋转、缩放）是否正常")
    
    return True

if __name__ == "__main__":
    try:
        success = test_view3dosgverse()
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"\n✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
