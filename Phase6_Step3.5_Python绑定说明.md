# Phase 6 Step 3.5: BackendRegistry Python 绑定

## 当前状态

✅ **OsgVerseGui 模块编译成功**  
⏭️ **BackendRegistry Python 绑定需要编译**

## 问题

当前 `BackendRegistry` 没有 Python 绑定，所以无法在 Python 中访问：

```python
>>> from Gui import BackendRegistry
NameError: name 'BackendRegistry' is not defined
```

## 解决方案

我已经创建了 Python 绑定代码，但需要重新编译 FreeCADGui。

### 新增文件

1. **`src/Gui/View3D/Interfaces/BackendRegistryPy.cpp`**
   - BackendRegistry 的 Python 包装器
   - 提供以下 Python 方法：
     - `getAvailableBackends()` - 获取可用后端列表
     - `isBackendAvailable(name)` - 检查后端是否可用
     - `getDefaultBackend()` - 获取默认后端
     - `setDefaultBackend(name)` - 设置默认后端
     - `getBackendInfo(name)` - 获取后端信息
     - `createViewer(name)` - 创建视图（待实现）
     - `createDefaultViewer()` - 创建默认视图（待实现）

### 修改文件

1. **`src/Gui/View3D/CMakeLists.txt`**
   - 添加 `BackendRegistryPy.cpp` 到源文件列表

2. **`src/Gui/View3D/Interfaces/BackendRegistry.h`**
   - 添加 `initBackendRegistryPython()` 函数声明

3. **`src/Gui/Application.cpp`**
   - 包含 `BackendRegistry.h`
   - 在模块初始化时调用 `initBackendRegistryPython()`

## 编译步骤

### 步骤 1: 关闭 FreeCAD

**重要**：必须先关闭 FreeCAD，否则无法重新编译 FreeCADGui.dll

### 步骤 2: 重新编译 FreeCADGui

```bash
# Windows (PowerShell)
cmake --build build --target FreeCADGui --config Release
```

**预期输出**：
```
Automatic MOC and UIC for target FreeCADGui
BackendRegistryPy.cpp
...
FreeCADGui.vcxproj -> E:\Repository\FreeCAD\FreeCAD\build\bin\FreeCADGui.dll
```

### 步骤 3: 启动 FreeCAD 并测试

```bash
build/bin/FreeCAD.exe
```

在 Python 控制台中：

```python
# 测试 BackendRegistry
from Gui import BackendRegistry

# 获取可用后端
backends = BackendRegistry.getAvailableBackends()
print(f"Available backends: {backends}")
# 应该输出: ['Coin3D']

# 导入 OsgVerseGui
import OsgVerseGui

# 再次检查后端
backends = BackendRegistry.getAvailableBackends()
print(f"Available backends: {backends}")
# 应该输出: ['Coin3D', 'OsgVerse']

# 获取后端信息
info = BackendRegistry.getBackendInfo("OsgVerse")
print("OsgVerse info:")
for key, value in info.items():
    print(f"  {key}: {value}")

# 获取默认后端
default = BackendRegistry.getDefaultBackend()
print(f"Default backend: {default}")
# 应该输出: Coin3D

# 切换默认后端
BackendRegistry.setDefaultBackend("OsgVerse")
print(f"New default: {BackendRegistry.getDefaultBackend()}")
# 应该输出: OsgVerse
```

## Python API 文档

### BackendRegistry.getAvailableBackends()

返回所有可用后端的名称列表。

**返回值**: `list[str]`

**示例**:
```python
backends = BackendRegistry.getAvailableBackends()
# ['Coin3D', 'OsgVerse']
```

### BackendRegistry.isBackendAvailable(name)

检查指定后端是否可用。

**参数**:
- `name` (str): 后端名称

**返回值**: `bool`

**示例**:
```python
if BackendRegistry.isBackendAvailable("OsgVerse"):
    print("OsgVerse is available")
```

### BackendRegistry.getDefaultBackend()

获取当前默认后端的名称。

**返回值**: `str`

**示例**:
```python
default = BackendRegistry.getDefaultBackend()
# 'Coin3D'
```

### BackendRegistry.setDefaultBackend(name)

设置默认后端。

**参数**:
- `name` (str): 后端名称

**返回值**: `bool` - 成功返回 True

**示例**:
```python
success = BackendRegistry.setDefaultBackend("OsgVerse")
if success:
    print("Default backend changed to OsgVerse")
```

### BackendRegistry.getBackendInfo(name)

获取后端的详细信息。

**参数**:
- `name` (str): 后端名称

**返回值**: `dict[str, str]`

**示例**:
```python
info = BackendRegistry.getBackendInfo("OsgVerse")
# {
#   'name': 'OsgVerse',
#   'description': 'OsgVerse rendering backend using OpenSceneGraph',
#   'version': 'OsgVerse + OSG 3.6+',
#   'priority': '5'
# }
```

### BackendRegistry.createViewer(name)

创建指定后端的视图。

**参数**:
- `name` (str): 后端名称

**返回值**: `IViewer3D` 或 `None`

**注意**: 当前返回 None，Python 包装器待实现

**示例**:
```python
viewer = BackendRegistry.createViewer("OsgVerse")
if viewer:
    print(f"Viewer created: {viewer.getBackendName()}")
```

### BackendRegistry.createDefaultViewer()

使用默认后端创建视图。

**返回值**: `IViewer3D` 或 `None`

**注意**: 当前返回 None，Python 包装器待实现

**示例**:
```python
viewer = BackendRegistry.createDefaultViewer()
```

## 临时测试方法

在 BackendRegistry Python 绑定编译之前，你可以：

### 方法 1: 直接测试模块导入

```python
# 测试 CoinGui
import CoinGui
print("CoinGui imported successfully")

# 测试 OsgVerseGui
import OsgVerseGui
print("OsgVerseGui imported successfully")
```

### 方法 2: 检查模块文件

```python
import sys
import os

# 检查 OsgVerseGui.pyd 是否存在
osgverse_path = os.path.join(sys.prefix, "Mod", "OsgVerseGui", "OsgVerseGui.pyd")
if os.path.exists(osgverse_path):
    print(f"OsgVerseGui.pyd found: {osgverse_path}")
else:
    print("OsgVerseGui.pyd not found")
```

## 下一步

1. ⏭️ **关闭 FreeCAD**
2. ⏭️ **重新编译 FreeCADGui**（包含 BackendRegistryPy.cpp）
3. ⏭️ **启动 FreeCAD 并测试 Python 绑定**
4. ⏭️ **测试 OsgVerseGui 模块加载和后端注册**

## 总结

- ✅ OsgVerseGui 模块已编译
- ✅ BackendRegistry Python 绑定代码已创建
- ⏭️ 需要重新编译 FreeCADGui
- ⏭️ 然后可以在 Python 中使用 BackendRegistry

---

**时间**: 2026-01-21  
**状态**: 等待重新编译 FreeCADGui  
**下一步**: 关闭 FreeCAD → 重新编译 → 测试

