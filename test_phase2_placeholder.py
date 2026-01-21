"""
Phase 2 临时测试 - 占位符渲染（不依赖 Part 模块）

由于 Part 模块链接问题，暂时测试占位符渲染是否正常工作
"""

import FreeCAD
import FreeCADGui
import time

print("=" * 60)
print("Phase 2 临时测试 - 占位符渲染")
print("=" * 60)

# 创建新文档
doc = FreeCAD.newDocument("Phase2PlaceholderTest")
print("\n✓ 创建文档")

# 创建一个简单对象（不是 Part 对象）
obj = doc.addObject("App::DocumentObjectGroup", "TestGroup")
doc.recompute()
print("✓ 创建测试对象")

# 显示文档
FreeCADGui.showMainWindow()
print("✓ 显示主窗口")

# 等待
time.sleep(1)

# 切换到 OsgVerse
print("\n切换到 OsgVerse 后端...")
try:
    FreeCADGui.switchRenderBackend(2)  # 2 = OsgVerse
    print("✓ 切换成功")
    
    time.sleep(1)
    
    print("\n" + "=" * 60)
    print("测试结果：")
    print("=" * 60)
    print("如果没有崩溃，说明基本功能正常")
    print("下一步需要解决 Part 模块依赖问题")
    print("=" * 60)
    
except Exception as e:
    print(f"✗ 切换失败: {e}")
    import traceback
    traceback.print_exc()

print("\n测试完成")
