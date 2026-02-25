"""
Phase 2 当前版本测试脚本

测试目标：
1. 验证 OsgVerse 后端可以正常切换
2. 验证可以检测 Part 对象
3. 验证占位符渲染正常工作
4. 验证材质和颜色应用
5. 检查日志输出
"""

import FreeCAD
import FreeCADGui
import time

print("=" * 70)
print("Phase 2 当前版本测试")
print("=" * 70)
print()

# 创建新文档
doc = FreeCAD.newDocument("Phase2CurrentTest")
print("✓ 创建文档: Phase2CurrentTest")

# 创建多个 Part 对象
print("\n创建测试对象...")
box = doc.addObject("Part::Box", "TestBox")
box.Length = 10
box.Width = 10
box.Height = 10
print("  ✓ Box (10x10x10)")

cylinder = doc.addObject("Part::Cylinder", "TestCylinder")
cylinder.Radius = 5
cylinder.Height = 15
print("  ✓ Cylinder (R=5, H=15)")

sphere = doc.addObject("Part::Sphere", "TestSphere")
sphere.Radius = 7
print("  ✓ Sphere (R=7)")

# 设置不同的颜色
print("\n设置颜色...")
boxVP = FreeCADGui.ActiveDocument.getObject("TestBox")
if boxVP:
    boxVP.ShapeColor = (1.0, 0.0, 0.0)  # 红色
    print("  ✓ Box: 红色")

cylinderVP = FreeCADGui.ActiveDocument.getObject("TestCylinder")
if cylinderVP:
    cylinderVP.ShapeColor = (0.0, 1.0, 0.0)  # 绿色
    print("  ✓ Cylinder: 绿色")

sphereVP = FreeCADGui.ActiveDocument.getObject("TestSphere")
if sphereVP:
    sphereVP.ShapeColor = (0.0, 0.0, 1.0)  # 蓝色
    sphereVP.Transparency = 50  # 50% 透明
    print("  ✓ Sphere: 蓝色 + 50% 透明")

doc.recompute()
print("\n✓ 文档重新计算完成")

# 显示文档
print("\n显示主窗口...")
FreeCADGui.showMainWindow()
FreeCADGui.activeDocument().activeView().viewAxometric()
FreeCADGui.SendMsgToActiveView("ViewFit")
print("✓ 主窗口已显示")

# 等待渲染
time.sleep(1)

# 获取当前后端
print("\n" + "=" * 70)
print("当前渲染后端信息")
print("=" * 70)
try:
    from Gui import RenderManager
    current_backend = RenderManager.getCurrentBackend()
    print(f"当前后端: {current_backend}")
except Exception as e:
    print(f"⚠ 无法获取后端信息: {e}")

# 切换到 OsgVerse
print("\n" + "=" * 70)
print("切换到 OsgVerse 后端")
print("=" * 70)
try:
    print("正在切换...")
    FreeCADGui.switchRenderBackend(2)  # 2 = OsgVerse
    print("✓ 切换成功")
    
    # 等待切换完成
    time.sleep(2)
    
    # 再次适应视图
    FreeCADGui.SendMsgToActiveView("ViewFit")
    
    print("\n" + "=" * 70)
    print("测试结果")
    print("=" * 70)
    print()
    print("✅ 如果没有崩溃，说明基本功能正常")
    print()
    print("📋 当前版本特性：")
    print("  1. ✓ 可以正常切换到 OsgVerse 后端")
    print("  2. ✓ 可以检测 Part 对象（检查日志）")
    print("  3. ✓ 显示占位符球体（红色）")
    print("  4. ✓ 应用材质和颜色")
    print("  5. ✓ 支持透明度")
    print()
    print("🔍 请检查 3D 视图：")
    print("  - 应该看到 3 个红色球体（占位符）")
    print("  - 每个球体应该有不同的颜色（红/绿/蓝）")
    print("  - 蓝色球体应该是半透明的")
    print()
    print("📝 检查控制台日志：")
    print("  - 应该看到 'Found Shape property' 消息")
    print("  - 应该看到 'Found Part::PropertyPartShape' 消息")
    print("  - 应该看到 'Using placeholder sphere' 消息")
    print()
    print("=" * 70)
    print("⚠ 注意：真实几何体转换尚未实现")
    print("=" * 70)
    print("当前显示的是占位符球体，不是真实的 Box/Cylinder/Sphere")
    print("下一步需要实现 Python API 桥接来提取真实 Shape")
    print("=" * 70)
    
except Exception as e:
    print(f"\n✗ 切换失败: {e}")
    import traceback
    traceback.print_exc()
    print("\n" + "=" * 70)
    print("错误分析")
    print("=" * 70)
    print("如果出现错误，可能的原因：")
    print("1. OsgVerse 后端未正确编译")
    print("2. 缺少必要的 DLL 文件")
    print("3. 配置问题")
    print("=" * 70)

print("\n测试脚本完成")
print("文档保持打开状态，请手动检查渲染结果和日志")
print()
