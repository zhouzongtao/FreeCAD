"""
测试 OsgVerse 后端切换
Test OsgVerse backend switching

这个脚本测试：
1. 当前后端状态（应该是 Coin3D）
2. 切换到 OsgVerse
3. 验证切换是否成功
"""

import FreeCAD as App
import FreeCADGui as Gui

def test_current_backend():
    """测试当前后端"""
    print("\n" + "="*60)
    print("测试 1: 检查当前后端")
    print("="*60)
    
    try:
        # 获取当前后端信息
        backend = Gui.RenderManager.getCurrentBackend()
        print(f"✓ 当前后端: {backend}")
        
        # 获取详细信息
        info = Gui.RenderManager.getBackendInfo()
        print(f"✓ 后端信息:")
        print(f"  - 名称: {info.get('name', 'N/A')}")
        print(f"  - 版本: {info.get('version', 'N/A')}")
        print(f"  - 描述: {info.get('description', 'N/A')}")
        
        return True
    except Exception as e:
        print(f"✗ 错误: {e}")
        return False

def test_available_backends():
    """测试可用的后端"""
    print("\n" + "="*60)
    print("测试 2: 检查可用后端")
    print("="*60)
    
    try:
        backends = ['Coin3D', 'OsgVerse']
        for backend in backends:
            available = Gui.RenderManager.isBackendAvailable(backend)
            status = "✓ 可用" if available else "✗ 不可用"
            print(f"{status}: {backend}")
        
        return True
    except Exception as e:
        print(f"✗ 错误: {e}")
        return False

def test_switch_to_osgverse():
    """测试切换到 OsgVerse"""
    print("\n" + "="*60)
    print("测试 3: 切换到 OsgVerse")
    print("="*60)
    
    try:
        # 检查 OsgVerse 是否可用
        if not Gui.RenderManager.isBackendAvailable('OsgVerse'):
            print("✗ OsgVerse 后端不可用")
            return False
        
        # 切换到 OsgVerse
        print("正在切换到 OsgVerse...")
        success = Gui.RenderManager.switchBackend('OsgVerse')
        
        if success:
            print("✓ 切换成功")
            
            # 验证切换
            current = Gui.RenderManager.getCurrentBackend()
            if current == 'OsgVerse':
                print(f"✓ 验证成功: 当前后端是 {current}")
                return True
            else:
                print(f"✗ 验证失败: 当前后端是 {current}，期望是 OsgVerse")
                return False
        else:
            print("✗ 切换失败")
            return False
            
    except Exception as e:
        print(f"✗ 错误: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """主测试函数"""
    print("\n" + "="*60)
    print("OsgVerse 后端切换测试")
    print("="*60)
    
    results = []
    
    # 测试 1: 当前后端
    results.append(("当前后端检查", test_current_backend()))
    
    # 测试 2: 可用后端
    results.append(("可用后端检查", test_available_backends()))
    
    # 测试 3: 切换到 OsgVerse
    results.append(("切换到 OsgVerse", test_switch_to_osgverse()))
    
    # 总结
    print("\n" + "="*60)
    print("测试总结")
    print("="*60)
    
    for name, result in results:
        status = "✓ 通过" if result else "✗ 失败"
        print(f"{status}: {name}")
    
    all_passed = all(result for _, result in results)
    print("\n" + "="*60)
    if all_passed:
        print("✓ 所有测试通过！")
    else:
        print("✗ 部分测试失败")
    print("="*60)

if __name__ == "__main__":
    main()
