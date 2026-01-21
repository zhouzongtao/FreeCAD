"""
最终测试 - OsgVerse 是否可用
"""

import FreeCADGui

print("=" * 70)
print("最终测试 - OsgVerse 可用性")
print("=" * 70)

# 1. 初始化 RenderManager
print("\n1. 初始化 RenderManager...")
try:
    result = FreeCADGui.initializeRenderManager()
    print(f"   初始化结果: {result}")
    if not result:
        print("   ❌ 初始化失败")
        exit(1)
    print("   ✅ 初始化成功")
except Exception as e:
    print(f"   ❌ 异常: {e}")
    exit(1)

# 2. 检查 OsgVerse 是否可用
print("\n2. 检查 OsgVerse 可用性...")
try:
    osgverse_available = FreeCADGui.isRenderBackendAvailable(2)
    print(f"   OsgVerse 可用: {osgverse_available}")
    if osgverse_available:
        print("   ✅ OsgVerse 可用！")
    else:
        print("   ❌ OsgVerse 不可用")
        print("   提示: 检查 Report View 中的日志")
except Exception as e:
    print(f"   ❌ 异常: {e}")
    exit(1)

# 3. 检查当前后端
print("\n3. 当前后端信息...")
try:
    current = FreeCADGui.getCurrentRenderBackend()
    backends = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print(f"   当前后端: {current} ({backends.get(current, 'Unknown')})")
    
    info = FreeCADGui.getRendererInfo()
    print(f"   渲染器信息: {info}")
except Exception as e:
    print(f"   ❌ 异常: {e}")

# 4. 如果 OsgVerse 可用，尝试切换
if osgverse_available:
    print("\n4. 切换到 OsgVerse...")
    try:
        switch_result = FreeCADGui.switchRenderBackend(2)
        print(f"   切换结果: {switch_result}")
        
        if switch_result:
            print("   ✅ 切换成功！")
            new_backend = FreeCADGui.getCurrentRenderBackend()
            new_info = FreeCADGui.getRendererInfo()
            print(f"   新后端: {new_backend} ({backends.get(new_backend, 'Unknown')})")
            print(f"   新渲染器: {new_info}")
        else:
            print("   ❌ 切换失败")
    except Exception as e:
        print(f"   ❌ 异常: {e}")

print("\n" + "=" * 70)
if osgverse_available:
    print("🎉 成功！OsgVerse 已可用并可以切换！")
else:
    print("❌ OsgVerse 仍然不可用，请检查 Report View 中的日志")
print("=" * 70)
