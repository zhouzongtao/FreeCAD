#!/usr/bin/env python3
"""
检查 DLL 依赖关系和运行时库版本
Check DLL dependencies and runtime library versions
"""

import os
import sys
import subprocess

def check_dll_with_dumpbin(dll_path):
    """使用 dumpbin 检查 DLL 依赖"""
    try:
        result = subprocess.run(
            ['dumpbin', '/DEPENDENTS', dll_path],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode == 0:
            print(f"\n依赖项 / Dependencies for {os.path.basename(dll_path)}:")
            print("=" * 60)
            
            in_deps = False
            for line in result.stdout.split('\n'):
                line = line.strip()
                if 'Image has the following dependencies:' in line:
                    in_deps = True
                    continue
                if in_deps:
                    if line.endswith('.dll'):
                        print(f"  {line}")
                        # 高亮 OSG 相关的 DLL
                        if 'osg' in line.lower() or 'openthreads' in line.lower():
                            print(f"    ^ OSG related")
                    elif 'Summary' in line:
                        break
            
            # 检查运行时库
            print("\n运行时库信息 / Runtime library info:")
            if 'MSVCR' in result.stdout or 'MSVCP' in result.stdout:
                for line in result.stdout.split('\n'):
                    if 'MSVCR' in line or 'MSVCP' in line or 'VCRUNTIME' in line:
                        print(f"  {line.strip()}")
            else:
                print("  使用静态链接或未找到 MSVC 运行时 / Static linking or no MSVC runtime found")
                
        else:
            print(f"错误 / Error: dumpbin failed for {dll_path}")
            print(result.stderr)
            
    except FileNotFoundError:
        print("错误 / Error: dumpbin not found. Please run from Visual Studio Developer Command Prompt")
        return False
    except Exception as e:
        print(f"错误 / Error: {e}")
        return False
    
    return True

def main():
    freecad_bin = r"E:\Repository\FreeCAD\FreeCAD\build\bin"
    
    print("=" * 60)
    print("FreeCAD DLL 依赖检查")
    print("FreeCAD DLL Dependency Check")
    print("=" * 60)
    
    # 检查关键 DLL
    dlls_to_check = [
        "FreeCADGui.dll",
        "osg161-osg.dll",
        "osg161-osgDB.dll",
        "osg161-osgViewer.dll",
        "ot21-OpenThreads.dll"
    ]
    
    for dll_name in dlls_to_check:
        dll_path = os.path.join(freecad_bin, dll_name)
        if os.path.exists(dll_path):
            check_dll_with_dumpbin(dll_path)
        else:
            print(f"\n✗ {dll_name} 不存在 / not found")
    
    print("\n" + "=" * 60)
    print("建议 / Recommendations:")
    print("=" * 60)
    print("1. 确保所有 DLL 使用相同的 MSVC 运行时库 (/MD)")
    print("   Ensure all DLLs use the same MSVC runtime (/MD)")
    print("2. 检查 OSG DLL 是否使用 /MD 编译")
    print("   Check if OSG DLLs are compiled with /MD")
    print("3. 如果运行时库不匹配，需要重新编译 OSG")
    print("   If runtime mismatch, need to recompile OSG")

if __name__ == '__main__':
    main()
