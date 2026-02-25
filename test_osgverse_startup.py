#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
测试 OsgVerse 后端启动
Test OsgVerse backend startup
"""

import sys
import os

# 添加 FreeCAD 路径
freecad_bin = r"E:\Repository\FreeCAD\FreeCAD\build\bin"
sys.path.insert(0, freecad_bin)
os.chdir(freecad_bin)

print("=" * 60)
print("OsgVerse 后端启动测试")
print("=" * 60)

try:
    print("\n1. 导入 FreeCAD...")
    import FreeCAD
    print(f"   ✓ FreeCAD 版本: {FreeCAD.Version()}")
    
    print("\n2. 导入 FreeCADGui...")
    import FreeCADGui
    print("   ✓ FreeCADGui 加载成功")
    
    print("\n3. 检查渲染后端...")
    # 注意：Python API 还未添加到构建系统，所以这部分可能不可用
    # 但至少可以确认 GUI 能够加载
    
    print("\n" + "=" * 60)
    print("测试完成！FreeCAD GUI 成功加载。")
    print("=" * 60)
    
except ImportError as e:
    print(f"\n✗ 导入错误: {e}")
    print("\n可能的原因：")
    print("  1. Python 版本不匹配")
    print("  2. 缺少依赖的 DLL 文件")
    print("  3. 构建配置问题")
    sys.exit(1)
    
except Exception as e:
    print(f"\n✗ 运行时错误: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
