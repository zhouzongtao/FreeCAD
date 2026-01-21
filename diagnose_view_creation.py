"""
诊断视图创建问题

检查：
1. 文档创建
2. 视图创建
3. 视图类型
4. Python 绑定
"""

import FreeCAD
import FreeCADGui
import traceback

def diagnose_view_creation():
    print("=" * 60)
    print("诊断视图创建问题")
    print("=" * 60)
    
    # 1. 检查现有文档
    print("\n1. 检查现有文档:")
    docs = FreeCAD.listDocuments()
    print(f"   现有文档数量: {len(docs)}")
    for name in docs:
        print(f"   - {name}")
    
    # 2. 创建新文档
    print("\n2. 创建新文档:")
    try:
        doc = FreeCAD.newDocument("DiagnoseTest")
        print(f"   ✓ 文档创建成功: {doc.Name}")
    except Exception as e:
        print(f"   ✗ 文档创建失败: {e}")
        traceback.print_exc()
        return
    
    # 3. 检查 GUI 文档
    print("\n3. 检查 GUI 文档:")
    try:
        gui_doc = FreeCADGui.getDocument(doc.Name)
        if gui_doc:
            print(f"   ✓ GUI 文档存在: {gui_doc}")
        else:
            print(f"   ✗ GUI 文档不存在")
            return
    except Exception as e:
        print(f"   ✗ 获取 GUI 文档失败: {e}")
        traceback.print_exc()
        return
    
    # 4. 检查活动文档
    print("\n4. 检查活动文档:")
    try:
        active_doc = FreeCADGui.activeDocument()
        if active_doc:
            print(f"   ✓ 活动文档: {active_doc}")
        else:
            print(f"   ✗ 没有活动文档")
    except Exception as e:
        print(f"   ✗ 获取活动文档失败: {e}")
        traceback.print_exc()
    
    # 5. 检查活动视图
    print("\n5. 检查活动视图:")
    try:
        active_view = FreeCADGui.activeDocument().activeView() if FreeCADGui.activeDocument() else None
        if active_view:
            print(f"   ✓ 活动视图: {active_view}")
            print(f"   - 类型: {type(active_view).__name__}")
            print(f"   - 类型ID: {type(active_view)}")
        else:
            print(f"   ✗ 没有活动视图 (activeView() 返回 None)")
            
            # 尝试获取所有视图
            print("\n   尝试获取所有视图:")
            try:
                # 这个方法可能不存在，但我们试试
                views = gui_doc.getMDIViews() if hasattr(gui_doc, 'getMDIViews') else []
                print(f"   - 视图数量: {len(views)}")
                for i, view in enumerate(views):
                    print(f"   - 视图 {i}: {view}, 类型: {type(view).__name__}")
            except Exception as e2:
                print(f"   - 无法获取视图列表: {e2}")
                
    except Exception as e:
        print(f"   ✗ 获取活动视图失败: {e}")
        traceback.print_exc()
    
    # 6. 检查主窗口
    print("\n6. 检查主窗口:")
    try:
        main_window = FreeCADGui.getMainWindow()
        if main_window:
            print(f"   ✓ 主窗口存在: {main_window}")
            
            # 检查活动窗口
            active_window = main_window.activeWindow() if hasattr(main_window, 'activeWindow') else None
            if active_window:
                print(f"   - 活动窗口: {active_window}")
            else:
                print(f"   - 没有活动窗口")
        else:
            print(f"   ✗ 主窗口不存在")
    except Exception as e:
        print(f"   ✗ 获取主窗口失败: {e}")
        traceback.print_exc()
    
    # 7. 尝试手动调用 viewDefaultOrientation
    print("\n7. 尝试调用 viewDefaultOrientation:")
    try:
        if active_view:
            if hasattr(active_view, 'viewDefaultOrientation'):
                print(f"   ✓ viewDefaultOrientation 方法存在")
                active_view.viewDefaultOrientation()
                print(f"   ✓ viewDefaultOrientation 调用成功")
            else:
                print(f"   ✗ viewDefaultOrientation 方法不存在")
        else:
            print(f"   ✗ 无法调用 (视图为 None)")
    except Exception as e:
        print(f"   ✗ 调用失败: {e}")
        traceback.print_exc()
    
    # 清理
    print("\n8. 清理:")
    try:
        FreeCAD.closeDocument(doc.Name)
        print(f"   ✓ 文档已关闭")
    except Exception as e:
        print(f"   ✗ 关闭文档失败: {e}")
    
    print("\n" + "=" * 60)
    print("诊断完成")
    print("=" * 60)

if __name__ == "__main__":
    diagnose_view_creation()
