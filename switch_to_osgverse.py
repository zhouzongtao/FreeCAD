"""
如何切换到OsgVerse视图

这个脚本演示了如何在FreeCAD中切换到OsgVerse渲染backend
"""

import FreeCAD
import FreeCADGui

print("=" * 70)
print("OsgVerse Backend 切换指南")
print("=" * 70)

# Backend类型常量
BACKEND_NONE = 0
BACKEND_COIN3D = 1
BACKEND_OSGVERSE = 2

print("\n步骤1: 检查当前backend")
print("-" * 70)
try:
    current = FreeCADGui.getCurrentRenderBackend()
    backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print("当前backend: {} ({})".format(current, backend_names.get(current, "Unknown")))
except Exception as e:
    print("错误: {}".format(e))
    print("提示: RenderManager可能未初始化")

print("\n步骤2: 检查OsgVerse是否可用")
print("-" * 70)
try:
    available = FreeCADGui.isRenderBackendAvailable(BACKEND_OSGVERSE)
    if available:
        print("[成功] OsgVerse backend 可用!")
    else:
        print("[警告] OsgVerse backend 不可用")
        print("       请确保OsgVerseGui模块已加载")
except Exception as e:
    print("错误: {}".format(e))

print("\n步骤3: 切换到OsgVerse backend")
print("-" * 70)
try:
    print("正在切换到OsgVerse...")
    success = FreeCADGui.switchRenderBackend(BACKEND_OSGVERSE)
    if success:
        print("[成功] 已切换到OsgVerse backend!")
        
        # 验证切换
        current = FreeCADGui.getCurrentRenderBackend()
        if current == BACKEND_OSGVERSE:
            print("[验证] 当前backend确实是OsgVerse")
        else:
            print("[警告] 切换可能未完全生效")
    else:
        print("[失败] 无法切换到OsgVerse backend")
        print("       可能的原因:")
        print("       - OsgVerse backend未注册")
        print("       - RenderManager未初始化")
except Exception as e:
    print("错误: {}".format(e))

print("\n步骤4: 创建测试文档")
print("-" * 70)
try:
    doc = FreeCAD.newDocument("OsgVerseTest")
    print("文档已创建: {}".format(doc.Name))
    
    # 添加一个Box
    box = doc.addObject("Part::Box", "Box")
    box.Length = 10
    box.Width = 10
    box.Height = 10
    doc.recompute()
    print("Box对象已添加")
    
    print("\n[提示] 新创建的3D视图应该使用OsgVerse backend")
    print("       请在FreeCAD窗口中查看3D视图")
    
except Exception as e:
    print("错误: {}".format(e))

print("\n步骤5: 获取渲染器信息")
print("-" * 70)
try:
    info = FreeCADGui.getRendererInfo()
    print("渲染器信息: {}".format(info))
except Exception as e:
    print("错误: {}".format(e))

print("\n" + "=" * 70)
print("切换完成!")
print("=" * 70)

print("\n其他有用的命令:")
print("-" * 70)
print("# 切换回Coin3D:")
print("FreeCADGui.switchRenderBackend(1)")
print()
print("# 获取渲染统计:")
print("stats = FreeCADGui.getRenderStats()")
print("print(stats)")
print()
print("# 重置统计:")
print("FreeCADGui.resetRenderStats()")
