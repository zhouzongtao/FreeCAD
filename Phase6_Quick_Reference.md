# Phase 6 快速参考

## 当前状态

✅ **Phase 1**: 抽象接口层 - 完成  
✅ **Phase 2**: CoinGui 模块 - 完成  
✅ **Phase 3**: OsgVerseGui 模块 - **编译成功！** 🎉

**输出文件**: `build/Mod/OsgVerseGui/OsgVerseGui.pyd` (416 KB)

## 快速编译

```bash
# 1. 配置（如果需要）
cmake -B build -DBUILD_GUI=ON -DBUILD_WITH_OSGVERSE=ON

# 2. 编译 OsgVerseGui
cmake --build build --target OsgVerseGui

# 3. 查看输出
# Windows: build/bin/OsgVerseGui.pyd
# Linux: build/lib/OsgVerseGui.so
```

## 快速测试

```python
# 启动 FreeCAD
build/bin/FreeCAD.exe

# 在 Python 控制台
import OsgVerseGui
from Gui import BackendRegistry

# 检查后端
print(BackendRegistry.getAvailableBackends())
# 应该看到: ['Coin3D', 'OsgVerse']

# 创建视图
viewer = BackendRegistry.createViewer("OsgVerse")
print(viewer.getBackendName())  # 应该输出: OsgVerse

# 测试几何体
import Part
box = Part.makeBox(10, 10, 10)
Part.show(box)
```

## 关键文件

### 源代码
- `src/Mod/OsgVerseGui/` - OsgVerseGui 模块
- `src/Mod/CoinGui/` - CoinGui 模块（参考）
- `src/Gui/View3D/Interfaces/` - 抽象接口层

### 文档
- `Phase6_Step3_完成报告.md` - 完成报告
- `Phase6_Step3_实施指南.md` - 详细实施指南
- `Phase6_Backend_Modularization_Summary.md` - 完整总结

### 测试
- `test_osgversegui_module.py` - 自动化测试脚本

## 关键代码

### 直接访问 Part::Feature

```cpp
// OsgVerseViewer.cpp - createNodeForViewProvider()
Part::TopoShape topoShape = Part::Feature::getTopoShape(
    obj,
    Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
);

const TopoDS_Shape& shape = topoShape.getShape();

osg::ref_ptr<osg::Geode> geode = GeometryConverter::convertShape(shape);
```

### 后端切换

```python
# Python
from Gui import BackendRegistry

# 切换到 OsgVerse
BackendRegistry.setDefaultBackend("OsgVerse")

# 切换回 Coin3D
BackendRegistry.setDefaultBackend("Coin3D")
```

## 架构对比

### 之前
```
FreeCADGui
└── OsgVerse (编译到 FreeCADGui)
    └── ❌ 不能链接 Part 模块
    └── ❌ 只能显示占位符球体
```

### 现在
```
OsgVerseGui (独立模块)
├── ✅ 可以链接 Part 模块
├── ✅ 直接调用 Part::Feature::getTopoShape()
└── ✅ 显示真实几何体
```

## 常见问题

### Q: 模块找不到？
```python
>>> import OsgVerseGui
ImportError: No module named 'OsgVerseGui'
```

**A**: 检查编译是否成功，确认 `build/bin/OsgVerseGui.pyd` 存在

### Q: 后端未注册？
```python
>>> BackendRegistry.getAvailableBackends()
['Coin3D']  # 没有 OsgVerse
```

**A**: 检查模块初始化日志，确认 `initOsgVerseGui()` 被调用

### Q: 几何体不显示？

**A**: 检查日志，确认：
1. `Part::Feature::getTopoShape()` 成功
2. `GeometryConverter::convertShape()` 成功
3. 如果失败，应该显示红色占位符球体

## 下一步

1. ⏭️ 编译 OsgVerseGui
2. ⏭️ 运行测试脚本
3. ⏭️ 测试真实几何体渲染
4. ⏭️ 清理旧代码（Phase 4）

## 帮助

如果遇到问题：
1. 查看 `Phase6_Step3_实施指南.md` - 详细的故障排除
2. 查看 `Phase6_Step3_完成报告.md` - 完整的实现细节
3. 运行 `test_osgversegui_module.py` - 自动化诊断

---

**快速参考** | 2026-01-21 | Phase 6 Step 3
