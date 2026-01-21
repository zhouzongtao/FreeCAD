"""
Phase 2 Step 1 测试脚本 - 真实几何体渲染

测试目标：
1. 创建 Part::Box 对象
2. 切换到 OsgVerse 后端
3. 验证显示的是真实立方体（不是红色球体）
4. 检查颜色和材质
"""

import FreeCAD
import FreeCADGui
import time

print("=" * 60)
print("Phase 2 Step 1 - 真实几何体渲染测试")
print("=" * 60)

# 创建新文档
doc = FreeCAD.newDocument("Phase2Test")
print("\n✓ 创建文档: Phase2Test")

# 创建 Box 对象
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()
print("✓ 创建 Box 对象 (10x10x10)")

# 显示文档
FreeCADGui.showMainWindow()
FreeCADGui.activeDocument().activeView().viewAxometric()
FreeCADGui.SendMsgToActiveView("ViewFit")
print("✓ 显示文档")

# 等待渲染
time.sleep(1)

# 获取当前后端
try:
    from Gui import RenderManager
    current_backend = RenderManager.getCurrentBackend()
    print(f"\n当前渲染后端: {current_backend}")
except:
    print("\n⚠ 无法获取当前后端信息")

# 切换到 OsgVerse
print("\n切换到 OsgVerse 后端...")
try:
    FreeCADGui.switchRenderBackend(2)  # 2 = OsgVerse
    print("✓ 切换成功")
    
    # 等待切换完成
    time.sleep(2)
    
    # 再次适应视图
    FreeCADGui.SendMsgToActiveView("ViewFit")
    
    print("\n" + "=" * 60)
    print("测试说明：")
    print("=" * 60)
    print("请检查 3D 视图中的对象：")
    print()
    print("✓ 应该看到：")
    print("  - 一个真实的立方体（6个面）")
    print("  - 正确的边缘和形状")
    print("  - 灰色材质（默认颜色）")
    print("  - 可以旋转、缩放")
    print()
    print("✗ 不应该看到：")
    print("  - 红色球体（那是 Phase 1 的占位符）")
    print()
    print("=" * 60)
    print("如果看到真实立方体，Phase 2 Step 1 成功！")
    print("=" * 60)
    
except Exception as e:
    print(f"✗ 切换失败: {e}")
    import traceback
    traceback.print_exc()

print("\n测试脚本完成")
print("文档保持打开状态，请手动检查渲染结果")
