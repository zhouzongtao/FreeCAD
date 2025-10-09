# Drawing工作台构建指南

## 🎯 构建目录说明

构建命令需要在**FreeCAD源代码根目录**下运行，这是非常重要的！

### 📍 正确的构建目录

```bash
# ✅ 正确：在FreeCAD源代码根目录下运行
E:\Repository\FreeCAD\FreeCAD\          # ← 在这个目录下运行构建命令
├── CMakeLists.txt                      # 🔥 主CMake文件（必须存在）
├── src/                                # 源代码目录
│   ├── Tools/bindings/                 # 🔥 AST/YAPTU工具位置
│   └── Mod/Drawing/                    # 🔥 我们创建的工作台
├── cMake/                              # CMake辅助文件
└── build/                              # 🔥 构建输出目录（会自动创建）
```

### ❌ 错误的构建目录

```bash
# ❌ 错误：在子目录下运行
E:\Repository\FreeCAD\FreeCAD\src\Mod\Drawing\  # ← 不要在这里运行
E:\Repository\FreeCAD\FreeCAD\src\              # ← 不要在这里运行  
E:\Repository\FreeCAD\FreeCAD\build\            # ← 不要在这里运行
```

## 🚀 完整构建流程

### 步骤1: 确认当前目录

```bash
# 在PowerShell中确认当前目录
pwd
# 应该显示: E:\Repository\FreeCAD\FreeCAD

# 如果不在正确目录，需要切换
cd E:\Repository\FreeCAD\FreeCAD

# 验证关键文件存在
ls CMakeLists.txt                        # ✅ 主CMake文件
ls src\Tools\bindings\generate.py       # ✅ AST/YAPTU工具
ls src\Mod\Drawing\App\DrawingFeature.pyi  # ✅ 我们创建的接口文件
```

### 步骤2: 配置构建

```bash
# 🔥 在FreeCAD根目录下运行配置命令
cmake -B build -S . -DBUILD_DRAWING=ON

# 解释参数：
# -B build        : 构建目录为 ./build/
# -S .            : 源代码目录为当前目录 (./)
# -DBUILD_DRAWING=ON : 启用Drawing模块构建
```

### 步骤3: 执行构建

```bash
# 🔥 在FreeCAD根目录下运行构建命令
cmake --build build --target Drawing DrawingGui

# 或者构建整个项目（包含Drawing）
cmake --build build

# 解释：
# --build build   : 使用build/目录进行构建
# --target Drawing DrawingGui : 只构建Drawing相关目标
```

## 🔍 目录结构和路径关系

### 构建时的路径解析

```bash
# 当前目录: E:\Repository\FreeCAD\FreeCAD\
# CMake会设置以下变量：

CMAKE_SOURCE_DIR = E:\Repository\FreeCAD\FreeCAD
CMAKE_BINARY_DIR = E:\Repository\FreeCAD\FreeCAD\build

# AST/YAPTU工具路径：
TOOL_PATH = ${CMAKE_SOURCE_DIR}/src/Tools/bindings/generate.py
         = E:\Repository\FreeCAD\FreeCAD\src\Tools\bindings\generate.py

# Drawing模块源码路径：
SOURCE_DIR = ${CMAKE_SOURCE_DIR}/src/Mod/Drawing
          = E:\Repository\FreeCAD\FreeCAD\src\Mod\Drawing

# Drawing模块构建路径：
BINARY_DIR = ${CMAKE_BINARY_DIR}/src/Mod/Drawing  
          = E:\Repository\FreeCAD\FreeCAD\build\src\Mod\Drawing
```

### AST/YAPTU执行时的工作目录

```bash
# AST/YAPTU工具的实际执行：
工作目录: E:\Repository\FreeCAD\FreeCAD\src\Mod\Drawing\App\
执行命令: python3 E:\Repository\FreeCAD\FreeCAD\src\Tools\bindings\generate.py \
            --outputPath E:\Repository\FreeCAD\FreeCAD\build\src\Mod\Drawing\App\ \
            DrawingFeature.pyi

# 🔥 关键点：工具在源码目录下运行，输出到构建目录
```

## 📝 实际操作示例

### 完整的构建命令序列

```powershell
# 1. 打开PowerShell并导航到正确目录
cd E:\Repository\FreeCAD\FreeCAD

# 2. 验证环境
Write-Host "🔍 验证构建环境..."
if (Test-Path CMakeLists.txt) {
    Write-Host "✅ 找到主CMakeLists.txt"
} else {
    Write-Host "❌ 错误：不在FreeCAD根目录"
    exit 1
}

if (Test-Path src\Tools\bindings\generate.py) {
    Write-Host "✅ 找到AST/YAPTU工具"
} else {
    Write-Host "❌ 错误：AST/YAPTU工具缺失"
    exit 1
}

if (Test-Path src\Mod\Drawing\App\DrawingFeature.pyi) {
    Write-Host "✅ 找到Drawing接口定义文件"
} else {
    Write-Host "❌ 错误：Drawing接口文件缺失"
    exit 1
}

# 3. 配置构建
Write-Host "🏗️ 配置构建..."
cmake -B build -S . -DBUILD_DRAWING=ON

# 4. 执行构建
Write-Host "⚙️ 开始构建Drawing模块..."
cmake --build build --target Drawing DrawingGui

# 5. 验证构建结果
Write-Host "🔍 验证构建结果..."
if (Test-Path build\bin\Drawing.pyd) {
    Write-Host "✅ Drawing.pyd 构建成功"
} else {
    Write-Host "❌ Drawing.pyd 构建失败"
}

if (Test-Path build\bin\DrawingGui.pyd) {
    Write-Host "✅ DrawingGui.pyd 构建成功"
} else {
    Write-Host "❌ DrawingGui.pyd 构建失败"
}

Write-Host "🎉 构建完成！"
```

## 🔧 构建过程中的AST/YAPTU执行

### 自动执行的代码生成

```bash
# 构建过程中会看到类似这样的输出：
[  5%] Building DrawingFeaturePy.h/.cpp out of DrawingFeature.pyi
TemplateClassPyExport E:\Repository\FreeCAD\FreeCAD\build\src\Mod\Drawing\App\DrawingFeaturePy
Done generating: DrawingFeaturePy

[  6%] Building LinePy.h/.cpp out of Line.pyi
TemplateClassPyExport E:\Repository\FreeCAD\FreeCAD\build\src\Mod\Drawing\App\LinePy
Done generating: LinePy

[  7%] Building CirclePy.h/.cpp out of Circle.pyi
TemplateClassPyExport E:\Repository\FreeCAD\FreeCAD\build\src\Mod\Drawing\App\CirclePy
Done generating: CirclePy

# 🔥 这表明AST/YAPTU工具正在工作！
```

### 生成的文件位置

```bash
# AST/YAPTU生成的文件会出现在构建目录中：
build\src\Mod\Drawing\App\
├── DrawingFeaturePy.h      # 🤖 自动生成的头文件
├── DrawingFeaturePy.cpp    # 🤖 自动生成的实现文件
├── LinePy.h                # 🤖 自动生成的头文件
├── LinePy.cpp              # 🤖 自动生成的实现文件
├── CirclePy.h              # 🤖 自动生成的头文件
└── CirclePy.cpp            # 🤖 自动生成的实现文件
```

## 🚨 常见错误和解决方法

### 错误1: 不在正确目录
```bash
# 错误信息：
CMake Error: The source directory "E:/Repository/FreeCAD/FreeCAD/src/Mod/Drawing" 
does not contain a CMakeLists.txt file.

# 解决方法：
cd E:\Repository\FreeCAD\FreeCAD  # 切换到根目录
cmake -B build -S .               # 重新运行配置
```

### 错误2: AST/YAPTU工具找不到
```bash
# 错误信息：
CMake Error: Cannot find Python3_EXECUTABLE or generate.py

# 解决方法：
# 1. 确认Python3已安装
python3 --version

# 2. 确认工具存在
ls src\Tools\bindings\generate.py

# 3. 如果工具缺失，检查FreeCAD源代码完整性
git status  # 检查是否有缺失文件
```

### 错误3: BUILD_DRAWING选项未启用
```bash
# 如果Drawing模块没有被构建，可能需要显式启用：
cmake -B build -S . -DBUILD_DRAWING=ON -DBUILD_GUI=ON
```

## 🎯 验证构建成功

### 检查生成的文件

```bash
# 在FreeCAD根目录下检查：
ls build\src\Mod\Drawing\App\*Py.h      # 应该看到生成的头文件
ls build\src\Mod\Drawing\App\*Py.cpp    # 应该看到生成的实现文件
ls build\bin\Drawing*.pyd               # 应该看到编译的Python模块
```

### 测试Python导入

```bash
# 启动FreeCAD Python控制台测试
cd build\bin
.\FreeCAD.exe -c

# 在FreeCAD控制台中：
>>> import Drawing
>>> print("Drawing模块导入成功！")
>>> doc = App.newDocument()
>>> line = Drawing.makeLine(App.Vector(0,0,0), App.Vector(10,5,0))
>>> print(f"创建直线成功，长度: {line.Length}")
```

## 🎉 总结

### 关键要点

1. **🎯 构建位置**: 必须在FreeCAD源代码根目录（包含主CMakeLists.txt的目录）
2. **🔧 工具路径**: AST/YAPTU工具在 `src/Tools/bindings/` 中
3. **📁 输出位置**: 生成的代码在 `build/src/Mod/Drawing/` 中
4. **⚙️ 自动执行**: CMake会自动调用AST/YAPTU工具

### 构建命令总结

```bash
# 🔥 标准构建流程（在FreeCAD根目录下）：
cd E:\Repository\FreeCAD\FreeCAD        # 1. 切换到根目录
cmake -B build -S . -DBUILD_DRAWING=ON # 2. 配置构建
cmake --build build --target Drawing   # 3. 构建Drawing模块

# 🔥 这个流程会自动：
# - 运行AST解析器分析.pyi文件  
# - 使用YAPTU模板生成C++绑定代码
# - 编译所有C++代码
# - 生成可用的Python模块
```

记住：**构建必须在FreeCAD根目录下进行**，这样CMake才能找到所有必要的工具和依赖项，AST/YAPTU工具才能正确执行！🚀
