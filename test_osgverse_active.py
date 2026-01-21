#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Phase 4.2: 测试 OsgVerse 后端是否激活
Test if OsgVerse backend is active
"""

print("=" * 70)
print("Phase 4.2: OsgVerse 后端激活测试")
print("=" * 70)
print()
print("说明：此脚本需要在 FreeCAD 的 Python 控制台中运行")
print("或者使用 FreeCAD 内置的 Python 解释器")
print()
print("请将以下代码复制到 FreeCAD 的 Python 控制台中运行：")
print()
print("-" * 70)
print("""
# 检查渲染后端
import FreeCAD
import FreeCADGui

print("FreeCAD 版本:", FreeCAD.Version())

# 尝试获取渲染后端信息
# 注意：如果 Python API 未启用，这些函数可能不存在
if hasattr(FreeCADGui, 'getCurrentRenderBackend'):
    backend = FreeCADGui.getCurrentRenderBackend()
    print("当前渲染后端:", backend)
else:
    print("Python API 未启用")
    print("默认后端已在编译时设置为 OsgVerse")

# 创建测试文档和对象
doc = FreeCAD.newDocument("TestOsgVerse")
print("创建文档:", doc.Name)

# 导入 Part 模块
import Part

# 创建一个立方体
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
print("创建立方体:", box.Name)

# 刷新文档
doc.recompute()
print("文档已刷新")

# 适应视图
FreeCADGui.SendMsgToActiveView("ViewFit")
print("视图已适应")

print()
print("如果看到立方体正确渲染，说明 OsgVerse 后端工作正常！")
print("请检查：")
print("1. 立方体是否可见？")
print("2. 可以用鼠标旋转视图吗？")
print("3. 可以用滚轮缩放吗？")
print("4. 控制台是否有 OsgVerse 相关的日志？")
""")
print("-" * 70)
print()
print("=" * 70)
