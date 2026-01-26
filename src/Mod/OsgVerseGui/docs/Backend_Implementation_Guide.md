# Backend Implementation Guide

This guide explains how to implement a new rendering backend for FreeCAD's render abstraction layer.

## Prerequisites

Before implementing a new backend, you should be familiar with:

- C++ programming
- Qt framework (QWidget, QOpenGLWidget)
- FreeCAD's ViewProvider system
- Your target rendering library (e.g., Vulkan, DirectX, custom engine)

## Step 1: Create the Backend Module

Create a new module directory in `src/Mod/`:

```
src/Mod/MyBackendGui/
├── CMakeLists.txt
├── PreCompiled.h
├── PreCompiled.cpp
├── AppMyBackendGui.cpp
├── MyBackendViewer.h
├── MyBackendViewer.cpp
├── MyBackendWidget.h
├── MyBackendWidget.cpp
├── MyBackendGuiExport.h
└── Init.py
```

## Step 2: Implement IViewer3D Interface

Create your viewer class that implements `Gui::View3D::IViewer3D`:

```cpp
// MyBackendViewer.h
#ifndef MYBACKENDGUI_VIEWER_H
#define MYBACKENDGUI_VIEWER_H

#include <Gui/View3D/IViewer3D.h>

namespace MyBackendGui {

class MyBackendViewer : public Gui::View3D::IViewer3D {
public:
    explicit MyBackendViewer(QWidget* parent = nullptr);
    virtual ~MyBackendViewer();

    // Basic Rendering
    void render() override;
    void resize(int width, int height) override;
    QWidget* getWidget() override;
    QOpenGLWidget* getGLWidget() override;

    // Scene Management
    void setSceneGraph(void* root) override;
    void* getSceneGraph() override;
    void updateScene() override;

    // Camera Control
    void setCamera(const Gui::View3D::CameraParams& params) override;
    Gui::View3D::CameraParams getCamera() const override;
    void viewAll() override;
    void resetCamera() override;
    void setCameraType(bool orthographic) override;
    bool isCameraOrthographic() const override;

    // Event Handling
    bool handleMouseEvent(QMouseEvent* event) override;
    bool handleKeyEvent(QKeyEvent* event) override;
    bool handleWheelEvent(QWheelEvent* event) override;

    // Picking and Selection
    Gui::View3D::PickResult pick(const QPoint& pos) override;
    void setSelectionMode(Gui::View3D::SelectionMode mode) override;
    Gui::View3D::SelectionMode getSelectionMode() const override;
    void startSelection(Gui::View3D::SelectionMode mode) override;
    void stopSelection() override;
    void abortSelection() override;
    bool isSelecting() const override;

    // ViewProvider Management
    void addViewProvider(Gui::ViewProvider* vp) override;
    void removeViewProvider(Gui::ViewProvider* vp) override;
    void updateViewProvider(Gui::ViewProvider* vp) override;
    bool hasViewProvider(Gui::ViewProvider* vp) const override;
    std::vector<Gui::ViewProvider*> getViewProviders() const override;

    // Rendering Settings
    void setRenderMode(Gui::View3D::RenderMode mode) override;
    Gui::View3D::RenderMode getRenderMode() const override;
    void setBackgroundColor(const Base::Color& color) override;
    Base::Color getBackgroundColor() const override;
    void setBacklightEnabled(bool enabled) override;
    bool isBacklightEnabled() const override;

    // Navigation
    void setNavigationStyle(const std::string& style) override;
    std::string getNavigationStyle() const override;
    void setViewing(bool enable) override;
    bool isViewing() const override;

    // Backend Information
    Gui::Render::BackendType getBackendType() const override;
    std::string getBackendName() const override;
    std::string getBackendVersion() const override;

    // Statistics
    Gui::Render::RenderStats getStats() const override;
    void resetStats() override;
    void setFPSEnabled(bool enabled) override;
    bool isFPSEnabled() const override;

    // Advanced Features
    QImage grabImage(int width = 0, int height = 0) override;
    bool saveScreenshot(const QString& filename, int width = 0, int height = 0) override;
    void setEditingViewProvider(Gui::ViewProvider* vp, int mode) override;
    Gui::ViewProvider* getEditingViewProvider() const override;
    bool isEditingViewProvider() const override;
    void resetEditingViewProvider() override;

private:
    // Your backend-specific members
    MyBackendWidget* _widget;
    // ... other members
};

} // namespace MyBackendGui

#endif
```

## Step 3: Implement the Widget

Create a Qt widget that handles rendering:

```cpp
// MyBackendWidget.h
#ifndef MYBACKENDGUI_WIDGET_H
#define MYBACKENDGUI_WIDGET_H

#include <QOpenGLWidget>

namespace MyBackendGui {

class MyBackendWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit MyBackendWidget(QWidget* parent = nullptr);
    ~MyBackendWidget() override;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    // Your rendering engine instance
};

} // namespace MyBackendGui

#endif
```

## Step 4: Register the Backend

In your `AppMyBackendGui.cpp`, register the backend with the factory:

```cpp
// AppMyBackendGui.cpp
#include "PreCompiled.h"

#include <Base/Console.h>
#include <Gui/Render/Core/ViewerFactory.h>
#include "MyBackendViewer.h"

namespace MyBackendGui {

// Factory function
Gui::View3D::IViewer3D* createMyBackendViewer(QWidget* parent) {
    return new MyBackendViewer(parent);
}

// Module initialization
PyObject* initModule() {
    // Register with ViewerFactory
    Gui::Render::ViewerFactory::registerBackend(
        Gui::Render::BackendType::MyBackend,  // Add to BackendType enum
        "MyBackend",
        &createMyBackendViewer
    );

    Base::Console().message("MyBackendGui: Backend registered\n");

    // Create Python module
    static struct PyModuleDef moduleDef = {
        PyModuleDef_HEAD_INIT,
        "MyBackendGui",
        "MyBackend rendering backend for FreeCAD",
        -1,
        nullptr
    };

    return PyModule_Create(&moduleDef);
}

} // namespace MyBackendGui

PyMOD_INIT_FUNC(MyBackendGui) {
    return MyBackendGui::initModule();
}
```

## Step 5: Implement Key Methods

### ViewProvider Management

The most critical methods to implement correctly:

```cpp
void MyBackendViewer::addViewProvider(Gui::ViewProvider* vp) {
    if (!vp) return;

    // 1. Create scene node for ViewProvider
    auto node = createNodeForViewProvider(vp);

    // 2. Store mapping
    _vpNodes[vp] = node;

    // 3. Add to scene graph
    _sceneRoot->addChild(node);

    // 4. Trigger render
    render();
}

void MyBackendViewer::updateViewProvider(Gui::ViewProvider* vp) {
    if (!vp) return;

    auto it = _vpNodes.find(vp);
    if (it == _vpNodes.end()) {
        addViewProvider(vp);
        return;
    }

    // 1. Remove old node
    _sceneRoot->removeChild(it->second);

    // 2. Create updated node
    auto newNode = createNodeForViewProvider(vp);

    // 3. Update mapping
    _vpNodes[vp] = newNode;

    // 4. Add to scene
    _sceneRoot->addChild(newNode);

    render();
}

void MyBackendViewer::removeViewProvider(Gui::ViewProvider* vp) {
    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        _sceneRoot->removeChild(it->second);
        _vpNodes.erase(it);
        render();
    }
}
```

### Picking Implementation

```cpp
Gui::View3D::PickResult MyBackendViewer::pick(const QPoint& pos) {
    Gui::View3D::PickResult result;
    result.valid = false;
    result.viewProvider = nullptr;

    // 1. Convert screen coordinates to ray
    Ray ray = screenToRay(pos);

    // 2. Perform intersection test
    HitResult hit = raycast(ray);

    if (hit.hasHit()) {
        result.valid = true;
        result.point = hit.worldPoint;
        result.normal = hit.worldNormal;
        result.distance = hit.distance;

        // 3. Find ViewProvider from hit node
        result.viewProvider = findViewProviderFromNode(hit.node);
    }

    return result;
}
```

### Geometry Conversion

Convert FreeCAD shapes to your rendering format:

```cpp
SceneNode* MyBackendViewer::createNodeForViewProvider(Gui::ViewProvider* vp) {
    auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);
    if (!vpDoc) return nullptr;

    App::DocumentObject* obj = vpDoc->getObject();
    if (!obj) return nullptr;

    // Get the shape
    if (obj->isDerivedFrom(Part::Feature::getClassTypeId())) {
        Part::TopoShape topoShape = Part::Feature::getTopoShape(obj);
        const TopoDS_Shape& shape = topoShape.getShape();

        if (!shape.IsNull()) {
            // Convert to your format
            return convertShapeToNode(shape);
        }
    }

    return nullptr;
}
```

## Step 6: CMakeLists.txt

```cmake
# CMakeLists.txt
add_library(MyBackendGui SHARED
    PreCompiled.h
    PreCompiled.cpp
    AppMyBackendGui.cpp
    MyBackendViewer.h
    MyBackendViewer.cpp
    MyBackendWidget.h
    MyBackendWidget.cpp
)

target_include_directories(MyBackendGui PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

target_link_libraries(MyBackendGui
    FreeCADGui
    Part
    Qt::OpenGLWidgets
    # Your rendering library
)

SET_BIN_DIR(MyBackendGui MyBackendGui /Mod/MyBackendGui)
SET_PYTHON_PREFIX_SUFFIX(MyBackendGui)

INSTALL(TARGETS MyBackendGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES Init.py DESTINATION Mod/MyBackendGui)
```

## Step 7: Python Module Init

```python
# Init.py
"""MyBackend Rendering Module"""
import MyBackendGui
print("MyBackendGui: Python Init.py executed")
```

## Testing Your Backend

Create a test file:

```python
# test_mybackend.py
import FreeCAD
import FreeCADGui
import MyBackendGui

def test_basic():
    doc = FreeCAD.newDocument("Test")
    box = doc.addObject("Part::Box", "Box")
    doc.recompute()

    # Verify ViewProvider exists
    assert box.ViewObject is not None

    FreeCAD.closeDocument("Test")
    print("Basic test passed!")

if __name__ == '__main__':
    test_basic()
```

## Best Practices

1. **Thread Safety**: Ensure all rendering operations happen on the main thread
2. **Memory Management**: Use smart pointers and proper cleanup
3. **Error Handling**: Gracefully handle null shapes and invalid inputs
4. **Performance**: Batch updates when possible
5. **Compatibility**: Support all FreeCAD ViewProvider types

## Common Issues

### Issue: Black Screen
- Check OpenGL context is valid
- Verify scene graph has content
- Check camera position/orientation

### Issue: Objects Not Appearing
- Verify `addViewProvider` is called
- Check shape is not null
- Verify geometry conversion succeeded

### Issue: Picking Not Working
- Check coordinate conversion (Qt Y-flip)
- Verify ray direction
- Check node-to-ViewProvider mapping
