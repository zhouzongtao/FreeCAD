#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
快速测试 BackendRegistry 和 OsgVerseGui 模块

在 FreeCAD Python 控制台中运行：
exec(open('test_backend_registry_quick.py', encoding='utf-8').read())
"""

print("=" * 60)
print("快速测试：BackendRegistry 和 OsgVerseGui")
print("=" * 60)

# 步骤 1: 导入 Gui 模块
print("\n步骤 1: 导入 Gui 模块...")
try:
    import Gui
    print("✅ Gui 模块导入成功")
except ImportError as e:
    print(f"❌ Gui 模块导入失败: {e}")
    print("提示：确保在 FreeCAD 中运行此脚本")
    exit(1)

# 步骤 2: 访问 BackendRegistry
print("\n步骤 2: 访问 BackendRegistry...")
try:
    from Gui import BackendRegistry
    print("✅ BackendRegistry 导入成功")
except ImportError as e:
    print(f"❌ BackendRegistry 导入失败: {e}")
    print("提示：BackendRegistry 可能未编译到 FreeCADGui 中")
    exit(1)

# 步骤 3: 获取可用后端
print("\n步骤 3: 获取可用后端...")
try:
    backends = BackendRegistry.getAvailableBackends()
    print(f"✅ 可用后端: {backends}")
    
    if "Coin3D" in backends:
        print("  ✅ Coin3D 后端已注册")
    else:
        print("  ⚠️  Coin3D 后端未注册")
    
    if "OsgVerse" in backends:
        print("  ✅ OsgVerse 后端已注册")
    else:
        print("  ⚠️  OsgVerse 后端未注册（可能模块未加载）")
except Exception as e:
    print(f"❌ 获取后端列表失败: {e}")
    import traceback
    traceback.print_exc()
    exit(1)

# 步骤 4: 尝试导入 OsgVerseGui
print("\n步骤 4: 导入 OsgVerseGui 模块...")
try:
    import OsgVerseGui
    print("✅ OsgVerseGui 模块导入成功")
except ImportError as e:
    print(f"❌ OsgVerseGui 模块导入失败: {e}")
    print("提示：")
    print("  1. 检查 build/Mod/OsgVerseGui/OsgVerseGui.pyd 是否存在")
    print("  2. 检查 Python 路径")
    import sys
    print(f"  Python 路径: {sys.path[:3]}...")
    exit(1)

# 步骤 5: 再次检查后端（OsgVerseGui 加载后）
print("\n步骤 5: 再次检查可用后端...")
try:
    backends = BackendRegistry.getAvailableBackends()
    print(f"✅ 可用后端: {backends}")
    
    if "OsgVerse" in backends:
        print("  ✅ OsgVerse 后端已注册（模块加载成功）")
    else:
        print("  ❌ OsgVerse 后端仍未注册（模块初始化可能失败）")
except Exception as e:
    print(f"❌ 获取后端列表失败: {e}")

# 步骤 6: 获取默认后端
print("\n步骤 6: 获取默认后端...")
try:
    default = BackendRegistry.getDefaultBackend()
    print(f"✅ 默认后端: {default}")
except Exception as e:
    print(f"❌ 获取默认后端失败: {e}")

# 步骤 7: 获取 OsgVerse 后端信息
print("\n步骤 7: 获取 OsgVerse 后端信息...")
try:
    if BackendRegistry.isBackendAvailable("OsgVerse"):
        info = BackendRegistry.getBackendInfo("OsgVerse")
        print("✅ OsgVerse 后端信息:")
        for key, value in info.items():
            print(f"  {key}: {value}")
    else:
        print("⚠️  OsgVerse 后端不可用")
except Exception as e:
    print(f"❌ 获取后端信息失败: {e}")

# 步骤 8: 尝试创建 OsgVerse 视图
print("\n步骤 8: 创建 OsgVerse 视图...")
try:
    if BackendRegistry.isBackendAvailable("OsgVerse"):
        viewer = BackendRegistry.createViewer("OsgVerse")
        if viewer:
            print(f"✅ 视图创建成功")
            print(f"  后端名称: {viewer.getBackendName()}")
            print(f"  版本: {viewer.getVersion()}")
        else:
            print("❌ 视图创建失败（返回 None）")
    else:
        print("⚠️  跳过（OsgVerse 后端不可用）")
except Exception as e:
    print(f"❌ 视图创建失败: {e}")
    import traceback
    traceback.print_exc()

print("\n" + "=" * 60)
print("测试完成")
print("=" * 60)
