# OsgVerse Backend 加载指南

## 当前状态

- ✅ `BUILD_WITH_OSGVERSE=ON` - OsgVerse渲染支持已启用
- ✅ `BUILD_OSGVERSE_GUI_MODULE=ON` - OsgVerseGui模块构建已启用（刚刚配置）

## 方法1：自动加载（推荐）

### 步骤1：确保OsgVerseGui模块已构建

```bash
# 启用模块构建
cd build/debug
cmake -DBUILD_OSGVERSE_GUI_MODULE=ON ../..

# 重新构建
pixi run build-debug
```

### 步骤2：验证模块已构建

```bash
# 检查模块文件是否存在
ls -lh build/debug/Mod/OsgVerseGui.so
```

### 步骤3：启动FreeCAD

启动FreeCAD后，Application.cpp中的代码会自动尝试导入OsgVerseGui模块：

```cpp
// 在Application.cpp中已添加
PyObject* osgVerseModule = PyImport_ImportModule("OsgVerseGui");
```

如果模块加载成功，OsgVerse backend会自动注册。

## 方法2：手动加载

### 在Python控制台中手动导入

```python
# 启动FreeCAD后，在Python控制台执行
import OsgVerseGui

# 验证backend是否已注册
# 可以通过创建新文档来测试
```

## 方法3：通过配置文件自动加载

### 创建启动脚本

在 `~/.FreeCAD/Macro/` 目录下创建 `startup.py`:

```python
# startup.py
try:
    import OsgVerseGui
    print("OsgVerse backend loaded successfully")
except ImportError as e:
    print(f"Failed to load OsgVerse backend: {e}")
```

## 验证Backend是否加载

### 方法1：检查日志

启动FreeCAD时查看控制台输出：

```
Application: Attempting to load OsgVerseGui module...
Application: OsgVerseGui module loaded successfully
OsgVerseGui: Initializing module
OsgVerseGui: Registered with ViewerFactory
```

### 方法2：在Python中检查

```python
import FreeCADGui

# 创建新文档测试
doc = FreeCAD.newDocument()

# 如果没有崩溃，说明backend处理正确
```

### 方法3：查看可用的Viewers

```python
# 检查ViewerFactory中注册的backends
# 这需要访问C++层的ViewerFactory
```

## 切换到OsgVerse Backend

### 通过RenderManager切换

```python
# 注意：这个API可能需要根据实际实现调整
import FreeCADGui
from Gui import Core

# 获取RenderManager实例
render_mgr = Core.RenderManager.instance()

# 切换到OsgVerse backend
# render_mgr.switchBackend(2)  # 2 = OsgVerse
```

## 故障排除

### 问题1：模块未找到

**症状**：
```
Application: OsgVerseGui module not available
```

**解决方案**：
1. 确认 `BUILD_OSGVERSE_GUI_MODULE=ON`
2. 重新构建项目
3. 检查 `build/debug/Mod/OsgVerseGui.so` 是否存在

### 问题2：Backend未注册

**症状**：
```
Viewer backend not registered: 2
```

**解决方案**：
1. 确认OsgVerseGui模块已加载
2. 检查模块初始化函数是否被调用
3. 查看是否有编译错误

### 问题3：创建视图时崩溃

**症状**：创建新文档或打开文件时崩溃

**解决方案**：
这个问题已经通过最新的提交修复。确保你使用的是最新代码：
- 添加了backend注册检查
- 自动回退到Coin3D渲染器

## 技术细节

### OsgVerseGui模块结构

```
src/Mod/OsgVerseGui/
├── AppOsgVerseGui.cpp      # 模块初始化和backend注册
├── OsgVerseViewer.cpp      # IViewer3D接口实现
├── OsgVerseWidget.cpp      # Qt OpenGL widget
├── GeometryConverter.cpp   # 几何转换
├── OsgVersePostProcess.cpp # 后处理效果
├── OsgVerseNaviCube.cpp    # 导航立方体
└── OsgVerseBackground.cpp  # 背景渲染
```

### Backend注册流程

1. **模块加载**：Python导入OsgVerseGui模块
2. **初始化函数**：调用 `initOsgVerseGui()`
3. **注册Viewer**：调用 `ViewerFactory::registerCreator()`
4. **创建Lambda**：注册创建OsgVerseViewer的工厂函数

```cpp
// 在AppOsgVerseGui.cpp中
ViewerFactory::registerCreator(
    Render::BackendType::OsgVerse,
    [](QWidget* parent, const QOpenGLWidget* shareWidget) {
        return std::make_unique<OsgVerseViewer>(parent);
    }
);
```

## 性能优化建议

1. **共享OpenGL上下文**：多个视图共享同一个OpenGL上下文可以提高性能
2. **启用VBO**：确保使用Vertex Buffer Objects
3. **后处理效果**：根据需要启用/禁用后处理效果

## 相关文件

- `src/Gui/Application.cpp` - 自动加载OsgVerseGui模块
- `src/Gui/Document.cpp` - Backend注册检查和回退逻辑
- `src/Mod/OsgVerseGui/` - OsgVerseGui模块源代码
- `src/Gui/View3D/ViewerFactory.cpp` - Viewer工厂实现
