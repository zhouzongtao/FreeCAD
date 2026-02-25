"""
Phase 1.5 优化后的测试脚本

测试优化后的代码是否正常工作：
1. 创建测试对象
2. 验证红色球体占位符显示
3. 验证相机控制
4. 验证日志输出
"""

import FreeCAD
import FreeCADGui
import Part

print("=" * 60)
print("Phase 1.5 优化测试")
print("=" * 60)

# 创建新文档
doc = FreeCAD.newDocument("Phase1.5Test")
print("✓ 文档创建成功")

# 创建测试对象
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
print(f"✓ 创建对象: {box.Name}")

# 重新计算
doc.recompute()
print("✓ 文档重新计算完成")

# 获取 ViewProvider
vp = box.ViewObject
print(f"✓ ViewProvider: {vp}")
print(f"  - 可见性: {vp.Visibility}")

# 获取活动视图
view = FreeCADGui.ActiveDocument.ActiveView
print(f"✓ 活动视图: {view}")

# 适应视图
FreeCADGui.SendMsgToActiveView("ViewFit")
print("✓ 视图已适应对象")

print("\n" + "=" * 60)
print("测试完成！")
print("=" * 60)
print("\n预期结果：")
print("1. 窗口中应该显示一个红色球体（占位符）")
print("2. 球体应该完整可见，没有被裁剪")
print("3. Report View 中应该有清晰的日志输出")
print("4. 日志应该带有 [OsgVerse] 前缀")
print("\n如果看到红色球体，说明优化成功！")
