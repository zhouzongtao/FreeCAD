# -*- coding: utf-8 -*-
"""
测试视口修复 - devicePixelRatio 方案
"""

import sys
import time

def test_viewport_fix():
    """测试视口是否正确填充整个窗口"""
    
    print("=" * 60)
    print("测试视口修复 (devicePixelRatio)")
    print("=" * 60)
    
    try:
        import FreeCAD
        import FreeCADGui
        
        print("\n1. 检查当前后端...")
        backend = FreeCADGui.getRenderBackend()
        print(f"   当前后端: {backend}")
        
        if backend != 2:
            print("   ⚠️ 当前不是 OsgVerse 后端，切换中...")
            FreeCADGui.setRenderBackend(2)
            print("   ✓ 已切换到 OsgVerse")
        
        print("\n2. 创建新文档...")
        doc = FreeCAD.newDocument("ViewportTest")
        print(f"   ✓ 文档创建: {doc.Name}")
        
        print("\n3. 等待视图创建...")
        time.sleep(1)
        
        # 获取活动视图
        view = FreeCADGui.activeView()
        if view:
            print(f"   ✓ 活动视图: {type(view).__name__}")
            
            # 获取视图信息
            try:
                backend_name = view.getBackendName()
                backend_version = view.getBackendVersion()
                print(f"   Backend: {backend_name}")
                print(f"   Version: {backend_version}")
            except:
                print("   (无法获取后端信息)")
        else:
            print("   ⚠️ 没有活动视图")
        
        print("\n4. 创建测试对象...")
        # 不创建对象，只测试空场景的视口
        print("   (跳过 - 只测试空场景)")
        
        print("\n" + "=" * 60)
        print("测试完成")
        print("=" * 60)
        print("\n请检查视图窗口:")
        print("1. 蓝灰色背景是否填充整个窗口？")
        print("2. 是否还有黑色区域？")
        print("3. 绿色球体是否完整显示（没有被裁剪）？")
        print("4. 拖动鼠标旋转视图，检查渲染区域是否正确")
        print("\n如果视口仍然不正确，请报告问题。")
        
    except Exception as e:
        print(f"\n❌ 错误: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    return True

if __name__ == "__main__":
    test_viewport_fix()
