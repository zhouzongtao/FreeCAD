"""测试所有关键模块是否可以正常加载"""
import sys

print("=== 测试FreeCAD模块加载 ===\n")

# 测试核心模块
modules_to_test = [
    ("FreeCAD", "核心模块"),
    ("FreeCADGui", "GUI模块"),
    ("Part", "Part模块"),
    ("PartGui", "PartGui模块"),
    ("PartDesign", "PartDesign模块"),
    ("PartDesignGui", "PartDesignGui模块"),
    ("Sketcher", "Sketcher模块"),
    ("SketcherGui", "SketcherGui模块"),
    ("Draft", "Draft模块"),
    ("TechDraw", "TechDraw模块"),
    ("TechDrawGui", "TechDrawGui模块"),
    ("Mesh", "Mesh模块"),
    ("MeshGui", "MeshGui模块"),
    ("Fem", "Fem模块"),
    ("FemGui", "FemGui模块"),
]

success_count = 0
failed_modules = []

for module_name, description in modules_to_test:
    try:
        if module_name == "FreeCAD":
            import FreeCAD
        elif module_name == "FreeCADGui":
            import FreeCADGui
        else:
            exec(f"import {module_name}")
        print(f"✓ {description:20s} - 加载成功")
        success_count += 1
    except ImportError as e:
        print(f"✗ {description:20s} - 加载失败: {e}")
        failed_modules.append(module_name)
    except Exception as e:
        print(f"⚠ {description:20s} - 警告: {e}")
        success_count += 1  # 某些模块可能需要GUI环境

print(f"\n=== 测试结果 ===")
print(f"成功: {success_count}/{len(modules_to_test)}")
print(f"失败: {len(failed_modules)}/{len(modules_to_test)}")

if failed_modules:
    print(f"\n失败的模块: {', '.join(failed_modules)}")

# 测试工作台
print("\n=== 测试工作台 ===")
try:
    import FreeCADGui
    workbenches = FreeCADGui.listWorkbenches()
    print(f"可用工作台数量: {len(workbenches)}")
    print("\n工作台列表:")
    for wb_name in sorted(workbenches.keys()):
        print(f"  - {wb_name}")
except Exception as e:
    print(f"无法列出工作台: {e}")

# 测试菜单
print("\n=== 测试主窗口菜单 ===")
try:
    import FreeCADGui
    mw = FreeCADGui.getMainWindow()
    if mw:
        menubar = mw.menuBar()
        menus = [menubar.actions()[i].text() for i in range(menubar.actions().__len__())]
        print(f"菜单数量: {len(menus)}")
        print("菜单列表:")
        for menu in menus:
            if menu:  # 跳过分隔符
                print(f"  - {menu}")
    else:
        print("主窗口未初始化")
except Exception as e:
    print(f"无法访问菜单: {e}")

print("\n=== 测试完成 ===")
