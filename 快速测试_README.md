# 快速测试指南

## 🚀 快速开始

### 方法 1：最简单的测试（推荐）

1. **启动 FreeCAD**
   ```bash
   cd build/bin
   ./FreeCAD.exe
   ```

2. **在 Python 控制台运行**
   ```python
   exec(open('../../test_simple_switch.py').read())
   ```

3. **预期结果**
   - 看到一个红色球体
   - 没有崩溃
   - ✅ 成功！

### 方法 2：完整测试

```python
exec(open('../../test_phase2_current_version.py').read())
```

预期：3 个不同颜色的球体（红/绿/蓝）

## 📋 测试文件

| 文件 | 用途 | 复杂度 |
|------|------|--------|
| `test_simple_switch.py` | 最简单测试 | ⭐ |
| `test_phase2_current_version.py` | 完整功能测试 | ⭐⭐⭐ |
| `test_phase2_placeholder.py` | 占位符测试 | ⭐⭐ |
| `Phase2_测试指南.md` | 详细测试文档 | 📖 |

## ✅ 成功标准

- [ ] FreeCAD 启动正常
- [ ] 可以创建 Part 对象
- [ ] 切换到 OsgVerse 不崩溃
- [ ] 看到红色球体（占位符）
- [ ] 控制台有 `[OsgVerse]` 日志

## ⚠ 当前限制

**所有对象都显示为红色球体**
- 这是预期行为
- 真实几何体转换尚未实现
- 下一步：实现 Python API 桥接

## 🐛 如果出错

1. 检查编译是否成功
2. 查看控制台错误消息
3. 阅读 `Phase2_测试指南.md`
4. 报告问题

## 📊 当前状态

```
Phase 2 进度：
├─ ✅ 编译成功
├─ ✅ 后端切换
├─ ✅ 对象检测
├─ ✅ 占位符渲染
├─ ✅ 材质应用
└─ ⏳ 真实几何体（待实现）
```

## 🎯 下一步

测试成功后，我们将实现：
1. Python API 桥接
2. Shape 提取
3. 真实几何体转换

---

**现在就开始测试吧！** 🚀
