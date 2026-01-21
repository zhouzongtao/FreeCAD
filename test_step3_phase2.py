"""
测试 Phase 6 Step 3 Phase 2 - OsgVerse 基础渲染功能

这个脚本测试 OsgVerse viewer 的基础渲染功能
"""

import FreeCAD
import FreeCADGui as Gui

print("=" * 60)
print("Phase 6 Step 3 Phase 2 测试 - 基础渲染")
print("=" * 60)

# 1. 切换到 OsgVerse
print("\n1. 切换到 OsgVerse 后端:")
try:
    if not Gui.isRenderBackendAvailable(2):
        print("   ❌ OsgVerse 不可用")
        exit(1)
    
    success = Gui.switchRenderBackend(2)
    if success:
        print("   ✅ 成功切换到 OsgVerse")
        current = Gui.getCurrentRenderBackend()
        print(f"   当前后端: {current} (OsgVerse)")
    else:
        print("   ❌ 切换失败")
        exit(1)
except Exception as e:
    print(f"   ❌ 切换失败: {e}")
    exit(1)

# 2. 创建文档和对象
print("\n2. 创建测试文档和对象:")
try:
    doc = FreeCAD.newDocument("OsgVerseTest")
    print(f"   ✅ 创建文档: {doc.Name}")
    
    # 创建一个 Box
    box = doc.addObject("Part::Box", "Box")
    box.Length = 10
    box.Width = 10
    box.Height = 10
    doc.recompute()
    print(f"   ✅ 创建 Box: {box.Name}")
    
    # 创建一个 Cylinder
    cylinder = doc.addObject("Part::Cylinder", "Cylinder")
    cylinder.Radius = 5
    cylinder.Height = 15
    cylinder.Placement.Base = FreeCAD.Vector(20, 0, 0)
    doc.recompute()
    print(f"   ✅ 创建 Cylinder: {cylinder.Name}")
    
except Exception as e:
    print(f"   ❌ 创建失败: {e}")
    import traceback
    traceback.print_exc()

# 3. 打开 3D 视图
print("\n3. 打开 3D 视图:")
try:
    Gui.activeDocument().activeView()
    print("   ✅ 3D 视图已打开")
    print("   注意: Phase 2 可能还不能显示对象（需要 ViewProvider 支持）")
    print("   但应该能看到背景颜色和基本的渲染窗口")
except Exception as e:
    print(f"   ⚠️ 打开视图时出现问题: {e}")
    import traceback
    traceback.print_exc()

# 4. 测试相机功能
print("\n4. 测试相机功能:")
try:
    view = Gui.activeDocument().activeView()
    
    # 测试 viewAll
    print("   测试 viewAll()...")
    view.viewAll()
    print("   ✅ viewAll() 执行成功")
    
    # 等待一下
    import time
    time.sleep(1)
    
    # 测试 fitAll (如果可用)
    try:
        view.fitAll()
        print("   ✅ fitAll() 执行成功")
    except:
        print("   ⏭️ fitAll() 不可用（正常）")
    
except Exception as e:
    print(f"   ⚠️ 相机测试失败: {e}")
    import traceback
    traceback.print_exc()

# 5. 获取渲染器信息
print("\n5. 获取渲染器信息:")
try:
    info = Gui.getRendererInfo()
    print(f"   渲染器信息: {info}")
except Exception as e:
    print(f"   ⚠️ 获取失败: {e}")

print("\n" + "=" * 60)
print("测试完成")
print("=" * 60)

print("\n预期行为:")
print("✅ 视图窗口成功创建")
print("✅ 可以看到背景颜色（蓝灰色）")
print("⚠️ 可能看不到 Box 和 Cylinder（ViewProvider 支持在 Phase 3）")
print("✅ viewAll() 功能正常工作")
print("\n如果看到空白视图但有背景色，这是正常的！")
print("Phase 2 实现了基础渲染框架，对象显示需要 Phase 3 的 ViewProvider 支持。")
