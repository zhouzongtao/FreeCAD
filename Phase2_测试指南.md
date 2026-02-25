# Phase 2 当前版本测试指南

## 编译状态

✅ **编译成功**
- 时间：2026-01-21 11:58:06
- 目标：FreeCADGui.dll
- 状态：无错误

## 当前功能

### ✅ 已实现
1. OsgVerse 后端切换
2. Part 对象检测（通过属性名称）
3. 占位符球体渲染
4. 材质和颜色应用
5. 透明度支持
6. 日志输出

### ⚠ 未实现
1. 真实几何体转换（Shape 提取）
2. 边缘渲染
3. 不同显示模式

## 测试步骤

### 方法 1：使用完整测试脚本

```bash
# 在 FreeCAD Python 控制台中运行
exec(open('test_phase2_current_version.py').read())
```

**预期结果**：
- 创建 3 个对象（Box, Cylinder, Sphere）
- 切换到 OsgVerse 后端
- 显示 3 个红色球体（占位符）
- 每个球体有不同颜色（红/绿/蓝）
- 蓝色球体半透明

### 方法 2：使用简单测试脚本

```bash
# 在 FreeCAD Python 控制台中运行
exec(open('test_simple_switch.py').read())
```

**预期结果**：
- 创建 1 个 Box
- 切换到 OsgVerse
- 显示 1 个红色球体

### 方法 3：手动测试

1. **启动 FreeCAD**
   ```bash
   cd build/bin
   ./FreeCAD.exe
   ```

2. **创建对象**
   - File → New
   - Part → Box

3. **切换后端**
   - 在 Python 控制台输入：
   ```python
   import FreeCADGui
   FreeCADGui.switchRenderBackend(2)
   ```

4. **观察结果**
   - 应该看到红色球体
   - 不应该崩溃

## 检查点

### ✅ 成功标准

1. **不崩溃**
   - FreeCAD 启动正常
   - 切换后端不崩溃
   - 可以正常操作

2. **显示占位符**
   - 看到红色球体
   - 球体位置正确
   - 可以旋转视图

3. **材质应用**
   - 球体有颜色
   - 透明度生效
   - 光照正常

4. **日志输出**
   - 控制台有 `[OsgVerse]` 日志
   - 看到 "Found Shape property" 消息
   - 看到 "Found Part::PropertyPartShape" 消息
   - 看到 "Using placeholder sphere" 消息

### ❌ 失败情况

1. **崩溃**
   - 切换后端时崩溃
   - 创建对象时崩溃
   - 操作视图时崩溃

2. **黑屏**
   - 切换后什么都看不到
   - 视图完全黑色

3. **错误消息**
   - 控制台有错误信息
   - 弹出错误对话框

## 日志分析

### 正常日志示例

```
[OsgVerse] Creating OsgVerse viewer (Phase 1 - Placeholder Rendering)
[OsgVerse] Viewer initialized successfully
[OsgVerse] Adding ViewProvider (Box)
[OsgVerse] Found Shape property for Box, type: Part::PropertyPartShape
[OsgVerse] Found Part::PropertyPartShape for Box, but safe extraction not yet implemented
[OsgVerse] Using placeholder sphere for now
[OsgVerse] Using placeholder sphere for Box
[OsgVerse] Applied material: color(1.00, 0.00, 0.00), transparency=0.00
[OsgVerse] Added node to container (total children: 1)
[OsgVerse] ViewProvider added successfully (placeholder geometry)
```

### 关键日志说明

| 日志消息 | 含义 |
|---------|------|
| `Creating OsgVerse viewer` | 后端初始化开始 |
| `Viewer initialized successfully` | 后端初始化成功 |
| `Found Shape property` | 检测到 Part 对象 |
| `Found Part::PropertyPartShape` | 确认是 Part 对象 |
| `safe extraction not yet implemented` | Shape 提取未实现（预期） |
| `Using placeholder sphere` | 使用占位符（预期） |
| `Applied material` | 材质应用成功 |
| `ViewProvider added successfully` | 对象添加成功 |

## 已知限制

### 当前版本限制

1. **只显示占位符球体**
   - 所有 Part 对象都显示为红色球体
   - 不是真实的几何形状
   - 这是预期行为

2. **颜色应用到球体**
   - 虽然是占位符，但颜色会正确应用
   - 可以验证材质系统工作正常

3. **无边缘显示**
   - 只有实体渲染
   - 没有线框模式

### 下一步需要实现

1. **Python API 桥接**
   - 提取真实 Shape
   - 转换为 OSG 几何体

2. **真实几何体渲染**
   - Box 显示为立方体
   - Cylinder 显示为圆柱
   - Sphere 显示为球体

## 故障排除

### 问题 1：找不到 FreeCADGui.dll

**症状**：
```
ImportError: DLL load failed
```

**解决**：
```bash
# 检查 DLL 是否存在
ls build/bin/FreeCADGui.dll

# 如果不存在，重新编译
cmake --build build --config Release --target FreeCADGui
```

### 问题 2：切换后端失败

**症状**：
```python
FreeCADGui.switchRenderBackend(2)
# 抛出异常
```

**解决**：
1. 检查 BUILD_WITH_OSGVERSE 是否启用
2. 检查 OSG 库是否正确安装
3. 查看控制台错误消息

### 问题 3：看不到任何对象

**症状**：
- 切换后端成功
- 但 3D 视图是空的

**可能原因**：
1. 对象未添加到场景
2. 相机位置不对
3. 光照问题

**解决**：
```python
# 重置视图
FreeCADGui.SendMsgToActiveView("ViewFit")

# 检查对象数量
import FreeCAD
doc = FreeCAD.ActiveDocument
print(len(doc.Objects))  # 应该 > 0
```

### 问题 4：崩溃

**症状**：
- FreeCAD 突然关闭
- 没有错误消息

**调试步骤**：
1. 在调试器中运行 FreeCAD
2. 查看崩溃堆栈
3. 检查日志文件
4. 报告问题

## 性能测试

### 简单场景
- 1-10 个对象
- 应该流畅（60 FPS）

### 中等场景
- 10-100 个对象
- 应该可用（30+ FPS）

### 大型场景
- 100+ 个对象
- 可能较慢（占位符球体也有开销）

## 下一步

### 立即可做
1. ✅ 运行测试脚本
2. ✅ 验证基本功能
3. ✅ 检查日志输出
4. ✅ 报告问题（如果有）

### 后续开发
1. ⏳ 实现 Python API 桥接
2. ⏳ 提取真实 Shape
3. ⏳ 转换真实几何体
4. ⏳ 测试真实渲染

## 总结

当前版本是一个**功能验证版本**：
- ✅ 证明 OsgVerse 后端可以工作
- ✅ 证明可以检测 Part 对象
- ✅ 证明材质系统正常
- ⚠ 真实几何体转换待实现

**测试目标**：验证基础架构正确，为下一步开发做准备。

---

**准备好了吗？运行测试脚本吧！** 🚀
