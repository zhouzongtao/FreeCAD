# Phase 2.5: View3DOsgVerse 实施计划

## 目标

创建专门的 OsgVerse 视图类，使 OsgVerse 后端能够真正用于 3D 渲染。

## 实施步骤

### Step 1: 创建 View3DOsgVerse 类
- 文件: `src/Gui/View3DOsgVerse.h`
- 文件: `src/Gui/View3DOsgVerse.cpp`
- 继承 View3DBase
- 使用 OsgVerseViewer

### Step 2: 修改 CMakeLists.txt
- 添加新文件到构建系统

### Step 3: 修改 Document::createView()
- 根据后端类型选择视图类
- Coin3D → View3DInventor
- OsgVerse → View3DOsgVerse

### Step 4: 实现 Python 绑定
- 创建 View3DOsgVersePy
- 导出到 Python

### Step 5: 测试
- 切换到 OsgVerse
- 创建文档和视图
- 验证渲染

## 设计

### View3DOsgVerse 类结构

```cpp
class View3DOsgVerse : public View3DBase
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    View3DOsgVerse(Gui::Document* pcDocument, QWidget* parent, 
                   Qt::WindowFlags wflags = Qt::WindowFlags());
    ~View3DOsgVerse() override;

    // View3DBase interface
    View3D::IViewer3D* getViewerInterface() override;
    BackendType getBackendType() const override;

    // MDIView interface
    void onUpdate() override;
    const char* getName() const override;
    bool onMsg(const char* pMsg, const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;
    void print() override;
    void printPdf() override;
    void printPreview() override;

private:
    std::unique_ptr<View3D::IViewer3D> _viewer;
};
```

## 实施
