#!/usr/bin/env python3
"""
检查当前 FreeCAD 使用的渲染后端
Check current FreeCAD rendering backend
"""

import sys

def check_backend():
    """检查当前渲染后端"""
    print("=" * 60)
    print("FreeCAD 渲染后端检查")
    print("FreeCAD Rendering Backend Check")
    print("=" * 60)
    print()
    
    try:
        import FreeCAD
        print(f"✓ FreeCAD 版本 / Version: {FreeCAD.Version()[0]}.{FreeCAD.Version()[1]}.{FreeCAD.Version()[2]}")
        print()
    except ImportError as e:
        print(f"✗ 无法导入 FreeCAD / Cannot import FreeCAD: {e}")
        return False
    
    try:
        import FreeCADGui as Gui
        print("✓ FreeCADGui 模块加载成功 / FreeCADGui module loaded")
        print()
    except ImportError as e:
        print(f"✗ 无法导入 FreeCADGui / Cannot import FreeCADGui: {e}")
        return False
    
    # 检查渲染相关的 API
    print("检查渲染 API / Checking rendering API:")
    print("-" * 60)
    
    # 方法 1: 检查是否有 RenderManager 相关 API
    has_render_api = False
    api_methods = [
        'getCurrentRenderBackend',
        'switchRenderBackend',
        'isRenderBackendAvailable',
        'listRenderBackends',
        'getRendererInfo'
    ]
    
    for method in api_methods:
        if hasattr(Gui, method):
            print(f"  ✓ Gui.{method} 可用 / available")
            has_render_api = True
        else:
            print(f"  ✗ Gui.{method} 不可用 / not available")
    
    print()
    
    if not has_render_api:
        print("⚠ 渲染管理 API 未暴露到 Python")
        print("⚠ Rendering management API not exposed to Python")
        print()
        print("这意味着:")
        print("This means:")
        print("1. RenderManagerPy.cpp 可能未编译进 FreeCADGui")
        print("   RenderManagerPy.cpp may not be compiled into FreeCADGui")
        print("2. 或者 Python 绑定未正确注册")
        print("   Or Python bindings not properly registered")
        print()
        print("默认情况下，FreeCAD 使用 Coin3D 渲染后端")
        print("By default, FreeCAD uses Coin3D rendering backend")
        print()
        return False
    
    # 方法 2: 如果 API 可用，获取当前后端
    try:
        current = Gui.getCurrentRenderBackend()
        backend_names = {
            0: "None",
            1: "Coin3D",
            2: "OsgVerse"
        }
        backend_name = backend_names.get(current, f"Unknown ({current})")
        
        print(f"当前渲染后端 / Current rendering backend:")
        print(f"  类型 / Type: {current}")
        print(f"  名称 / Name: {backend_name}")
        print()
        
        # 列出可用后端
        if hasattr(Gui, 'listRenderBackends'):
            available = Gui.listRenderBackends()
            print(f"可用后端 / Available backends: {available}")
            for backend_id in available:
                name = backend_names.get(backend_id, f"Unknown ({backend_id})")
                is_current = " (当前 / current)" if backend_id == current else ""
                print(f"  [{backend_id}] {name}{is_current}")
        print()
        
        # 获取渲染器信息
        if hasattr(Gui, 'getRendererInfo'):
            info = Gui.getRendererInfo()
            print("渲染器信息 / Renderer info:")
            for key, value in info.items():
                print(f"  {key}: {value}")
        print()
        
        return True
        
    except Exception as e:
        print(f"✗ 获取后端信息失败 / Failed to get backend info: {e}")
        return False

def main():
    success = check_backend()
    
    print("=" * 60)
    if success:
        print("✓ 检查完成 / Check complete")
    else:
        print("⚠ 检查未完全成功 / Check not fully successful")
        print()
        print("注意 / Note:")
        print("如果 API 不可用，这是正常的，因为 RenderManagerPy.cpp")
        print("If API is not available, this is normal because RenderManagerPy.cpp")
        print("可能还未添加到构建系统中。")
        print("may not be added to the build system yet.")
        print()
        print("FreeCAD 默认使用 Coin3D 后端进行渲染。")
        print("FreeCAD uses Coin3D backend for rendering by default.")
    print("=" * 60)
    
    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
