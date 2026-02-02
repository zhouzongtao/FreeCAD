#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
测试OsgVerse光照修复效果
Test OsgVerse lighting fix
"""

import FreeCAD as App
import FreeCADGui as Gui
import Part

def test_lighting_fix():
    """测试光照修复效果"""
    
    print("=== 测试OsgVerse光照修复 ===")
    
    # 1. 切换到OsgVerse后端
    print("1. 切换到OsgVerse渲染后端...")
    try:
        # 获取当前活动视图
        view = Gui.ActiveDocument.ActiveView
        if hasattr(view, 'setRenderBackend'):
            view.setRenderBackend('OsgVerse')
            print("   ✓ 已切换到OsgVerse后端")
        else:
            print("   ⚠ 当前视图不支持后端切换")
    except Exception as e:
        print(f"   ✗ 切换后端失败: {e}")
    
    # 2. 创建测试文档
    print("2. 创建测试文档...")
    try:
        doc = App.newDocument("LightingTest")
        print("   ✓ 文档创建成功")
    except Exception as e:
        print(f"   ✗ 文档创建失败: {e}")
        return
    
    # 3. 创建测试几何体
    print("3. 创建测试几何体...")
    try:
        # 创建一个立方体
        box = doc.addObject("Part::Box", "TestBox")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        
        # 创建一个圆柱体
        cylinder = doc.addObject("Part::Cylinder", "TestCylinder")
        cylinder.Radius = 10
        cylinder.Height = 30
        cylinder.Placement.Base = App.Vector(30, 0, 0)
        
        # 创建一个球体
        sphere = doc.addObject("Part::Sphere", "TestSphere")
        sphere.Radius = 12
        sphere.Placement.Base = App.Vector(-30, 0, 0)
        
        doc.recompute()
        print("   ✓ 几何体创建成功: Box, Cylinder, Sphere")
    except Exception as e:
        print(f"   ✗ 几何体创建失败: {e}")
        return
    
    # 4. 调整视图
    print("4. 调整视图...")
    try:
        Gui.SendMsgToActiveView("ViewFit")
        print("   ✓ 视图已调整到合适大小")
        
        # 设置等轴测视图
        view = Gui.ActiveDocument.ActiveView
        if hasattr(view, 'viewIsometric'):
            view.viewIsometric()
            print("   ✓ 已设置为等轴测视图")
    except Exception as e:
        print(f"   ⚠ 视图调整失败: {e}")
    
    # 5. 检查渲染后端
    print("5. 检查当前渲染后端...")
    try:
        view = Gui.ActiveDocument.ActiveView
        if hasattr(view, 'getRenderBackend'):
            backend = view.getRenderBackend()
            print(f"   当前后端: {backend}")
        else:
            print("   ⚠ 无法获取后端信息")
    except Exception as e:
        print(f"   ⚠ 后端检查失败: {e}")
    
    print("\n=== 光照测试指南 ===")
    print("请手动检查以下光照效果:")
    print("1. 旋转视图，观察各个面的光照")
    print("2. 检查立方体的6个面是否都有适当的光照")
    print("3. 检查圆柱体侧面的光照是否均匀")
    print("4. 检查球体表面的光照过渡是否自然")
    print("5. 确认没有面过亮或过暗")
    print("\n预期效果:")
    print("✓ 顶面: 明亮（主光源）")
    print("✓ 侧面: 适中（填充光）") 
    print("✓ 底面: 可见（背光）")
    print("✓ 整体对比度柔和")
    print("✓ 立体感良好")
    
    print("\n=== 测试完成 ===")
    print("如果光照效果不理想，可以:")
    print("1. 尝试切换到Coin3D后端对比")
    print("2. 调整光照参数（需要重新编译）")
    print("3. 报告具体的光照问题")

if __name__ == "__main__":
    test_lighting_fix()