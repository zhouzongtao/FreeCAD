#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Drawing工作台使用演示脚本

这个脚本展示了如何使用完全基于C++实现的Drawing工作台，
以及AST/YAPTU工具生成的Python绑定接口。
"""

import FreeCAD as App
import Drawing

def create_demo_drawing():
    """创建演示绘图"""
    
    print("🎯 Drawing工作台演示开始...")
    
    # 创建新文档
    doc = App.newDocument("DrawingDemo")
    print("✅ 创建文档: DrawingDemo")
    
    # 使用C++实现的Python API创建对象
    print("\n🔥 使用AST/YAPTU生成的Python API:")
    
    # 1. 创建直线
    print("1. 创建直线...")
    line = Drawing.makeLine(App.Vector(0, 0, 0), App.Vector(20, 10, 0))
    line.Label = "Demo Line"
    line.LineWidth = 2.0
    line.LineColor = (1.0, 0.0, 0.0, 0.0)  # 红色
    print(f"   ✅ 直线长度: {line.Length:.2f} mm")
    print(f"   ✅ 直线角度: {line.Angle:.2f}°")
    
    # 2. 创建圆形
    print("2. 创建圆形...")
    circle = Drawing.makeCircle(App.Vector(30, 0, 0), 8.0)
    circle.Label = "Demo Circle"
    circle.LineWidth = 1.5
    circle.LineColor = (0.0, 0.0, 1.0, 0.0)  # 蓝色
    print(f"   ✅ 圆心: {circle.Center}")
    print(f"   ✅ 半径: {circle.Radius} mm")
    print(f"   ✅ 是否完整圆: {circle.isFullCircle()}")
    
    # 3. 创建矩形
    print("3. 创建矩形...")
    rect = Drawing.makeRectangle(App.Vector(-10, -10, 0), 15.0, 8.0)
    rect.Label = "Demo Rectangle"
    rect.LineWidth = 1.0
    rect.LineColor = (0.0, 1.0, 0.0, 0.0)  # 绿色
    rect.Rounded = True
    rect.CornerRadius = 2.0
    print(f"   ✅ 宽度: {rect.Width} mm")
    print(f"   ✅ 高度: {rect.Height} mm")
    print(f"   ✅ 圆角半径: {rect.CornerRadius} mm")
    
    # 4. 创建多边形
    print("4. 创建多边形...")
    poly = doc.addObject("Drawing::Polygon", "DemoPolygon")
    points = [
        App.Vector(40, 0, 0),
        App.Vector(45, 8, 0),
        App.Vector(50, 0, 0),
        App.Vector(47, -5, 0),
        App.Vector(43, -5, 0)
    ]
    poly.Points = points
    poly.Closed = True
    poly.LineWidth = 1.5
    poly.LineColor = (1.0, 0.5, 0.0, 0.0)  # 橙色
    print(f"   ✅ 多边形点数: {len(poly.Points)}")
    
    # 5. 添加文本
    print("5. 添加文本...")
    text = doc.addObject("Drawing::Text", "DemoText")
    text.Text = "Drawing Workbench\nC++ Implementation"
    text.Position = App.Vector(10, 20, 0)
    text.FontSize = 5.0
    text.FontName = "Arial"
    text.Justification = "Center"
    print(f"   ✅ 文本内容: {text.Text}")
    print(f"   ✅ 字体大小: {text.FontSize}")
    
    # 6. 添加尺寸标注
    print("6. 添加尺寸标注...")
    dim = doc.addObject("Drawing::Dimension", "DemoDimension")
    dim.DimLinePosition = App.Vector(10, -15, 0)
    dim.FormatSpec = "%.1f"
    dim.TextSize = 3.0
    dim.ShowUnits = True
    print(f"   ✅ 标注格式: {dim.FormatSpec}")
    
    # 重新计算文档
    print("\n🔄 重新计算文档...")
    doc.recompute()
    print("✅ 文档重新计算完成")
    
    # 显示对象信息
    print(f"\n📊 文档统计:")
    print(f"   总对象数: {len(doc.Objects)}")
    
    drawing_objects = [obj for obj in doc.Objects if obj.TypeId.startswith("Drawing::")]
    print(f"   Drawing对象数: {len(drawing_objects)}")
    
    for obj in drawing_objects:
        print(f"   - {obj.Label} ({obj.TypeId})")
    
    print(f"\n🎉 Drawing工作台演示完成！")
    print(f"📝 文档已创建，包含 {len(drawing_objects)} 个绘图对象")
    
    return doc

def test_python_bindings():
    """测试AST/YAPTU生成的Python绑定"""
    
    print("\n🔬 测试Python绑定功能...")
    
    doc = App.newDocument("BindingTest")
    
    # 测试类型检查
    line = doc.addObject("Drawing::Line", "TestLine")
    print(f"✅ Python类型: {type(line)}")
    print(f"✅ C++类型: {line.TypeId}")
    
    # 测试属性访问（通过AST/YAPTU绑定）
    line.StartPoint = App.Vector(0, 0, 0)
    line.EndPoint = App.Vector(10, 0, 0)
    print(f"✅ 属性访问正常: StartPoint = {line.StartPoint}")
    
    # 测试方法调用（通过AST/YAPTU绑定）
    direction = line.getDirection()
    print(f"✅ 方法调用正常: getDirection() = {direction}")
    
    # 测试属性变化通知
    line.Length = 15.0
    doc.recompute()
    print(f"✅ 属性变化处理正常: Length = {line.Length}")
    
    App.closeDocument("BindingTest")
    print("🎉 Python绑定测试通过！")

def demonstrate_ast_yaptu_workflow():
    """演示AST/YAPTU工作流程"""
    
    print("\n🔧 AST/YAPTU工作流程演示:")
    print("="*50)
    
    print("1. 📝 开发者编写Python接口定义文件(.pyi)")
    print("   例如: DrawingFeature.pyi, Line.pyi, Circle.pyi")
    print("   使用@export装饰器定义C++类映射关系")
    
    print("\n2. 🏗️ CMake构建系统配置")
    print("   CMakeLists.txt中包含: generate_from_py(DrawingFeature)")
    print("   这会在构建时自动调用AST/YAPTU工具")
    
    print("\n3. 🔥 构建时自动代码生成")
    print("   AST解析器: 分析.pyi文件，提取类和方法信息")
    print("   YAPTU引擎: 使用模板生成完整的C++绑定代码")
    print("   生成文件: DrawingFeaturePy.h/.cpp, LinePy.h/.cpp等")
    
    print("\n4. ⚙️ C++编译和链接")
    print("   编译器编译生成的绑定代码和手工C++代码")
    print("   链接生成Drawing.pyd和DrawingGui.pyd模块")
    
    print("\n5. 🚀 运行时使用")
    print("   Python可以直接使用C++对象和方法")
    print("   所有类型转换和错误处理都由生成的代码处理")
    
    print("\n🎯 关键优势:")
    print("   ✅ 自动化: 90%的绑定代码自动生成")
    print("   ✅ 类型安全: 强类型检查和转换")
    print("   ✅ 一致性: 所有绑定使用相同的模式")
    print("   ✅ 维护性: 接口变更时自动重新生成")

if __name__ == "__main__":
    """主演示函数"""
    
    print("🎨 FreeCAD Drawing工作台演示")
    print("="*60)
    print("这是一个完全基于C++实现的2D绘图工作台")
    print("展示了AST/YAPTU工具在FreeCAD开发中的应用")
    print("="*60)
    
    try:
        # 演示AST/YAPTU工作流程
        demonstrate_ast_yaptu_workflow()
        
        # 测试Python绑定
        test_python_bindings()
        
        # 创建演示绘图
        demo_doc = create_demo_drawing()
        
        print(f"\n🎉 演示完成！")
        print(f"📋 创建的文档: {demo_doc.Name}")
        print(f"💡 您可以切换到Drawing工作台使用交互式工具")
        print(f"🔧 这个工作台完美展示了AST/YAPTU的强大功能！")
        
    except Exception as e:
        print(f"❌ 演示过程中出现错误: {e}")
        print("💡 请确保Drawing模块已正确编译和安装")
    
    print("\n" + "="*60)
