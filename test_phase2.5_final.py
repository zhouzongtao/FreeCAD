# -*- coding: utf-8 -*-
"""
Phase 2.5 最终测试 - 验证清理后的代码
"""

import time

def test_phase2_5_final():
    """测试 Phase 2.5 的最终状态"""
    
    print("=" * 60)
    print("Phase 2.5 最终测试")
    print("=" * 60)
    
    try:
        import FreeCAD
        import FreeCADGui
        
        print("\n1. 检查后端...")
        backend = FreeCADGui.getRenderBackend()
        print(f"   当前后端: {backend}")
        
        if backend != 2:
            print("   切换到 OsgVerse...")
            FreeCADGui.setRenderBackend(2)
            backend = FreeCADGui.getRenderBackend()
            print(f"   ✓ 后端: {backend}")
        
        print("\n2. 创建新文档...")
        doc = FreeCAD.newDocument("Phase2_5_Final")
        print(f"   ✓ 文档: {doc.Name}")
        
        print("\n3. 等待视图创建...")
        time.sleep(1)
        
        view = FreeCADGui.activeView()
        if view:
            print(f"   ✓ 视图类型: {type(view).__name__}")
            
            try:
                backend_name = view.getBackendName()
                backend_version = view.getBackendVersion()
                print(f"   ✓ Backend: {backend_name}")
                print(f"   ✓ Version: {backend_version}")
            except:
                pass
        
        print("\n" + "=" * 60)
        print("Phase 2.5 最终测试完成")
        print("=" * 60)
        print("\n✅ 预期结果:")
        print("1. 深蓝灰色背景填充整个窗口")
        print("2. 没有黑色区域")
        print("3. 没有测试球体（已清理）")
        print("4. 鼠标拖动和滚轮工作正常")
        print("\n准备进入 Phase 3: ViewProvider 集成")
        
    except Exception as e:
        print(f"\n❌ 错误: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    return True

if __name__ == "__main__":
    test_phase2_5_final()
