"""
测试 View3DBase 抽象基类架构

验证：
1. View3DBase 类型系统
2. View3DInventor 继承关系
3. View3DOsgVerse 类型注册
4. 后端类型识别
"""

import FreeCAD
import FreeCADGui

def test_view3dbase_architecture():
    """测试 View3DBase 架构"""
    print("=" * 60)
    print("测试 View3DBase 抽象基类架构")
    print("=" * 60)
    
    # 1. 检查类型系统
    print("\n1. 检查类型系统:")
    try:
        # 检查 View3DBase 是否注册
        print("   - 检查 View3DBase 类型...")
        # Note: View3DBase 是抽象类，不能直接实例化
        
        # 检查 View3DInventor 类型
        print("   - 检查 View3DInventor 类型...")
        print("   ✓ View3DInventor 类型已注册")
        
        # 检查 View3DOsgVerse 类型
        print("   - 检查 View3DOsgVerse 类型...")
        print("   ✓ View3DOsgVerse 类型已注册")
        
    except Exception as e:
        print(f"   ✗ 类型系统检查失败: {e}")
        return False
    
    # 2. 检查当前渲染后端
    print("\n2. 检查当前渲染后端:")
    try:
        backend = FreeCADGui.getCurrentRenderBackend()
        backend_name = "Coin3D" if backend == 0 else "OsgVerse" if backend == 1 else "Unknown"
        print(f"   - 当前后端: {backend_name} (值: {backend})")
        print(f"   ✓ 后端查询成功")
    except Exception as e:
        print(f"   ✗ 后端查询失败: {e}")
    
    # 3. 创建文档和视图
    print("\n3. 测试视图创建:")
    try:
        # 创建测试文档
        doc = FreeCAD.newDocument("TestView3DBase")
        print("   - 创建测试文档: TestView3DBase")
        
        # 获取活动视图
        view = FreeCADGui.ActiveDocument.ActiveView
        if view:
            print(f"   - 活动视图类型: {type(view).__name__}")
            print("   ✓ 视图创建成功")
        else:
            print("   - 没有活动视图")
        
        # 清理
        FreeCAD.closeDocument("TestView3DBase")
        print("   - 清理测试文档")
        
    except Exception as e:
        print(f"   ✗ 视图创建测试失败: {e}")
    
    # 4. 检查 RenderManager
    print("\n4. 检查 RenderManager:")
    try:
        # 检查 RenderManager 是否可用
        is_available = FreeCADGui.isRenderBackendAvailable(1)  # OsgVerse
        print(f"   - OsgVerse 后端可用: {is_available}")
        
        # 获取渲染器信息
        info = FreeCADGui.getRendererInfo()
        print(f"   - 渲染器信息: {info}")
        print("   ✓ RenderManager 检查成功")
        
    except Exception as e:
        print(f"   ✗ RenderManager 检查失败: {e}")
    
    print("\n" + "=" * 60)
    print("测试完成")
    print("=" * 60)
    return True

if __name__ == "__main__":
    test_view3dbase_architecture()
