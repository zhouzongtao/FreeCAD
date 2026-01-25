# Phase 2: 事件处理 - 快速参考

---

## 🚀 快速启动测试

### 方法 1: 使用批处理文件
```cmd
test_phase2.cmd
```

### 方法 2: 手动启动
```cmd
cd E:\Repository\FreeCAD\FreeCAD
.\build\bin\FreeCAD.exe
```

然后在 Python 控制台中：
```python
exec(open(r'E:\Repository\FreeCAD\FreeCAD\test_phase2_events.py', encoding='utf-8').read())
```

---

## 📋 测试清单（简化版）

### 鼠标测试
- [ ] **左键拖拽** → 旋转
- [ ] **中键拖拽** → 平移
- [ ] **右键拖拽** → 缩放
- [ ] **滚轮** → 缩放

### 键盘测试
- [ ] **方向键** → 旋转
- [ ] **+/- 键** → 缩放
- [ ] **V 键** → 控制台消息
- [ ] **Home 键** → 控制台消息

### 质量检查
- [ ] 平滑移动
- [ ] 无延迟
- [ ] 无崩溃
- [ ] 无错误

---

## ✅ 完成标准

所有测试项通过 → Phase 2 完成 ✅

---

## 📚 详细文档

- **完整测试指南**: `Phase2_测试指南.md`
- **实现报告**: `Phase2_Implementation_Complete.md`
- **中文总结**: `Phase2_完成总结.md`
- **规范文档**: `.kiro/specs/osgverse-rendering/phase2-event-handling.md`

---

## 🐛 遇到问题？

1. 检查控制台错误消息
2. 确认模块已编译：`OsgVerseGui.pyd`
3. 确保使用 FreeCAD GUI（不是 FreeCADCmd）
4. 点击 3D 视图获得焦点（键盘事件）

---

## 📞 测试脚本路径

```
完整测试: E:\Repository\FreeCAD\FreeCAD\test_phase2_events.py
简单测试: E:\Repository\FreeCAD\FreeCAD\test_phase2_simple.py
```

---

**准备好了？运行 `test_phase2.cmd` 开始测试！** 🎉
