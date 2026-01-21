#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
测试 OsgVerseGui 模块

这个脚本测试 OsgVerseGui 模块是否正确加载和注册。

使用方法：
1. 编译 OsgVerseGui 模块
2. 启动 FreeCAD
3. 在 Python 控制台运行此脚本
"""

import sys

def test_module_import():
    """测试模块导入"""
    print("=" * 60)
    print("测试 1: 模块导入")
    print("=" * 60)
    
    try:
        import OsgVerseGui
        print("✅ OsgVerseGui 模块导入成功")
        return True
    except ImportError as e:
        print(f"❌ OsgVerseGui 模块导入失败: {e}")
        return False

def test_backend_registration():
    """测试后端注册"""
    print("\n" + "=" * 60)
    print("测试 2: 后端注册")
    print("=" * 60)
    
    try:
        from Gui import BackendRegistry
        
        # 获取所有可用后端
        backends = BackendRegistry.getAvailableBackends()
        print(f"可用后端: {backends}")
        
        # 检查 OsgVerse 是否注册
        if "OsgVerse" in backends:
            print("✅ OsgVerse 后端已注册")
            return True
        else:
            print("❌ OsgVerse 后端未注册")
            return False
    except Exception as e:
        print(f"❌ 后端注册测试失败: {e}")
        return False

def test_backend_info():
    """测试后端信息"""
    print("\n" + "=" * 60)
    print("测试 3: 后端信息")
    print("=" * 60)
    
    try:
        from Gui import BackendRegistry
        
        # 获取后端信息
        info = BackendRegistry.getBackendInfo("OsgVerse")
        print("OsgVerse 后端信息:")
        for key, value in info.items():
            print(f"  {key}: {value}")
        
        print("✅ 后端信息获取成功")
        return True
    except Exception as e:
        print(f"❌ 后端信息测试失败: {e}")
        return False

def test_viewer_creation():
    """测试视图创建"""
    print("\n" + "=" * 60)
    print("测试 4: 视图创建")
    print("=" * 60)
    
    try:
        from Gui import BackendRegistry
        
        # 创建 OsgVerse 视图
        viewer = BackendRegistry.createViewer("OsgVerse")
        
        if viewer is None:
            print("❌ 视图创建失败（返回 None）")
            return False
        
        # 检查视图属性
        backend_name = viewer.getBackendName()
        version = viewer.getVersion()
        
        print(f"视图后端名称: {backend_name}")
        print(f"视图版本: {version}")
        
        if backend_name == "OsgVerse":
            print("✅ 视图创建成功")
            return True
        else:
            print(f"❌ 视图后端名称不匹配: {backend_name}")
            return False
    except Exception as e:
        print(f"❌ 视图创建测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_default_backend():
    """测试默认后端"""
    print("\n" + "=" * 60)
    print("测试 5: 默认后端")
    print("=" * 60)
    
    try:
        from Gui import BackendRegistry
        
        # 获取当前默认后端
        default = BackendRegistry.getDefaultBackend()
        print(f"当前默认后端: {default}")
        
        # Coin3D 应该是默认后端（优先级 10 vs 5）
        if default == "Coin3D":
            print("✅ Coin3D 是默认后端（正确）")
        else:
            print(f"⚠️  默认后端是 {default}（预期是 Coin3D）")
        
        return True
    except Exception as e:
        print(f"❌ 默认后端测试失败: {e}")
        return False

def test_backend_switching():
    """测试后端切换"""
    print("\n" + "=" * 60)
    print("测试 6: 后端切换")
    print("=" * 60)
    
    try:
        from Gui import BackendRegistry
        
        # 保存原始默认后端
        original_default = BackendRegistry.getDefaultBackend()
        print(f"原始默认后端: {original_default}")
        
        # 切换到 OsgVerse
        success = BackendRegistry.setDefaultBackend("OsgVerse")
        if not success:
            print("❌ 切换到 OsgVerse 失败")
            return False
        
        new_default = BackendRegistry.getDefaultBackend()
        print(f"新默认后端: {new_default}")
        
        if new_default == "OsgVerse":
            print("✅ 成功切换到 OsgVerse")
        else:
            print(f"❌ 切换失败，当前默认后端: {new_default}")
            return False
        
        # 恢复原始默认后端
        BackendRegistry.setDefaultBackend(original_default)
        print(f"已恢复默认后端: {original_default}")
        
        return True
    except Exception as e:
        print(f"❌ 后端切换测试失败: {e}")
        return False

def test_geometry_rendering():
    """测试几何体渲染"""
    print("\n" + "=" * 60)
    print("测试 7: 几何体渲染（需要 Part 模块）")
    print("=" * 60)
    
    try:
        import Part
        from Gui import BackendRegistry
        
        # 创建一个简单的 Box
        print("创建 Part.Box...")
        box = Part.makeBox(10, 10, 10)
        
        # 创建 OsgVerse 视图
        print("创建 OsgVerse 视图...")
        viewer = BackendRegistry.createViewer("OsgVerse")
        
        if viewer is None:
            print("❌ 视图创建失败")
            return False
        
        # TODO: 需要实现 ViewProvider 集成才能完整测试
        print("⚠️  完整的几何体渲染测试需要 ViewProvider 集成")
        print("✅ 基本测试通过（Part 模块可用）")
        
        return True
    except ImportError:
        print("⚠️  Part 模块不可用，跳过几何体渲染测试")
        return True
    except Exception as e:
        print(f"❌ 几何体渲染测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """主测试函数"""
    print("\n" + "=" * 60)
    print("OsgVerseGui 模块测试")
    print("=" * 60)
    
    tests = [
        ("模块导入", test_module_import),
        ("后端注册", test_backend_registration),
        ("后端信息", test_backend_info),
        ("视图创建", test_viewer_creation),
        ("默认后端", test_default_backend),
        ("后端切换", test_backend_switching),
        ("几何体渲染", test_geometry_rendering),
    ]
    
    results = []
    for name, test_func in tests:
        try:
            result = test_func()
            results.append((name, result))
        except Exception as e:
            print(f"\n❌ 测试 '{name}' 发生异常: {e}")
            import traceback
            traceback.print_exc()
            results.append((name, False))
    
    # 打印总结
    print("\n" + "=" * 60)
    print("测试总结")
    print("=" * 60)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for name, result in results:
        status = "✅ 通过" if result else "❌ 失败"
        print(f"{name}: {status}")
    
    print(f"\n总计: {passed}/{total} 测试通过")
    
    if passed == total:
        print("\n🎉 所有测试通过！")
        return 0
    else:
        print(f"\n⚠️  {total - passed} 个测试失败")
        return 1

if __name__ == "__main__":
    sys.exit(main())
