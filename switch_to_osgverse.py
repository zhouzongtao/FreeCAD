#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
切换 FreeCAD 渲染后端到 OsgVerse
Switch FreeCAD render backend to OsgVerse

使用方法 / Usage:
1. 在 FreeCAD 中打开 Python 控制台
2. 执行: exec(open('switch_to_osgverse.py').read())

或者 / Or:
1. 工具 -> 宏 -> 宏...
2. 创建新宏，粘贴此脚本内容
3. 执行宏
"""

import FreeCAD as App
import FreeCADGui as Gui

def switch_to_osgverse():
    """切换到 OsgVerse 渲染后端"""
    
    print("=" * 60)
    print("FreeCAD 渲染后端切换工具")
    print("=" * 60)
    
    # 方法 1: 尝试使用 Python 绑定（如果可用）
    try:
        # 检查是否有 switchRenderBackend 函数
        if hasattr(Gui, 'switchRenderBackend'):
            print("\n使用 Python API 切换后端...")
            
            # 检查 OsgVerse 是否可用
            if Gui.isRenderBackendAvailable(2):
                print("✓ OsgVerse 后端可用")
                
                # 获取当前后端
                current = Gui.getCurrentRenderBackend()
                backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
                print(f"当前后端: {backend_names.get(current, 'Unknown')}")
                
                if current == 2:
                    print("✓ 已经在使用 OsgVerse 后端")
                    return True
                
                # 切换到 OsgVerse
                print("正在切换到 OsgVerse...")
                success = Gui.switchRenderBackend(2)
                
                if success:
                    print("✓ 成功切换到 OsgVerse 后端")
                    print("\n渲染器信息:")
                    print(Gui.getRendererInfo())
                    
                    # 显示统计信息
                    stats = Gui.getRenderStats()
                    print("\n渲染统计:")
                    for key, value in stats.items():
                        print(f"  {key}: {value}")
                    
                    return True
                else:
                    print("✗ 切换失败")
                    return False
            else:
                print("✗ OsgVerse 后端不可用")
                print("  可能原因:")
                print("  1. OsgVerse 未编译到此版本")
                print("  2. 缺少 OpenSceneGraph 依赖库")
                print("  3. 系统不支持 OsgVerse")
                return False
        else:
            raise AttributeError("Python binding not available")
            
    except (ImportError, AttributeError) as e:
        print(f"\nPython API 不可用: {e}")
        print("使用参数设置方法...")
        
        # 方法 2: 通过参数设置（需要重启）
        try:
            param = App.ParamGet("User parameter:BaseApp/Preferences/View")
            param.SetInt("RenderBackend", 2)
            
            print("✓ 已设置渲染后端参数为 OsgVerse")
            print("⚠ 请重启 FreeCAD 使设置生效")
            
            # 显示当前参数
            current_value = param.GetInt("RenderBackend", 1)
            print(f"当前参数值: {current_value}")
            
            return True
            
        except Exception as e:
            print(f"✗ 设置参数失败: {e}")
            return False
    
    except Exception as e:
        print(f"✗ 发生错误: {e}")
        import traceback
        traceback.print_exc()
        return False

def switch_to_coin3d():
    """切换回 Coin3D 渲染后端"""
    
    print("\n" + "=" * 60)
    print("切换回 Coin3D 后端")
    print("=" * 60)
    
    try:
        if hasattr(Gui, 'switchRenderBackend'):
            success = Gui.switchRenderBackend(1)
            if success:
                print("✓ 成功切换到 Coin3D 后端")
            else:
                print("✗ 切换失败")
            return success
        else:
            param = App.ParamGet("User parameter:BaseApp/Preferences/View")
            param.SetInt("RenderBackend", 1)
            print("✓ 已设置渲染后端参数为 Coin3D")
            print("⚠ 请重启 FreeCAD 使设置生效")
            return True
    except Exception as e:
        print(f"✗ 发生错误: {e}")
        return False

def get_backend_info():
    """获取当前后端信息"""
    
    print("\n" + "=" * 60)
    print("渲染后端信息")
    print("=" * 60)
    
    try:
        if hasattr(Gui, 'getCurrentRenderBackend'):
            current = Gui.getCurrentRenderBackend()
            backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
            
            print(f"\n当前后端: {backend_names.get(current, 'Unknown')} ({current})")
            print(f"渲染器: {Gui.getRendererInfo()}")
            
            print("\n可用后端:")
            for backend_id, backend_name in backend_names.items():
                available = Gui.isRenderBackendAvailable(backend_id)
                status = "✓" if available else "✗"
                print(f"  {status} {backend_name} ({backend_id})")
            
            if current != 0:
                print("\n渲染统计:")
                stats = Gui.getRenderStats()
                for key, value in stats.items():
                    print(f"  {key}: {value}")
        else:
            param = App.ParamGet("User parameter:BaseApp/Preferences/View")
            backend_value = param.GetInt("RenderBackend", 1)
            backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
            print(f"\n参数设置: {backend_names.get(backend_value, 'Unknown')} ({backend_value})")
            print("(需要重启 FreeCAD 才能生效)")
            
    except Exception as e:
        print(f"✗ 获取信息失败: {e}")

# 主程序
if __name__ == "__main__":
    # 显示当前信息
    get_backend_info()
    
    # 切换到 OsgVerse
    print("\n")
    success = switch_to_osgverse()
    
    if success:
        print("\n" + "=" * 60)
        print("操作完成")
        print("=" * 60)
    else:
        print("\n" + "=" * 60)
        print("操作失败，请检查日志")
        print("=" * 60)

# 如果直接在 FreeCAD 控制台执行
try:
    # 显示菜单
    print("\n可用命令:")
    print("  switch_to_osgverse()  - 切换到 OsgVerse")
    print("  switch_to_coin3d()    - 切换到 Coin3D")
    print("  get_backend_info()    - 显示后端信息")
except:
    pass
