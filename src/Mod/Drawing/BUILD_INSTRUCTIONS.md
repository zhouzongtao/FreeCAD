# Drawing Workbench Build Instructions

## 概述

这个Drawing工作台是一个完整的C++实现，展示了如何使用AST/YAPTU工具创建FreeCAD扩展。

## 🔧 构建过程中AST/YAPTU的作用

### 1. 输入文件（开发者编写）
```
src/Mod/Drawing/App/
├── DrawingFeature.pyi    # 🔥 Python接口定义
├── Line.pyi              # 🔥 Line类接口定义  
├── Circle.pyi            # 🔥 Circle类接口定义
├── DrawingFeature.cpp    # C++实现
└── CMakeLists.txt        # 包含generate_from_py()调用
```

### 2. AST/YAPTU处理过程
```bash
# 当运行cmake --build时，会自动执行：

# Step 1: AST解析器分析.pyi文件
python3 src/Tools/bindings/generate.py DrawingFeature.pyi
# → 解析@export装饰器和类定义
# → 提取方法签名和属性信息

# Step 2: YAPTU模板引擎生成C++代码
# → 使用templateClassPyExport.py模板
# → 生成DrawingFeaturePy.h和DrawingFeaturePy.cpp

# Step 3: 对每个.pyi文件重复上述过程
python3 src/Tools/bindings/generate.py Line.pyi      # → LinePy.h/.cpp
python3 src/Tools/bindings/generate.py Circle.pyi    # → CirclePy.h/.cpp
```

### 3. 生成的文件（自动生成）
```
build/src/Mod/Drawing/App/
├── DrawingFeaturePy.h    # 🤖 AST/YAPTU自动生成
├── DrawingFeaturePy.cpp  # 🤖 AST/YAPTU自动生成
├── LinePy.h              # 🤖 AST/YAPTU自动生成
├── LinePy.cpp            # 🤖 AST/YAPTU自动生成
├── CirclePy.h            # 🤖 AST/YAPTU自动生成
└── CirclePy.cpp          # 🤖 AST/YAPTU自动生成
```

## 🚀 构建步骤

### 前提条件
```bash
# 1. 确保有完整的FreeCAD开发环境
# 2. Python 3.8+ (用于AST/YAPTU)
# 3. CMake 3.22+
# 4. C++编译器

# 验证AST/YAPTU工具存在
ls src/Tools/bindings/generate.py
ls src/Tools/bindings/model/generateModel_Python.py
ls src/Tools/bindings/model/generateTools.py
```

### 构建命令
```bash
# 1. 配置构建（AST/YAPTU工具会被自动配置）
cmake -B build -S .

# 2. 构建Drawing模块（AST/YAPTU会自动运行）
cmake --build build --target Drawing DrawingGui

# 3. 验证生成的绑定代码
ls build/src/Mod/Drawing/App/*Py.h
ls build/src/Mod/Drawing/App/*Py.cpp
```

### 构建日志示例
```
[  5%] Building DrawingFeaturePy.h/.cpp out of DrawingFeature.pyi
TemplateClassPyExport build/src/Mod/Drawing/App/DrawingFeaturePy
Done generating: DrawingFeaturePy

[  6%] Building LinePy.h/.cpp out of Line.pyi  
TemplateClassPyExport build/src/Mod/Drawing/App/LinePy
Done generating: LinePy

[  7%] Building CirclePy.h/.cpp out of Circle.pyi
TemplateClassPyExport build/src/Mod/Drawing/App/CirclePy  
Done generating: CirclePy

[ 15%] Building CXX object src/Mod/Drawing/App/CMakeFiles/Drawing.dir/DrawingFeature.cpp.o
[ 16%] Building CXX object src/Mod/Drawing/App/CMakeFiles/Drawing.dir/DrawingFeaturePy.cpp.o
[ 17%] Building CXX object src/Mod/Drawing/App/CMakeFiles/Drawing.dir/LinePy.cpp.o
```

## 🎯 使用示例

### Python API使用
```python
# 启动FreeCAD并测试Drawing模块
import FreeCAD as App
import Drawing

# 创建文档
doc = App.newDocument()

# 使用C++实现的Python API
line = Drawing.makeLine(App.Vector(0,0,0), App.Vector(10,5,0))
circle = Drawing.makeCircle(App.Vector(0,0,0), 5.0)

# 访问C++对象的属性（通过AST/YAPTU生成的绑定）
print(f"Line length: {line.Length}")
print(f"Line angle: {line.Angle}")
print(f"Circle radius: {circle.Radius}")
print(f"Is full circle: {circle.isFullCircle()}")

# 修改属性
line.LineWidth = 2.0
line.LineColor = (1.0, 0.0, 0.0, 0.0)  # Red
circle.FirstAngle = 0.0
circle.LastAngle = 180.0  # Make it a semicircle

doc.recompute()
```

### 交互式使用
```python
# 在FreeCAD GUI中：
# 1. 切换到Drawing工作台
# 2. 使用工具栏按钮创建对象：
#    - Line工具：点击两个点创建直线
#    - Circle工具：点击中心点，再点击半径点
#    - Rectangle工具：点击对角两点
#    - Text工具：点击位置并输入文本
```

## 🔍 调试AST/YAPTU生成

### 手动运行代码生成
```bash
# 进入Drawing/App目录
cd src/Mod/Drawing/App

# 手动运行AST/YAPTU工具
python3 ../../../Tools/bindings/generate.py \
    --outputPath ../../../../build/src/Mod/Drawing/App/ \
    DrawingFeature.pyi

# 查看生成的代码
cat ../../../../build/src/Mod/Drawing/App/DrawingFeaturePy.h
cat ../../../../build/src/Mod/Drawing/App/DrawingFeaturePy.cpp
```

### 验证生成的绑定
```cpp
// 生成的DrawingFeaturePy.h应该包含：
class DrawingFeaturePy : public App::DocumentObjectPy {
    // 构造函数
    DrawingFeaturePy(Drawing::Feature *pcObject, PyTypeObject *T = &Type);
    
    // 获取C++对象指针
    Drawing::Feature *getDrawingFeaturePtr() const;
    
    // Python方法声明
    PyObject* recompute(PyObject *args);
    
    // 属性访问器
    static PyObject* staticCallback_getStartPoint(PyObject *self, void *closure);
    static int staticCallback_setStartPoint(PyObject *self, PyObject *value, void *closure);
    // ... 更多属性访问器
};
```

## 🎉 成功验证

构建成功后，您应该能够：

1. **在FreeCAD中看到Drawing工作台**
2. **使用Python API创建绘图对象**
3. **通过工具栏进行交互式绘图**
4. **修改对象属性并看到实时更新**

## 🚀 扩展开发

基于这个基础，您可以：

1. **添加更多绘图对象**：椭圆、样条曲线、标注等
2. **增强交互功能**：捕捉、网格、图层管理
3. **添加修改工具**：移动、复制、旋转、缩放
4. **集成外部库**：DXF导入导出、SVG支持等

这个Drawing工作台完美展示了**AST/YAPTU工具在实际FreeCAD开发中的应用**，是学习C++工作台开发的绝佳示例！
