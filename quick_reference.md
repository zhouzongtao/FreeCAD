# 快速参考手册

## 一键切换命令

### 切换到 Coin3D（真实几何体）
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\switch_to_coin3d.py', encoding='utf-8').read())
```

### 切换到 OsgVerse（红色球体占位符）
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\switch_to_osgverse.py', encoding='utf-8').read())
```

### 完整测试（包含来回切换）
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\test_backend_switch.py', encoding='utf-8').read())
```

## 简短命令

### 切换到 Coin3D
```python
import FreeCADGui; FreeCADGui.switchRenderBackend(1); FreeCADGui.SendMsgToActiveView("ViewFit")
```

### 切换到 OsgVerse
```python
import FreeCADGui; FreeCADGui.switchRenderBackend(2); FreeCADGui.SendMsgToActiveView("ViewFit")
```

### 查看当前后端
```python
import FreeCADGui; print({0:"None", 1:"Coin3D", 2:"OsgVerse"}[FreeCADGui.getCurrentRenderBackend()])
```

## 测试 Phase 1.5 优化

### 测试优化后的代码
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\test_phase1.5_optimized.py', encoding='utf-8').read())
```

## 预期效果

### Coin3D
- ✓ 真实的 3D 几何体
- ✓ 正确的颜色和材质
- ✓ 所有功能正常

### OsgVerse (Phase 1)
- ✓ 红色球体占位符（半径 5.0）
- ✓ 完整可见，无裁剪
- ✓ `[OsgVerse]` 日志前缀
- ✓ 视图控制正常

## 故障排除

### 切换失败
```python
# 检查后端可用性
import FreeCADGui
print(f"Coin3D: {FreeCADGui.isRenderBackendAvailable(1)}")
print(f"OsgVerse: {FreeCADGui.isRenderBackendAvailable(2)}")
```

### 视图黑屏
```python
# 重新适应视图
FreeCADGui.SendMsgToActiveView("ViewFit")
```

### 查看日志
- 打开 View → Panels → Report view
- 查找 `[OsgVerse]` 前缀的消息

## 文档索引

- **渲染流程图**: `rendering_mechanism.md`
- **切换指南**: `backend_switch_guide.md`
- **Phase 1.5 优化报告**: `Phase1.5_优化完成报告.md`
- **Phase 1 完成总结**: `Phase1_完成总结.md`
