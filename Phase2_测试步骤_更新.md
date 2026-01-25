# Phase 2: 事件处理 - 测试步骤（更新版）

**日期**: 2026-01-21  
**更新**: 添加了 Init.py 以确保模块自动加载

---

## 🔧 问题修复

### 问题
```
BackendRegistry: Backend 'OsgVerse' not found
```

### 原因
OsgVerseGui 模块没有被 FreeCAD 自动加载。

### 解决方案
✅ 已添加 `Init.py` 文件到 OsgVerseGui 模块  
✅ 已更新 CMakeLists.txt 以安装 Init.py  
✅ 已重新编译模块

---

## 🚀 测试步骤

### 步骤 1: 启动 FreeCAD GUI

```cmd
cd E:\Repository\FreeCAD\FreeCAD
.\build\bin\FreeCAD.exe
```

### 步骤 2: 验证后端注册

在 FreeCAD Python 控制台中运行：

```python
# 快速验证后端是否注册
exec(open(r'E:\Repository\FreeCAD\FreeCAD\test_osgverse_registration.py', encoding='utf-8').read())
```

**预期输出**：
```
OsgVerse Backend Registration Test
============================================================
[Step 1] Checking available backends...
Available backends: ['Coin3D', 'OsgVerse']
[SUCCESS] ✅ OsgVerse backend is registered!

[Step 2] Getting backend info...
  Name: OsgVerse
  Version: OsgVerse + OSG 3.6+
  Available: True

[Step 3] Creating viewer...
[SUCCESS] ✅ Viewer created!
  Type: Viewer3DWrapper
  Backend: OsgVerse

============================================================
ALL TESTS PASSED! ✅
============================================================
```

### 步骤 3: 运行 Phase 2 事件处理测试

如果步骤 2 成功，继续运行完整测试：

```python
# 运行完整的 Phase 2 测试
exec(open(r'E:\Repository\FreeCAD\FreeCAD\test_phase2_events.py', encoding='utf-8').read())
```

---

## 🐛 如果仍然出现 "Backend not found" 错误

### 方法 1: 手动导入模块

```python
# 手动导入 OsgVerseGui 模块
import OsgVerseGui

# 然后检查后端
from FreeCADGui import BackendRegistry
print(BackendRegistry.getAvailableBackends())
```

### 方法 2: 检查模块文件

```python
# 检查模块文件是否存在
import os
module_path = r"E:\Repository\FreeCAD\FreeCAD\build\Mod\OsgVerseGui\OsgVerseGui.pyd"
print(f"Module exists: {os.path.exists(module_path)}")

init_path = r"E:\Repository\FreeCAD\FreeCAD\build\Mod\OsgVerseGui\Init.py"
print(f"Init.py exists: {os.path.exists(init_path)}")
```

### 方法 3: 检查 Python 路径

```python
# 检查模块目录是否在 Python 路径中
import sys
mod_dir = r"E:\Repository\FreeCAD\FreeCAD\build\Mod\OsgVerseGui"
print(f"Module dir in sys.path: {mod_dir in sys.path}")

# 如果不在，手动添加
if mod_dir not in sys.path:
    sys.path.insert(0, mod_dir)
    print("Added to sys.path")
```

### 方法 4: 查看控制台输出

检查 FreeCAD 控制台是否有以下消息：
```
OsgVerseGui: Initializing module
OsgVerseGui: Backend registered successfully
OsgVerseGui: Module initialized
OsgVerseGui: Python module created
OsgVerseGui: Python Init.py executed
```

如果没有这些消息，说明模块没有被加载。

---

## ✅ 成功标志

当你看到以下输出时，说明一切正常：

```
[SUCCESS] ✅ OsgVerse backend is registered!
[SUCCESS] ✅ Viewer created!
```

然后你可以继续进行 Phase 2 的手动交互测试。

---

## 📋 Phase 2 手动测试清单

一旦后端注册成功，按照以下清单测试：

### 鼠标测试
- [ ] 左键拖拽 → 旋转相机
- [ ] 中键拖拽 → 平移相机
- [ ] 右键拖拽 → 缩放相机
- [ ] 鼠标滚轮 → 缩放

### 键盘测试
- [ ] 方向键 → 旋转相机
- [ ] +/- 键 → 缩放
- [ ] V 键 → 控制台消息
- [ ] Home 键 → 控制台消息

### 质量检查
- [ ] 平滑移动，无卡顿
- [ ] 无延迟
- [ ] 无崩溃
- [ ] 无错误消息

---

## 📞 需要帮助？

如果遇到问题：

1. **检查编译**：确认 OsgVerseGui.pyd 和 Init.py 都存在
2. **检查控制台**：查看是否有错误消息
3. **手动导入**：尝试 `import OsgVerseGui`
4. **运行诊断**：使用 `diagnose_osgverse_module.py`

---

## 🎯 快速命令参考

```python
# 1. 验证后端注册
exec(open(r'E:\Repository\FreeCAD\FreeCAD\test_osgverse_registration.py', encoding='utf-8').read())

# 2. 运行 Phase 2 测试
exec(open(r'E:\Repository\FreeCAD\FreeCAD\test_phase2_events.py', encoding='utf-8').read())

# 3. 手动导入（如果需要）
import OsgVerseGui
from FreeCADGui import BackendRegistry
print(BackendRegistry.getAvailableBackends())
```

---

**准备好了？启动 FreeCAD GUI 并运行测试！** 🚀
