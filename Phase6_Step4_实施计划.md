# Phase 6 Step 4 - View3D 抽象基类架构实施计划

## 目标

创建 View3D 抽象基类，将 View3DInventor 重构为继承 View3D，并创建 View3DOsgVerse 类。

## 新架构

```
Document
    └── View3D (抽象基类 - MDI 窗口容器)
        ├── View3DInventor (Coin3D 专用实现)
        │   └── CoinViewer → View3DInventorViewer
        └── View3DOsgVerse (OsgVerse 专用实现)
            └── OsgVerseViewerImpl
```

## 实施步骤

### Step 1: 创建 View3D 抽象基类 ✅

**文件**: `src/Gui/View3D.h`, `src/Gui/View3D.cpp`

**内容**:
- 从 View3DInventor 提取通用功能
- 定义抽象接口
- 管理 IViewer3D 指针

**工作量**: ~300 行代码

### Step 2: 重构 View3DInventor 继承 View3D ✅

**文件**: `src/Gui/View3DInventor.h`, `src/Gui/View3DInventor.cpp`

**修改**:
- 继承 View3D 而不是 MDIView
- 移除重复代码
- 实现 View3D 抽象接口

**工作量**: ~100 行修改

### Step 3: 创建 View3DOsgVerse 类 ✅

**文件**: `src/Gui/View3DOsgVerse.h`, `src/Gui/View3DOsgVerse.cpp`

**内容**:
- 继承 View3D
- 使用 OsgVerseViewerImpl
- 实现 OsgVerse 特定功能

**工作量**: ~200 行代码

### Step 4: 修改文档创建逻辑 ✅

**文件**: `src/Gui/Document.cpp`

**修改**:
- 根据后端选择创建不同的视图类
- 保持向后兼容

**工作量**: ~50 行修改

### Step 5: Python 绑定 ✅

**文件**: `src/Gui/View3DPy.cpp` (新建或修改)

**内容**:
- View3D 的 Python 绑定
- 保持 View3DInventorPy 向后兼容

**工作量**: ~100 行代码

### Step 6: 更新 CMakeLists.txt ✅

**文件**: `src/Gui/CMakeLists.txt`

**修改**:
- 添加新文件到构建系统

**工作量**: ~10 行修改

### Step 7: 测试和验证 ✅

**测试**:
- Coin3D 功能正常
- OsgVerse 功能正常
- 后端切换正常
- Python 接口正常

## 详细设计

### View3D 基类设计

```cpp
// src/Gui/View3D.h
#ifndef GUI_VIEW3D_H
#define GUI_VIEW3D_H

#include "MDIView.h"
#include "View3D/IViewer3D.h"
#include <memory>

namespace Gui {

class View3DPy;

/**
 * @brief 3D 视图抽象基类
 * 
 * 提供 MDI 窗口管理和通用 3D 视图功能，支持多种渲染后端。
 * 具体的渲染实现由子类提供（View3DInventor, View3DOsgVerse 等）。
 */
class GuiExport View3D : public MDIView
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    View3D(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags = Qt::WindowFlags());
    ~View3D() override;

    // MDIView 接口实现
    bool onMsg(const char* pMsg, const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;
    void deleteSelf() override;
    void onRename(Gui::Document* pDoc) override;
    void onUpdate() override;
    void viewAll() override;
    const char* getName() const override;
    
    // 打印功能
    void print() override;
    void printPdf() override;
    void printPreview() override;
    void print(QPrinter*) override;
    
    // Python 接口
    PyObject* getPyObject() override;
    
    // 3D 视图抽象接口（子类必须实现）
    virtual View3D::IViewer3D* getViewer() = 0;
    virtual const View3D::IViewer3D* getViewer() const = 0;
    
    // 相机控制（委托给 viewer）
    virtual void setCameraType(bool orthographic);
    virtual bool isCameraOrthographic() const;
    virtual void setBackgroundColor(const Base::Color& color);
    virtual Base::Color getBackgroundColor() const;
    
    // 渲染后端信息
    virtual Render::BackendType getBackendType() const;
    virtual std::string getBackendName() const;

protected:
    View3DPy* _viewerPy;
};

} // namespace Gui

#endif // GUI_VIEW3D_H
```

### View3DInventor 重构

```cpp
// src/Gui/View3DInventor.h
class GuiExport View3DInventor : public View3D
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    View3DInventor(Gui::Document* pcDocument, QWidget* parent, 
                   const QOpenGLWidget* sharewidget = nullptr,
                   Qt::WindowFlags wflags = Qt::WindowFlags());
    ~View3DInventor() override;
    
    // 实现 View3D 抽象接口
    View3D::IViewer3D* getViewer() override;
    const View3D::IViewer3D* getViewer() const override;
    
    // Coin3D 特定功能（保持向后兼容）
    View3DInventorViewer* getCoinViewer();
    const View3DInventorViewer* getCoinViewer() const;
    
    // 其他 View3DInventor 特定功能...

private:
    std::unique_ptr<View3D::Coin::CoinViewer> _coinViewer;
    View3DInventorViewer* _viewer;  // 指向 CoinViewer 内部的 viewer
};
```

### View3DOsgVerse 设计

```cpp
// src/Gui/View3DOsgVerse.h
#ifndef GUI_VIEW3DOSGVERSE_H
#define GUI_VIEW3DOSGVERSE_H

#include "View3D.h"
#include "View3D/Backends/OsgVerse/OsgVerseViewerImpl.h"

namespace Gui {

/**
 * @brief OsgVerse 3D 视图
 * 
 * 使用 OsgVerse 渲染后端的 3D 视图实现。
 */
class GuiExport View3DOsgVerse : public View3D
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    View3DOsgVerse(Gui::Document* pcDocument, QWidget* parent,
                   const QOpenGLWidget* sharewidget = nullptr,
                   Qt::WindowFlags wflags = Qt::WindowFlags());
    ~View3DOsgVerse() override;
    
    // 实现 View3D 抽象接口
    View3D::IViewer3D* getViewer() override;
    const View3D::IViewer3D* getViewer() const override;
    
    // OsgVerse 特定功能
    View3D::OsgVerse::OsgVerseViewerImpl* getOsgVerseViewer();
    const View3D::OsgVerse::OsgVerseViewerImpl* getOsgVerseViewer() const;

private:
    std::unique_ptr<View3D::OsgVerse::OsgVerseViewerImpl> _osgVerseViewer;
};

} // namespace Gui

#endif // GUI_VIEW3DOSGVERSE_H
```

### 文档创建逻辑

```cpp
// src/Gui/Document.cpp
MDIView* Document::createView(const Base::Type& typeId)
{
    if (typeId == View3DInventor::getClassTypeId()) {
        // 根据当前后端选择视图类型
        auto backend = Core::RenderManager::instance().getCurrentBackend();
        
        if (backend == Render::BackendType::OsgVerse) {
            #ifdef BUILD_WITH_OSGVERSE
            return new View3DOsgVerse(this, getMainWindow());
            #else
            Base::Console().warning("OsgVerse backend requested but not available, using Coin3D\n");
            return new View3DInventor(this, getMainWindow());
            #endif
        } else {
            // 默认使用 Coin3D
            return new View3DInventor(this, getMainWindow());
        }
    }
    
    // 其他视图类型...
}
```

## 实施顺序

### 阶段 1: 基础架构（今天）
1. ✅ 创建 View3D.h 和 View3D.cpp
2. ✅ 实现基础功能
3. ✅ 编译测试

### 阶段 2: 重构 View3DInventor（今天）
1. ✅ 修改继承关系
2. ✅ 移除重复代码
3. ✅ 测试 Coin3D 功能

### 阶段 3: 创建 View3DOsgVerse（今天）
1. ✅ 创建新类
2. ✅ 集成 OsgVerseViewerImpl
3. ✅ 测试基本功能

### 阶段 4: 集成和测试（今天）
1. ✅ 修改文档创建逻辑
2. ✅ 更新 CMakeLists.txt
3. ✅ 全面测试

### 阶段 5: Python 绑定（可选，明天）
1. ⏭️ 创建 View3DPy
2. ⏭️ 更新 Python 接口
3. ⏭️ 测试 Python 功能

## 风险和缓解

### 风险 1: 破坏现有功能
**缓解**: 
- 保持 View3DInventor 接口不变
- 充分测试
- 可以回滚

### 风险 2: 编译错误
**缓解**:
- 分步实施
- 每步编译测试
- 使用条件编译

### 风险 3: Python 绑定问题
**缓解**:
- Python 绑定作为最后一步
- 保持向后兼容
- 充分测试

## 成功标准

### 功能测试 ✅
- [ ] Coin3D 视图正常工作
- [ ] OsgVerse 视图正常工作
- [ ] 后端切换正常
- [ ] 所有现有功能保持不变

### 性能测试 ✅
- [ ] 启动时间无明显增加
- [ ] 渲染性能无下降
- [ ] 内存使用合理

### 代码质量 ✅
- [ ] 代码清晰易懂
- [ ] 注释完整
- [ ] 符合 FreeCAD 编码规范

## 预计工作量

- **代码编写**: 4-6 小时
- **测试**: 2-3 小时
- **文档**: 1 小时
- **总计**: 7-10 小时

## 开始实施

准备好了吗？让我们开始 Step 1: 创建 View3D 抽象基类！
