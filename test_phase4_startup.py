#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Phase 4.1: 基础启动测试
Test FreeCAD startup with OsgVerse backend
"""

import sys
import os

print("=" * 70)
print("Phase 4.1: FreeCAD 基础启动测试")
print("=" * 70)

# 设置路径
freecad_bin = r"E:\Repository\FreeCAD\FreeCAD\build\bin"
if os.path.exists(freecad_bin):
    sys.path.insert(0, freecad_bin)
    os.chdir(freecad_bin)
    print(f"✓ 工作目录: {freecad_bin}")
else:
    print(f"✗ 目录不存在: {freecad_bin}")
    sys.exit(1)

print("\n" + "-" * 70)
print("步骤 1: 导入 FreeCAD 核心模块")
print("-" * 70)

try:
    import FreeCAD
    version = FreeCAD.Version()
    print(f"✓ FreeCAD 版本: {'.'.join(version[:3])}")
    print(f"  构建日期: {version[3]}")
    print(f"  Git 哈希: {version[4]}")
except Exception as e:
    print(f"✗ 导入 FreeCAD 失败: {e}")
    sys.exit(1)

print("\n" + "-" * 70)
print("步骤 2: 导入 FreeCADGui 模块")
print("-" * 70)

try:
    import FreeCADGui
    print("✓ FreeCADGui 模块导入成功")
except Exception as e:
    print(f"✗ 导入 FreeCADGui 失败: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

print("\n" + "-" * 70)
print("步骤 3: 检查渲染后端信息")
print("-" * 70)

# 尝试获取渲染后端信息（可能不可用，因为 Python API 未启用）
try:
    # 检查是否有 RenderManager 相关的 API
    if hasattr(FreeCADGui, 'getCurrentRenderBackend'):
        backend = FreeCADGui.getCurrentRenderBackend()
        print(f"✓ 当前渲染后端: {backend}")
    else:
        print("⚠ Python API 未启用，无法查询当前后端")
        print("  (这是预期的，因为 RenderManagerPy.cpp 未添加到构建系统)")
except Exception as e:
    print(f"⚠ 无法查询渲染后端: {e}")

print("\n" + "-" * 70)
print("步骤 4: 创建测试文档")
print("-" * 70)

try:
    doc = FreeCAD.newDocument("TestDoc")
    print(f"✓ 创建文档: {doc.Name}")
except Exception as e:
    print(f"✗ 创建文档失败: {e}")
    sys.exit(1)

print("\n" + "-" * 70)
print("步骤 5: 检查 3D 视图")
print("-" * 70)

try:
    # 注意：在无 GUI 模式下，这可能会失败
    if hasattr(FreeCADGui, 'ActiveDocument') and FreeCADGui.ActiveDocument:
        print("✓ ActiveDocument 可用")
        if hasattr(FreeCADGui.ActiveDocument, 'ActiveView'):
            view = FreeCADGui.ActiveDocument.ActiveView
            if view:
                print(f"✓ ActiveView 可用: {type(view)}")
            else:
                print("⚠ ActiveView 为 None（可能是无 GUI 模式）")
        else:
            print("⚠ 无 ActiveView 属性")
    else:
        print("⚠ 无 ActiveDocument（可能是无 GUI 模式）")
except Exception as e:
    print(f"⚠ 检查视图失败: {e}")

print("\n" + "=" * 70)
print("测试总结")
print("=" * 70)

print("""
✓ FreeCAD 核心模块正常
✓ FreeCADGui 模块正常
⚠ Python API 未启用（预期）

下一步：
1. 如果需要测试 GUI 功能，请直接运行 FreeCAD.exe
2. 如果需要切换到 OsgVerse，请修改 RenderEngine.h 中的默认后端
3. 如果需要 Python API，请完成 Phase 5
""")

print("=" * 70)
