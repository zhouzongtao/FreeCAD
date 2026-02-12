# OsgVerse快速参考卡
# OsgVerse Quick Reference Card

## 🚀 快速命令 / Quick Commands

### 基础路径 / Base Paths
```bash
FREECAD_ROOT="/Users/zhouzongtao/repository/FreeCAD"
FREECAD_BIN="${FREECAD_ROOT}/build/debug/bin/FreeCAD"
```

---

## 📝 控制台模式脚本 / Console Mode Scripts

### 1. 诊断 / Diagnose
```bash
cd /Users/zhouzongtao/repository/FreeCAD
./build/debug/bin/FreeCAD --console diagnose_rendermanager.py
```

### 2. 切换到OsgVerse / Switch to OsgVerse
```bash
cd /Users/zhouzongtao/repository/FreeCAD
./build/debug/bin/FreeCAD --console switch_to_osgverse_ascii.py
```

### 3. 验证 / Verify
```bash
cd /Users/zhouzongtao/repository/FreeCAD
./build/debug/bin/FreeCAD --console verify_osgverse_rendering.py
```

### 4. 对比 / Compare
```bash
cd /Users/zhouzongtao/repository/FreeCAD
./build/debug/bin/FreeCAD --console compare_renderers.py
```

---

## 🖥️ GUI模式 / GUI Mode

### 启动GUI / Start GUI
```bash
cd /Users/zhouzongtao/repository/FreeCAD
./build/debug/bin/FreeCAD
```

### 在Python控制台中运行 / Run in Python Console
```python
# 验证OsgVerse
exec(open('/Users/zhouzongtao/repository/FreeCAD/verify_osgverse_gui.py', encoding='utf-8').read())
```

---

## 🔧 快速运行器 / Quick Runner

### 使用交互式菜单 / Use Interactive Menu
```bash
cd /Users/zhouzongtao/repository/FreeCAD
./run_verification.sh
```

---

## 🐍 Python命令 / Python Commands

### 检查Backend / Check Backend
```python
import FreeCADGui

# 当前backend (0=None, 1=Coin3D, 2=OsgVerse)
print(FreeCADGui.getCurrentRenderBackend())

# 渲染器信息
print(FreeCADGui.getRendererInfo())
```

### 切换Backend / Switch Backend
```python
import FreeCADGui

# 切换到Coin3D
FreeCADGui.switchRenderBackend(1)

# 切换到OsgVerse
FreeCADGui.switchRenderBackend(2)
```

### 检查可用性 / Check Availability
```python
import FreeCADGui

# Coin3D
print(FreeCADGui.isRenderBackendAvailable(1))

# OsgVerse
print(FreeCADGui.isRenderBackendAvailable(2))
```

### 渲染统计 / Render Statistics
```python
import FreeCADGui

# 获取统计
stats = FreeCADGui.getRenderStats()
print("FPS:", stats['fps'])
print("Triangles:", stats['triangleCount'])

# 重置统计
FreeCADGui.resetRenderStats()
```

---

## 📋 所有脚本路径 / All Script Paths

| 脚本名称 / Script Name | 完整路径 / Full Path |
|----------------------|---------------------|
| 诊断脚本 | `/Users/zhouzongtao/repository/FreeCAD/diagnose_rendermanager.py` |
| 切换脚本 | `/Users/zhouzongtao/repository/FreeCAD/switch_to_osgverse_ascii.py` |
| 验证脚本 | `/Users/zhouzongtao/repository/FreeCAD/verify_osgverse_rendering.py` |
| 对比脚本 | `/Users/zhouzongtao/repository/FreeCAD/compare_renderers.py` |
| GUI验证 | `/Users/zhouzongtao/repository/FreeCAD/verify_osgverse_gui.py` |
| 最终测试 | `/Users/zhouzongtao/repository/FreeCAD/test_final_verification.py` |
| 快速运行器 | `/Users/zhouzongtao/repository/FreeCAD/run_verification.sh` |

---

## 🔍 验证清单 / Verification Checklist

- [ ] `FreeCADGui.getRendererInfo()` 返回 "OsgVerse 3.6.5"
- [ ] `FreeCADGui.getCurrentRenderBackend()` 返回 2
- [ ] `FreeCADGui.isRenderBackendAvailable(2)` 返回 True
- [ ] 可以创建新文档
- [ ] 可以添加对象
- [ ] 视图类型包含 "OsgVerse"

---

## 📚 文档 / Documentation

| 文档名称 / Document Name | 路径 / Path |
|------------------------|------------|
| 使用指南 | `SCRIPTS_USAGE_GUIDE.md` |
| 切换指南 | `HOW_TO_SWITCH_TO_OSGVERSE.md` |
| 为什么看起来一样 | `WHY_OSGVERSE_LOOKS_SAME.md` |
| 状态报告 | `RENDERMANAGER_STATUS.md` |
| 重新编译指南 | `REBUILD_GUIDE.md` |

---

## 💡 常用别名 / Common Aliases

添加到 `~/.bashrc` 或 `~/.zshrc`:

```bash
# FreeCAD别名
alias fc='/Users/zhouzongtao/repository/FreeCAD/build/debug/bin/FreeCAD'
alias fcc='fc --console'
alias fcgui='fc'

# 验证脚本别名
alias fc-diagnose='fcc /Users/zhouzongtao/repository/FreeCAD/diagnose_rendermanager.py'
alias fc-switch='fcc /Users/zhouzongtao/repository/FreeCAD/switch_to_osgverse_ascii.py'
alias fc-verify='fcc /Users/zhouzongtao/repository/FreeCAD/verify_osgverse_rendering.py'
alias fc-test='/Users/zhouzongtao/repository/FreeCAD/run_verification.sh'

# 使用示例
# fc-diagnose
# fc-switch
# fc-verify
```

---

## 🆘 故障排除 / Troubleshooting

### 问题：找不到脚本
```bash
# 检查当前目录
pwd

# 切换到正确目录
cd /Users/zhouzongtao/repository/FreeCAD

# 列出脚本
ls -la *.py
```

### 问题：编码错误
```python
# 使用encoding参数
exec(open('/path/to/script.py', encoding='utf-8').read())
```

### 问题：AttributeError
```bash
# 重新编译
cd /Users/zhouzongtao/repository/FreeCAD/build/debug
cmake --build . -j$(sysctl -n hw.ncpu)
```

---

**快速帮助 / Quick Help:**
```bash
./run_verification.sh
```

**最后更新 / Last Updated:** 2026-02-12
