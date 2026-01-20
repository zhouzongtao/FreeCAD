"""
渲染后端切换测试脚本

测试在 Coin3D 和 OsgVerse 之间动态切换的功能
"""

import FreeCAD
import FreeCADGui
import time

print("=" * 70)
print("渲染后端切换测试")
print("=" * 70)

# 后端类型常量
BACKEND_NONE = 0
BACKEND_COIN3D = 1
BACKEND_OSGVERSE = 2

backend_names = {
    0: "None",
    1: "Coin3D",
    2: "OsgVerse"
}

def get_current_backend():
    """获取当前后端"""
    try:
        backend = FreeCADGui.getCurrentRenderBackend()
        return backend, backend_names.get(backend, "Unknown")
    except Exception as e:
        print(f"错误：无法获取当前后端 - {e}")
        return None, None

def check_backend_available(backend_type):
    """检查后端是否可用"""
    try:
        available = FreeCADGui.isRenderBackendAvailable(backend_type)
        return available
    except Exception as e:
        print(f"错误：无法检查后端可用性 - {e}")
        return False

def switch_backend(backend_type):
    """切换到指定后端"""
    try:
        success = FreeCADGui.switchRenderBackend(backend_type)
        return success
    except Exception as e:
        print(f"错误：切换后端失败 - {e}")
        return False

def get_renderer_info():
    """获取渲染器信息"""
    try:
        info = FreeCADGui.getRendererInfo()
        return info
    except Exception as e:
        print(f"错误：无法获取渲染器信息 - {e}")
        return "Unknown"

# 1. 显示当前状态
print("\n1. 当前状态")
print("-" * 70)
current_backend, current_name = get_current_backend()
if current_backend is not None:
    print(f"当前后端: {current_name} (类型 {current_backend})")
    print(f"渲染器信息: {get_renderer_info()}")

# 2. 检查可用的后端
print("\n2. 检查可用后端")
print("-" * 70)
for backend_type, name in backend_names.items():
    if backend_type == BACKEND_NONE:
        continue
    available = check_backend_available(backend_type)
    status = "✓ 可用" if available else "✗ 不可用"
    print(f"{name:12} : {status}")

# 3. 创建测试对象（如果还没有）
print("\n3. 创建测试对象")
print("-" * 70)
doc = FreeCAD.ActiveDocument
if not doc:
    doc = FreeCAD.newDocument("BackendSwitchTest")
    print("✓ 创建新文档")
else:
    print(f"✓ 使用现有文档: {doc.Name}")

# 创建几个不同的对象
if not hasattr(doc, 'TestBox'):
    box = doc.addObject("Part::Box", "TestBox")
    box.Length = 10
    box.Width = 10
    box.Height = 10
    print("✓ 创建 Box")
else:
    print("✓ Box 已存在")

if not hasattr(doc, 'TestCylinder'):
    cylinder = doc.addObject("Part::Cylinder", "TestCylinder")
    cylinder.Radius = 5
    cylinder.Height = 15
    cylinder.Placement.Base = FreeCAD.Vector(20, 0, 0)
    print("✓ 创建 Cylinder")
else:
    print("✓ Cylinder 已存在")

if not hasattr(doc, 'TestSphere'):
    sphere = doc.addObject("Part::Sphere", "TestSphere")
    sphere.Radius = 6
    sphere.Placement.Base = FreeCAD.Vector(-20, 0, 0)
    print("✓ 创建 Sphere")
else:
    print("✓ Sphere 已存在")

doc.recompute()
print("✓ 文档重新计算完成")

# 4. 测试切换到 Coin3D
print("\n4. 切换到 Coin3D")
print("-" * 70)
if check_backend_available(BACKEND_COIN3D):
    print("正在切换到 Coin3D...")
    if switch_backend(BACKEND_COIN3D):
        print("✓ 成功切换到 Coin3D")
        time.sleep(0.5)  # 等待切换完成
        current_backend, current_name = get_current_backend()
        print(f"  当前后端: {current_name}")
        print(f"  渲染器信息: {get_renderer_info()}")
        
        # 适应视图
        FreeCADGui.SendMsgToActiveView("ViewFit")
        print("✓ 视图已适应")
        
        print("\n请检查：")
        print("  - 对象应该显示为真实的几何体（Box、Cylinder、Sphere）")
        print("  - 对象应该有正确的颜色和材质")
        print("  - 可以正常旋转、缩放视图")
    else:
        print("✗ 切换到 Coin3D 失败")
else:
    print("✗ Coin3D 不可用")

input("\n按 Enter 继续切换到 OsgVerse...")

# 5. 测试切换到 OsgVerse
print("\n5. 切换到 OsgVerse")
print("-" * 70)
if check_backend_available(BACKEND_OSGVERSE):
    print("正在切换到 OsgVerse...")
    if switch_backend(BACKEND_OSGVERSE):
        print("✓ 成功切换到 OsgVerse")
        time.sleep(0.5)  # 等待切换完成
        current_backend, current_name = get_current_backend()
        print(f"  当前后端: {current_name}")
        print(f"  渲染器信息: {get_renderer_info()}")
        
        # 适应视图
        FreeCADGui.SendMsgToActiveView("ViewFit")
        print("✓ 视图已适应")
        
        print("\n请检查：")
        print("  - 所有对象应该显示为红色球体（Phase 1 占位符）")
        print("  - 球体应该完整可见，没有被裁剪")
        print("  - 可以正常旋转、缩放视图")
        print("  - Report View 中应该有 [OsgVerse] 前缀的日志")
    else:
        print("✗ 切换到 OsgVerse 失败")
else:
    print("✗ OsgVerse 不可用")

input("\n按 Enter 继续切换回 Coin3D...")

# 6. 切换回 Coin3D
print("\n6. 切换回 Coin3D")
print("-" * 70)
if check_backend_available(BACKEND_COIN3D):
    print("正在切换回 Coin3D...")
    if switch_backend(BACKEND_COIN3D):
        print("✓ 成功切换回 Coin3D")
        time.sleep(0.5)
        current_backend, current_name = get_current_backend()
        print(f"  当前后端: {current_name}")
        print(f"  渲染器信息: {get_renderer_info()}")
        
        # 适应视图
        FreeCADGui.SendMsgToActiveView("ViewFit")
        print("✓ 视图已适应")
        
        print("\n请检查：")
        print("  - 对象应该恢复为真实的几何体")
        print("  - 所有功能应该正常工作")
    else:
        print("✗ 切换回 Coin3D 失败")
else:
    print("✗ Coin3D 不可用")

# 7. 总结
print("\n" + "=" * 70)
print("测试完成！")
print("=" * 70)
current_backend, current_name = get_current_backend()
print(f"最终后端: {current_name}")
print(f"渲染器信息: {get_renderer_info()}")

print("\n测试要点：")
print("1. ✓ 可以动态切换后端")
print("2. ✓ Coin3D 显示真实几何体")
print("3. ✓ OsgVerse 显示红色球体占位符（Phase 1）")
print("4. ✓ 切换后视图正常工作")
print("5. ✓ 可以多次来回切换")

print("\n如果所有检查都通过，说明后端切换功能正常！")
