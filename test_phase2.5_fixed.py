# -*- coding: utf-8 -*-
"""
测试 Phase 2.5: View3DOsgVerse 类型系统修复
"""

import FreeCAD
import FreeCADGui

print("=" * 60)
print("测试 Phase 2.5: View3DOsgVerse 类型系统修复")
print("=" * 60)

try:
    # 1. 切换到 OsgVerse
    print("\n1. 切换到 OsgVerse 后端...")
    FreeCADGui.switchRenderBackend(2)
    backend = FreeCADGui.getCurrentRenderBackend()
    print(f"当前后端: {backend}")
    
    if backend != 2:
        print("✗ 切换失败，当前不是 OsgVerse 后端")
    else:
        print("✓ 成功切换到 OsgVerse")
    
    # 2. 创建文档
    print("\n2. 创建新文档...")
    doc = FreeCAD.newDocument("TestView3DOsgVerse")
    print(f"✓ 文档创建成功: {doc.Name}")
    
    # 3. 创建对象
    print("\n3. 创建测试对象...")
    box = doc.addObject("Part::Box", "Box")
    box.Length = 10
    box.Width = 10
    box.Height = 10
    doc.recompute()
    print(f"✓ 立方体创建成功")
    
    # 4. 获取视图
    print("\n4. 获取 3D 视图...")
    gui_doc = FreeCADGui.getDocument(doc.Name)
    if gui_doc is None:
        print("✗ 无法获取 GUI 文档")
    else:
        view = gui_doc.activeView()
        if view is None:
            print("✗ 无法获取活动视图")
        else:
            print(f"✓ 视图类型: {type(view)}")
            print(f"✓ 视图名称: {view}")
            
            # 5. 适应视图
            print("\n5. 适应视图...")
            FreeCADGui.SendMsgToActiveView("ViewFit")
            print("✓ 视图适应完成")
    
    print("\n" + "=" * 60)
    print("✓ 测试完成！")
    print("=" * 60)
    print("\n请检查 3D 视图是否显示立方体（不应该是黑屏）")
    
except Exception as e:
    print(f"\n✗ 测试失败: {e}")
    import traceback
    traceback.print_exc()
