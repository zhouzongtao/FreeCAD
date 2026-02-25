# OsgVerse 快速启用指南

## 🚀 一键启用 OsgVerse

### 在 FreeCAD Python 控制台中运行：

```python
import FreeCADGui

# 1. 初始化 RenderManager（注册 OsgVerse）
FreeCADGui.initializeRenderManager()

# 2. 切换到 OsgVerse
FreeCADGui.switchRenderBackend(2)

# 3. 验证
print("当前后端:", FreeCADGui.getCurrentRenderBackend())  # 应该是 2
print("渲染器:", FreeCADGui.getRendererInfo())  # 应该显示 OsgVerse
```

## 📋 完整的 Python API

### 初始化
```python
# 初始化 RenderManager（必须先调用）
FreeCADGui.initializeRenderManager() -> bool
```

### 查询后端
```python
# 获取当前后端：0=None, 1=Coin3D, 2=OsgVerse
FreeCADGui.getCurrentRenderBackend() -> int

# 检查后端是否可用
FreeCADGui.isRenderBackendAvailable(2) -> bool  # 2 = OsgVerse

# 获取渲染器信息
FreeCADGui.getRendererInfo() -> str
```

### 切换后端
```python
# 切换到指定后端
FreeCADGui.switchRenderBackend(2) -> bool  # 2 = OsgVerse
FreeCADGui.switchRenderBackend(1) -> bool  # 1 = Coin3D
```

### 统计信息
```python
# 获取渲染统计
stats = FreeCADGui.getRenderStats()
print(stats)  # {'frameCount': ..., 'fps': ..., ...}

# 重置统计
FreeCADGui.resetRenderStats()
```

## 🎯 后端类型

| 值 | 名称 | 说明 |
|----|------|------|
| 0 | None | 无渲染后端 |
| 1 | Coin3D | 默认后端（稳定） |
| 2 | OsgVerse | 新后端（实验性，现代特性） |

## 📝 创建启动宏

创建文件 `InitOsgVerse.FCMacro`：

```python
"""
自动初始化并切换到 OsgVerse 渲染后端
"""
import FreeCADGui

# 初始化 RenderManager
if not FreeCADGui.initializeRenderManager():
    print("❌ 初始化失败")
    exit()

print("✅ RenderManager 初始化成功")

# 检查 OsgVerse 是否可用
if not FreeCADGui.isRenderBackendAvailable(2):
    print("❌ OsgVerse 不可用")
    print("   请检查 OSG DLL 是否已复制到 build/bin 目录")
    exit()

print("✅ OsgVerse 可用")

# 切换到 OsgVerse
if not FreeCADGui.switchRenderBackend(2):
    print("❌ 切换失败")
    exit()

print("✅ 已切换到 OsgVerse")
print(f"   渲染器: {FreeCADGui.getRendererInfo()}")
```

## 🔧 自动启动配置

### 方法 1: 启动脚本
创建 `~/.FreeCAD/Macro/start.py`：

```python
import FreeCADGui

# 自动初始化并切换到 OsgVerse
FreeCADGui.initializeRenderManager()
if FreeCADGui.isRenderBackendAvailable(2):
    FreeCADGui.switchRenderBackend(2)
```

### 方法 2: 用户配置
在 FreeCAD 的 Python 控制台中：

```python
# 保存当前后端选择
import FreeCAD
param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
param.SetInt("RenderBackend", 2)  # 2 = OsgVerse
```

## 🐛 故障排除

### 问题 1: `initializeRenderManager()` 不存在
```python
>>> FreeCADGui.initializeRenderManager()
AttributeError: module 'FreeCADGui' has no attribute 'initializeRenderManager'
```

**解决方案**：
- 确认已重新编译 FreeCADGui
- 检查是否使用了正确的 FreeCAD 可执行文件

### 问题 2: OsgVerse 不可用
```python
>>> FreeCADGui.isRenderBackendAvailable(2)
False
```

**解决方案**：
1. 检查 OSG DLL 是否在 `build/bin` 目录中
2. 运行 `copy_osg_runtime.ps1` 复制 DLL
3. 检查编译时是否启用了 `BUILD_WITH_OSGVERSE`

### 问题 3: 切换失败
```python
>>> FreeCADGui.switchRenderBackend(2)
False
```

**解决方案**：
1. 查看 Report View 中的错误消息
2. 确认 OsgVerse 可用：`FreeCADGui.isRenderBackendAvailable(2)`
3. 检查 OSG 库版本兼容性

### 问题 4: 初始化返回 False
```python
>>> FreeCADGui.initializeRenderManager()
False
```

**解决方案**：
1. 查看 Report View 中的详细错误
2. 检查 `registerOsgVerseEngine()` 是否被调用
3. 验证 OsgVerse 引擎的实现

## 📊 验证安装

运行以下完整测试：

```python
import FreeCADGui

print("=" * 60)
print("OsgVerse 安装验证")
print("=" * 60)

# 1. 检查函数
print("\n1. 检查 API...")
funcs = ['initializeRenderManager', 'getCurrentRenderBackend', 
         'isRenderBackendAvailable', 'switchRenderBackend', 
         'getRendererInfo', 'getRenderStats', 'resetRenderStats']
for func in funcs:
    status = "✓" if hasattr(FreeCADGui, func) else "✗"
    print(f"   {status} {func}")

# 2. 初始化
print("\n2. 初始化 RenderManager...")
if FreeCADGui.initializeRenderManager():
    print("   ✓ 初始化成功")
else:
    print("   ✗ 初始化失败")
    exit()

# 3. 检查后端
print("\n3. 检查可用后端...")
backends = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
for id, name in backends.items():
    available = FreeCADGui.isRenderBackendAvailable(id)
    status = "✓" if available else "✗"
    print(f"   {status} {name} (id={id})")

# 4. 当前状态
print("\n4. 当前状态...")
current = FreeCADGui.getCurrentRenderBackend()
print(f"   当前后端: {current} ({backends.get(current, 'Unknown')})")
print(f"   渲染器: {FreeCADGui.getRendererInfo()}")

# 5. 测试切换
if FreeCADGui.isRenderBackendAvailable(2):
    print("\n5. 测试切换到 OsgVerse...")
    if FreeCADGui.switchRenderBackend(2):
        print("   ✓ 切换成功")
        print(f"   当前后端: {FreeCADGui.getCurrentRenderBackend()}")
        print(f"   渲染器: {FreeCADGui.getRendererInfo()}")
    else:
        print("   ✗ 切换失败")

print("\n" + "=" * 60)
print("验证完成")
print("=" * 60)
```

## 🎓 使用示例

### 示例 1: 性能比较
```python
import FreeCADGui
import time

# 初始化
FreeCADGui.initializeRenderManager()

# 测试 Coin3D
FreeCADGui.switchRenderBackend(1)
FreeCADGui.resetRenderStats()
time.sleep(5)
coin3d_stats = FreeCADGui.getRenderStats()

# 测试 OsgVerse
FreeCADGui.switchRenderBackend(2)
FreeCADGui.resetRenderStats()
time.sleep(5)
osgverse_stats = FreeCADGui.getRenderStats()

# 比较
print("Coin3D FPS:", coin3d_stats['fps'])
print("OsgVerse FPS:", osgverse_stats['fps'])
```

### 示例 2: 条件切换
```python
import FreeCADGui

# 初始化
FreeCADGui.initializeRenderManager()

# 根据场景复杂度选择后端
def choose_backend(object_count):
    if object_count > 1000:
        # 大场景使用 OsgVerse
        if FreeCADGui.isRenderBackendAvailable(2):
            FreeCADGui.switchRenderBackend(2)
            return "OsgVerse"
    # 默认使用 Coin3D
    FreeCADGui.switchRenderBackend(1)
    return "Coin3D"

backend = choose_backend(len(FreeCAD.ActiveDocument.Objects))
print(f"使用后端: {backend}")
```

### 示例 3: 错误处理
```python
import FreeCADGui

def safe_switch_to_osgverse():
    """安全地切换到 OsgVerse，带完整错误处理"""
    try:
        # 初始化
        if not FreeCADGui.initializeRenderManager():
            print("错误: 初始化失败")
            return False
        
        # 检查可用性
        if not FreeCADGui.isRenderBackendAvailable(2):
            print("错误: OsgVerse 不可用")
            print("提示: 请检查 OSG DLL 是否已安装")
            return False
        
        # 切换
        if not FreeCADGui.switchRenderBackend(2):
            print("错误: 切换失败")
            return False
        
        # 验证
        if FreeCADGui.getCurrentRenderBackend() != 2:
            print("错误: 切换后后端不正确")
            return False
        
        print(f"成功: 已切换到 {FreeCADGui.getRendererInfo()}")
        return True
        
    except Exception as e:
        print(f"异常: {e}")
        return False

# 使用
if safe_switch_to_osgverse():
    print("OsgVerse 已启用")
else:
    print("使用默认的 Coin3D 后端")
```

## 📚 更多信息

- **详细测试指南**: `Phase5_手动初始化测试指南.md`
- **项目状态**: `Phase5_最终状态总结.md`
- **完整文档**: `切换OsgVerse渲染引擎指南.md`

---

**快速开始**: 在 FreeCAD Python 控制台中运行：
```python
import FreeCADGui
FreeCADGui.initializeRenderManager()
FreeCADGui.switchRenderBackend(2)
```

就这么简单！🎉
