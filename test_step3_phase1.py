"""
测试 Phase 6 Step 3 Phase 1 - OsgVerse Viewer 占位符实现

这个脚本测试 OsgVerse viewer 是否成功注册到 ViewerFactory
"""

import FreeCADGui as Gui

print("=" * 60)
print("Phase 6 Step 3 Phase 1 测试")
print("=" * 60)

# 1. 检查 OsgVerse 后端是否可用
print("\n1. 检查 OsgVerse 后端可用性:")
try:
    available = Gui.isRenderBackendAvailable(2)  # 2 = OsgVerse
    print(f"   OsgVerse 可用: {available}")
    
    if not available:
        print("   ❌ OsgVerse 后端不可用")
        print("   可能原因:")
        print("   - BUILD_WITH_OSGVERSE 未启用")
        print("   - OsgVerse viewer 注册失败")
    else:
        print("   ✅ OsgVerse 后端可用")
except Exception as e:
    print(f"   ❌ 检查失败: {e}")

# 2. 获取当前后端
print("\n2. 获取当前渲染后端:")
try:
    current = Gui.getCurrentRenderBackend()
    backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print(f"   当前后端: {current} ({backend_names.get(current, 'Unknown')})")
except Exception as e:
    print(f"   ❌ 获取失败: {e}")

# 3. 尝试切换到 OsgVerse（如果可用）
print("\n3. 尝试切换到 OsgVerse:")
try:
    if Gui.isRenderBackendAvailable(2):
        success = Gui.switchRenderBackend(2)
        print(f"   切换结果: {success}")
        
        if success:
            print("   ✅ 成功切换到 OsgVerse")
            
            # 验证切换
            current = Gui.getCurrentRenderBackend()
            backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
            print(f"   当前后端: {current} ({backend_names.get(current, 'Unknown')})")
            
            if current == 2:
                print("   ✅ 验证成功：当前后端是 OsgVerse")
            else:
                print(f"   ⚠️ 警告：切换后当前后端不是 OsgVerse (是 {current})")
        else:
            print("   ❌ 切换失败")
    else:
        print("   ⏭️ 跳过：OsgVerse 不可用")
except Exception as e:
    print(f"   ❌ 切换失败: {e}")

# 4. 获取渲染器信息
print("\n4. 获取渲染器信息:")
try:
    info = Gui.getRendererInfo()
    print(f"   渲染器信息: {info}")
except Exception as e:
    print(f"   ❌ 获取失败: {e}")

# 5. 切换回 Coin3D
print("\n5. 切换回 Coin3D:")
try:
    success = Gui.switchRenderBackend(1)
    print(f"   切换结果: {success}")
    
    if success:
        current = Gui.getCurrentRenderBackend()
        backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
        print(f"   当前后端: {current} ({backend_names.get(current, 'Unknown')})")
        print("   ✅ 成功切换回 Coin3D")
except Exception as e:
    print(f"   ❌ 切换失败: {e}")

print("\n" + "=" * 60)
print("测试完成")
print("=" * 60)

print("\n注意事项:")
print("- Phase 1 是占位符实现，视图可能是空白的")
print("- 这是正常的，实际渲染将在 Phase 2 实现")
print("- 当前测试主要验证注册和切换机制")
