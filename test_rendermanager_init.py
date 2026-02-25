#!/usr/bin/env python3
"""
测试 RenderManager 初始化状态
Test RenderManager initialization status
"""

import sys
import os

# 添加 FreeCAD 路径
freecad_path = r"E:\Repository\FreeCAD\FreeCAD\build\bin"
if freecad_path not in sys.path:
    sys.path.insert(0, freecad_path)

try:
    import FreeCAD
    import FreeCADGui
    
    print("=" * 80)
    print("RenderManager 初始化状态测试")
    print("=" * 80)
    
    # 检查 RenderManager Python 绑定是否可用
    print("\n1. 检查 Python 绑定...")
    if hasattr(FreeCADGui, 'getCurrentRenderBackend'):
        print("   ✓ RenderManager Python 绑定已加载")
    else:
        print("   ✗ RenderManager Python 绑定未加载")
        sys.exit(1)
    
    # 获取当前后端
    print("\n2. 获取当前渲染后端...")
    try:
        backend = FreeCADGui.getCurrentRenderBackend()
        backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
        print(f"   当前后端: {backend_names.get(backend, 'Unknown')} ({backend})")
    except Exception as e:
        print(f"   ✗ 获取后端失败: {e}")
        sys.exit(1)
    
    # 检查 OsgVerse 是否可用
    print("\n3. 检查 OsgVerse 可用性...")
    try:
        available = FreeCADGui.isRenderBackendAvailable(2)  # 2 = OsgVerse
        print(f"   OsgVerse 可用: {available}")
    except Exception as e:
        print(f"   ✗ 检查失败: {e}")
    
    # 获取渲染器信息
    print("\n4. 获取渲染器信息...")
    try:
        info = FreeCADGui.getRendererInfo()
        print(f"   渲染器: {info}")
    except Exception as e:
        print(f"   ✗ 获取信息失败: {e}")
    
    # 尝试切换到 OsgVerse
    print("\n5. 尝试切换到 OsgVerse...")
    try:
        result = FreeCADGui.switchRenderBackend(2)  # 2 = OsgVerse
        print(f"   切换结果: {result}")
        
        # 再次检查当前后端
        backend = FreeCADGui.getCurrentRenderBackend()
        print(f"   切换后的后端: {backend_names.get(backend, 'Unknown')} ({backend})")
    except Exception as e:
        print(f"   ✗ 切换失败: {e}")
    
    print("\n" + "=" * 80)
    print("测试完成")
    print("=" * 80)
    
except ImportError as e:
    print(f"导入失败: {e}")
    sys.exit(1)
except Exception as e:
    print(f"测试失败: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
