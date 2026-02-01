"""快速验证菜单是否恢复"""
import FreeCAD
import FreeCADGui

print("=" * 60)
print("FreeCAD 菜单验证")
print("=" * 60)

# 1. 检查工作台
print("\n【1】工作台检查")
print("-" * 60)
workbenches = FreeCADGui.listWorkbenches()
important_wbs = ["PartWorkbench", "PartDesignWorkbench", "SketcherWorkbench", 
                 "DraftWorkbench", "TechDrawWorkbench", "FemWorkbench"]

for wb in important_wbs:
    status = "✓" if wb in workbenches else "✗"
    wb_name = wb.replace("Workbench", "")
    print(f"{status} {wb_name:15s} {'可用' if wb in workbenches else '缺失'}")

print(f"\n总计: {len(workbenches)} 个工作台")

# 2. 检查主菜单
print("\n【2】主菜单检查")
print("-" * 60)
try:
    mw = FreeCADGui.getMainWindow()
    menubar = mw.menuBar()
    
    menu_count = 0
    for action in menubar.actions():
        menu_text = action.text().replace("&", "")
        if menu_text:  # 跳过分隔符
            print(f"  • {menu_text}")
            menu_count += 1
    
    print(f"\n总计: {menu_count} 个主菜单")
except Exception as e:
    print(f"✗ 无法访问菜单: {e}")

# 3. 检查Part工作台菜单
print("\n【3】Part工作台菜单检查")
print("-" * 60)
try:
    FreeCADGui.activateWorkbench("PartWorkbench")
    
    # 检查Part菜单是否存在
    part_menu_found = False
    for action in menubar.actions():
        menu_text = action.text().replace("&", "")
        if "Part" in menu_text:
            part_menu_found = True
            menu = action.menu()
            if menu:
                print(f"✓ Part菜单存在，包含 {len(menu.actions())} 个项目")
                # 显示前10个菜单项
                for i, act in enumerate(menu.actions()[:10]):
                    item_text = act.text().replace("&", "")
                    if item_text:
                        print(f"  • {item_text}")
            break
    
    if not part_menu_found:
        print("✗ Part菜单未找到")
        
except Exception as e:
    print(f"✗ 无法激活Part工作台: {e}")

# 4. 检查PartDesign工作台
print("\n【4】PartDesign工作台菜单检查")
print("-" * 60)
try:
    FreeCADGui.activateWorkbench("PartDesignWorkbench")
    
    pd_menu_found = False
    for action in menubar.actions():
        menu_text = action.text().replace("&", "")
        if "PartDesign" in menu_text or "Part Design" in menu_text:
            pd_menu_found = True
            menu = action.menu()
            if menu:
                print(f"✓ PartDesign菜单存在，包含 {len(menu.actions())} 个项目")
            break
    
    if not pd_menu_found:
        print("⚠ PartDesign菜单未找到（可能合并到Part菜单）")
        
except Exception as e:
    print(f"✗ 无法激活PartDesign工作台: {e}")

# 5. 检查Sketcher工作台
print("\n【5】Sketcher工作台菜单检查")
print("-" * 60)
try:
    FreeCADGui.activateWorkbench("SketcherWorkbench")
    
    sk_menu_found = False
    for action in menubar.actions():
        menu_text = action.text().replace("&", "")
        if "Sketch" in menu_text:
            sk_menu_found = True
            menu = action.menu()
            if menu:
                print(f"✓ Sketch菜单存在，包含 {len(menu.actions())} 个项目")
            break
    
    if not sk_menu_found:
        print("✗ Sketch菜单未找到")
        
except Exception as e:
    print(f"✗ 无法激活Sketcher工作台: {e}")

# 6. 检查工具栏
print("\n【6】工具栏检查")
print("-" * 60)
try:
    FreeCADGui.activateWorkbench("PartWorkbench")
    toolbars = mw.findChildren(mw.findChild(type(mw.menuBar())).__class__)
    
    # 获取所有工具栏
    from PySide2 import QtWidgets
    toolbars = mw.findChildren(QtWidgets.QToolBar)
    
    visible_toolbars = [tb for tb in toolbars if tb.isVisible()]
    print(f"可见工具栏: {len(visible_toolbars)}")
    
    for tb in visible_toolbars[:10]:  # 只显示前10个
        print(f"  • {tb.windowTitle()}")
        
except Exception as e:
    print(f"⚠ 无法检查工具栏: {e}")

# 总结
print("\n" + "=" * 60)
print("验证完成")
print("=" * 60)

if len(workbenches) >= 6:
    print("✓ 工作台数量正常")
else:
    print("✗ 工作台数量不足，可能有模块未加载")

print("\n提示: 如果菜单仍然缺失，请检查:")
print("  1. 模块是否编译成功 (*.pyd文件)")
print("  2. FreeCAD启动日志中的错误信息")
print("  3. 运行 test_all_modules.py 进行详细诊断")
