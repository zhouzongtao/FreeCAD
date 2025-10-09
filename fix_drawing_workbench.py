"""
修复Drawing工作台显示问题的脚本

请在FreeCAD的Python控制台中运行此脚本
"""

import FreeCAD
import FreeCADGui

# 方法1：简单的Python工作台
class DrawingWorkbench(FreeCADGui.Workbench):
    def __init__(self):
        self.__class__.MenuText = "Drawing"
        self.__class__.ToolTip = "2D Drawing workbench"
    
    def Initialize(self):
        # 导入Drawing模块
        import Drawing
        print("Drawing模块已加载")
        
        # 创建一个简单的测试命令
        class DrawingInfoCommand:
            def GetResources(self):
                return {'MenuText': 'Drawing Info', 'ToolTip': 'Show Drawing info'}
            def Activated(self):
                import Drawing
                FreeCAD.Console.PrintMessage(f"Drawing functions: {dir(Drawing)}\n")
            def IsActive(self):
                return True
        
        FreeCADGui.addCommand("Drawing_Info", DrawingInfoCommand())
        
        # 创建菜单
        self.appendMenu("Drawing", ["Drawing_Info"])
        self.appendToolbar("Drawing", ["Drawing_Info"])
    
    def GetClassName(self):
        return "Gui::PythonWorkbench"

# 先移除可能存在的旧工作台
try:
    FreeCADGui.removeWorkbench("Drawing")
    print("已移除旧的Drawing工作台")
except:
    pass

# 注册新工作台
try:
    FreeCADGui.addWorkbench(DrawingWorkbench())
    print("✓ Drawing工作台已成功注册！")
    print("请重新启动FreeCAD或刷新工作台列表")
except Exception as e:
    print(f"注册失败: {e}")
    
    # 备选方案：直接使用C++工作台
    try:
        # 尝试直接激活C++工作台
        import DrawingGui
        print("✓ DrawingGui模块可用，工作台应该自动注册")
    except Exception as e2:
        print(f"DrawingGui导入失败: {e2}")
