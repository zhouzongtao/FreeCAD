# 📐 FreeCAD Draft 模块架构解析 - 总目录

这是一份完整的FreeCAD Draft模块架构解析文档，分为三个部分。

## 📚 文档结构

### [第1部分](./Draft模块架构解析_第1部分.md)

**内容概览**：
- 一、总体架构概览
- 二、目录结构详解
  - App/ - C++扩展模块
  - draftobjects/ - 对象模型层（Model）
  - draftviewproviders/ - 视图提供者层（View）
  - draftguitools/ - GUI命令层（Controller）

**关键概念**：
- Draft模块的整体架构
- MVC模式的应用
- 对象模型系统
- 视图提供者系统
- GUI命令继承体系

---

### [第2部分](./Draft模块架构解析_第2部分.md)

**内容概览**：
- 三、GUI工具详细解析（续）
  - Line工具完整示例
  - 事件处理机制
- draftmake/ - 对象创建工厂
- draftgeoutils/ - 几何工具库
- draftutils/ - 工具函数库
  - todo.py - 延迟执行系统 ⭐
  - params.py - 参数管理
  - gui_tool_utils.py - GUI工具函数 ⭐

**关键概念**：
- 完整的命令执行流程
- 事件回调机制
- 工厂模式的应用
- 延迟执行系统（避免Coin3D崩溃）
- 鼠标输入控制机制

---

### [第3部分](./Draft模块架构解析_第3部分.md)

**内容概览**：
- 四、核心系统深度解析
  - WorkingPlane（工作平面系统）
  - Snapper（捕捉系统）
  - DraftToolBar（工具栏/任务面板）
- 五、数据流与交互流程
  - 创建对象的完整流程图
  - 修改对象的流程图
- 六、关键设计模式
  - 代理模式
  - 工厂模式
  - 命令模式
  - 观察者模式
  - 单例模式
- 七、最佳实践与注意事项
- 八、总结

**关键概念**：
- 工作平面的坐标系统
- 捕捉系统的实现
- 鼠标延迟机制
- 完整的数据流
- 设计模式的应用

---

## 🔍 快速索引

### 核心类和系统

| 类/系统 | 文件位置 | 文档部分 |
|--------|---------|---------|
| `DraftObject` | draftobjects/base.py | 第1部分 |
| `ViewProviderDraft` | draftviewproviders/view_base.py | 第1部分 |
| `DraftTool` | draftguitools/gui_base_original.py | 第1部分 |
| `Line` (工具示例) | draftguitools/gui_lines.py | 第2部分 |
| `WorkingPlane` | WorkingPlane.py | 第3部分 |
| `Snapper` | draftguitools/gui_snapper.py | 第3部分 |
| `DraftToolBar` | DraftGui.py | 第3部分 |
| `ToDo` | draftutils/todo.py | 第2部分 |

### 重要函数

| 函数 | 文件位置 | 文档部分 | 说明 |
|-----|---------|---------|------|
| `get_point()` | draftutils/gui_tool_utils.py | 第2部分 | 获取捕捉点 |
| `make_line()` | draftmake/make_line.py | 第2部分 | 创建线对象 |
| `delayCommit()` | draftutils/todo.py | 第2部分 | 延迟执行命令 |
| `snap()` | draftguitools/gui_snapper.py | 第3部分 | 捕捉系统 |

### 关键概念索引

| 概念 | 文档位置 | 关键词 |
|-----|---------|--------|
| 代理模式 | 第1部分, 第3部分 | `obj.Proxy`, 脚本化对象 |
| 事件处理 | 第1部分, 第2部分 | `addEventCallback`, `action()` |
| 延迟执行 | 第2部分, 第3部分 | `todo.delay()`, Coin3D限制 |
| 鼠标控制 | 第2部分, 第3部分 | `ui.mouse`, `MouseDelay` |
| 工作平面 | 第3部分 | 坐标转换, `getLocalCoords()` |
| 捕捉系统 | 第3部分 | 端点, 中点, 中心, 网格 |

---

## 🎯 常见问题快速查找

### 问题1：鼠标点击无响应
- **查看**：第2部分 - gui_tool_utils.py 中的 `get_point()` 函数
- **查看**：第3部分 - DraftToolBar 中的 `setMouseMode()` 方法
- **关键**：检查 `ui.mouse` 标志和 `MouseDelay` 参数

### 问题2：在Coin回调中崩溃
- **查看**：第2部分 - todo.py 延迟执行系统
- **查看**：第3部分 - 最佳实践与注意事项
- **关键**：使用 `todo.delay()` 或 `todo.delayCommit()`

### 问题3：坐标系混乱
- **查看**：第3部分 - WorkingPlane 系统
- **关键**：`getLocalCoords()` 和 `getGlobalCoords()` 的使用

### 问题4：如何添加新的Draft对象类型
- **查看**：第1部分 - draftobjects/ 目录结构
- **查看**：第2部分 - draftmake/ 工厂模式
- **步骤**：
  1. 创建对象类（继承 `DraftObject`）
  2. 创建视图提供者类（继承 `ViewProviderDraft`）
  3. 创建工厂函数（`make_*`）
  4. 创建GUI工具（继承 `Creator`）

### 问题5：如何添加新的捕捉类型
- **查看**：第3部分 - Snapper 系统
- **步骤**：在 `Snapper` 类中添加新的 `snapTo*()` 方法

---

## 📖 阅读建议

### 初学者路径

1. **第1部分** → 了解整体架构和基本概念
2. **第2部分** → 理解工厂模式和工具函数
3. **第3部分** → 深入核心系统

### 开发者路径

1. **第3部分** → 先看设计模式和流程图
2. **第1部分** → 理解对象模型
3. **第2部分** → 掌握工具实现细节

### 调试问题路径

1. 在"常见问题快速查找"中查找类似问题
2. 跳转到对应文档部分
3. 阅读相关代码示例

---

## 🛠️ 代码示例索引

### 完整的命令实现
- **文件**：第2部分 - Line工具完整示例
- **包含**：
  - `Activated()` - 命令激活
  - `action()` - 事件处理
  - `drawUpdate()` - 预览更新
  - `finish()` - 命令完成

### 对象创建
- **文件**：第1部分 - Wire对象示例
- **文件**：第2部分 - make_line() 工厂函数

### 坐标转换
- **文件**：第3部分 - WorkingPlane示例

### 捕捉实现
- **文件**：第3部分 - Snapper类完整代码

---

## 📝 文档信息

- **创建时间**：2024年10月
- **FreeCAD版本**：基于当前主分支
- **文档总字数**：约 25,000 字
- **代码示例**：50+ 个
- **流程图**：3 个

---

## 🔗 相关资源

- [FreeCAD官方文档](https://wiki.freecad.org/Draft_Workbench)
- [FreeCAD源代码](https://github.com/FreeCAD/FreeCAD)
- [Draft模块路径](./src/Mod/Draft/)

---

## ✨ 贡献

如果您发现文档中的错误或有改进建议，欢迎反馈！

---

**享受探索Draft模块的旅程！** 🚀


