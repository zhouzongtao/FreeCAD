"""
手动加载Drawing工作台的脚本

在FreeCAD的Python控制台中运行此脚本来加载Drawing工作台
"""

import FreeCAD
import FreeCADGui

# 首先确保Drawing模块可以加载
try:
    import Drawing
    print("✓ Drawing模块加载成功")
except ImportError as e:
    print(f"✗ Drawing模块加载失败: {e}")
    exit()

try:
    import DrawingGui
    print("✓ DrawingGui模块加载成功")
except ImportError as e:
    print(f"✗ DrawingGui模块加载失败: {e}")
    exit()

# 定义工作台类
class DrawingWorkbench(FreeCADGui.Workbench):
    def __init__(self):
        self.__class__.MenuText = "Drawing"
        self.__class__.ToolTip = "2D Drawing workbench"
    
    def Initialize(self):
        # 创建菜单和工具栏
        self.appendMenu("Drawing", [])
        self.appendToolbar("Drawing", [])
        print("Drawing工作台菜单和工具栏已创建")
    
    def GetClassName(self):
        return "DrawingGui::Workbench"

# 注册工作台
try:
    wb = DrawingWorkbench()
    FreeCADGui.addWorkbench(wb)
    print("✓ Drawing工作台已注册成功！")
    print("请检查工作台选择器，应该能看到'Drawing'选项。")
except Exception as e:
    print(f"✗ 工作台注册失败: {e}")
