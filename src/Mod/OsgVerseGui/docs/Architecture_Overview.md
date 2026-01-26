# Architecture Overview

This document describes the architecture of FreeCAD's Render Abstraction Layer.

## Design Goals

1. **Backend Independence** - Support multiple rendering backends through a unified interface
2. **Minimal Disruption** - Integrate with existing FreeCAD architecture without major changes
3. **Performance** - Maintain high rendering performance across all backends
4. **Extensibility** - Easy addition of new rendering backends
5. **Compatibility** - Full compatibility with existing FreeCAD features

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         FreeCAD Application                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐                   │
│  │ App::Document│   │ Gui::Document│   │Gui::Selection│                 │
│  │ (Model)     │   │ (View Model) │   │ (Selection)  │                 │
│  └──────┬──────┘   └──────┬──────┘   └──────┬───────┘                  │
│         │                 │                  │                          │
│         ▼                 ▼                  ▼                          │
│  ┌─────────────────────────────────────────────────────────┐           │
│  │              Gui::ViewProvider System                    │           │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │           │
│  │  │ViewProvider  │  │ViewProvider  │  │ViewProvider  │   │           │
│  │  │DocumentObject│  │Part          │  │Mesh          │   │           │
│  │  └──────────────┘  └──────────────┘  └──────────────┘   │           │
│  └─────────────────────────┬───────────────────────────────┘           │
│                            │                                            │
│                            ▼                                            │
│  ┌─────────────────────────────────────────────────────────┐           │
│  │                   View3DInventor                         │           │
│  │              (MDI Window Container)                      │           │
│  └─────────────────────────┬───────────────────────────────┘           │
│                            │                                            │
│                            ▼                                            │
│  ┌─────────────────────────────────────────────────────────┐           │
│  │                     IViewer3D                            │           │
│  │            (Abstract Viewer Interface)                   │           │
│  └─────────────────────────┬───────────────────────────────┘           │
│                            │                                            │
│            ┌───────────────┼───────────────┐                           │
│            ▼               ▼               ▼                            │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐                   │
│  │ CoinViewer  │   │OsgVerseViewer│  │ Future      │                   │
│  │ (Coin3D)    │   │ (OSG)       │   │ Backend     │                   │
│  └──────┬──────┘   └──────┬──────┘   └─────────────┘                   │
│         │                 │                                             │
│         ▼                 ▼                                             │
│  ┌─────────────┐   ┌─────────────┐                                     │
│  │Quarter      │   │OsgVerseWidget│                                    │
│  │Viewer       │   │(Qt/OpenGL)  │                                     │
│  └─────────────┘   └─────────────┘                                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. IViewer3D Interface

The central abstraction that all rendering backends implement:

```cpp
namespace Gui::View3D {

class IViewer3D {
public:
    virtual ~IViewer3D() = default;

    // Core rendering
    virtual void render() = 0;
    virtual QWidget* getWidget() = 0;

    // Scene management
    virtual void addViewProvider(Gui::ViewProvider* vp) = 0;
    virtual void removeViewProvider(Gui::ViewProvider* vp) = 0;
    virtual void updateViewProvider(Gui::ViewProvider* vp) = 0;

    // Camera control
    virtual void setCamera(const CameraParams& params) = 0;
    virtual CameraParams getCamera() const = 0;
    virtual void viewAll() = 0;

    // Picking and selection
    virtual PickResult pick(const QPoint& pos) = 0;

    // Backend info
    virtual Gui::Render::BackendType getBackendType() const = 0;
    virtual std::string getBackendName() const = 0;
};

}
```

### 2. ViewerFactory

Factory for creating viewer instances:

```cpp
namespace Gui::Render {

class ViewerFactory {
public:
    using CreateFunc = std::function<IViewer3D*(QWidget*)>;

    // Registration
    static void registerBackend(
        BackendType type,
        const std::string& name,
        CreateFunc factory
    );

    // Creation
    static IViewer3D* createViewer(
        BackendType type,
        QWidget* parent = nullptr
    );

    // Query
    static std::vector<BackendType> getAvailableBackends();
    static BackendType getDefaultBackend();
    static void setDefaultBackend(BackendType type);

private:
    static std::map<BackendType, CreateFunc> _factories;
    static std::map<BackendType, std::string> _names;
    static BackendType _defaultBackend;
};

}
```

### 3. View3DInventor

The MDI window container that holds the viewer:

```cpp
class View3DInventor : public MDIView {
public:
    // Get the abstract viewer interface
    IViewer3D* getViewer() const { return _viewer.get(); }

    // Convenience methods that delegate to viewer
    void viewAll() { _viewer->viewAll(); }
    void setCamera(const CameraParams& p) { _viewer->setCamera(p); }

private:
    std::unique_ptr<IViewer3D> _viewer;
};
```

### 4. Backend Types

```cpp
namespace Gui::Render {

enum class BackendType {
    Coin3D = 0,    // Traditional Coin3D/Open Inventor
    OsgVerse = 1,  // OpenSceneGraph-based
    // Future: Vulkan = 2, DirectX = 3, etc.
};

}
```

## Data Flow

### Object Creation Flow

```
User creates Part::Box
        │
        ▼
App::Document::addObject("Part::Box", "Box")
        │
        ▼
Gui::Document creates ViewProvider for Box
        │
        ▼
ViewProvider::attach() called
        │
        ▼
View3DInventor notified of new ViewProvider
        │
        ▼
IViewer3D::addViewProvider(vp) called
        │
        ├─── CoinViewer: Creates SoNode, adds to scene graph
        │
        └─── OsgVerseViewer: Converts shape to osg::Geode,
                             adds to scene graph
        │
        ▼
render() triggered
```

### Property Change Flow

```
User changes Box.Height
        │
        ▼
Part::Box::onChanged() - shape recomputed
        │
        ▼
Gui::Document::signalChangedObject emitted
        │
        ▼
ViewProvider::updateData() called
        │
        ▼
View3DInventor notified
        │
        ▼
IViewer3D::updateViewProvider(vp) called
        │
        ├─── CoinViewer: Updates SoNode geometry
        │
        └─── OsgVerseViewer: Regenerates osg::Geode
        │
        ▼
render() triggered
```

### Selection Flow

```
User clicks on object
        │
        ▼
Qt mouse event received by widget
        │
        ▼
IViewer3D::pick(screenPos) called
        │
        ├─── CoinViewer: SoRayPickAction
        │
        └─── OsgVerseViewer: osgUtil::Intersector
        │
        ▼
PickResult with ViewProvider returned
        │
        ▼
Gui::Selection::addSelection() called
        │
        ▼
Selection::signalSelectionChanged emitted
        │
        ▼
ViewProviders update highlight state
        │
        ▼
render() triggered
```

## Module Structure

### OsgVerseGui Module

```
src/Mod/OsgVerseGui/
├── CMakeLists.txt              # Build configuration
├── PreCompiled.h               # Precompiled header
├── PreCompiled.cpp
├── AppOsgVerseGui.cpp          # Module initialization
├── OsgVerseGuiExport.h         # DLL export macros
│
├── OsgVerseViewer.h            # IViewer3D implementation
├── OsgVerseViewer.cpp
├── OsgVerseWidget.h            # Qt OpenGL widget
├── OsgVerseWidget.cpp
│
├── GeometryConverter.h         # OCCT to OSG conversion
├── GeometryConverter.cpp
│
├── Init.py                     # Python initialization
│
├── TestRenderAbstractionLayer.py  # Interface tests
├── TestOsgVerseBackend.py         # Backend tests
├── TestHeadless.py                # Headless tests
├── run_tests.py                   # Test runner
│
└── docs/
    ├── README.md
    ├── API_Reference.md
    ├── Backend_Implementation_Guide.md
    ├── ViewProvider_Integration.md
    ├── Geometry_Converter_Guide.md
    ├── Performance_Optimization.md
    └── Architecture_Overview.md
```

### Core Gui Components (in src/Gui/)

```
src/Gui/
├── View3D/
│   ├── IViewer3D.h             # Abstract interface
│   └── CameraParams.h          # Camera parameter struct
│
├── Render/
│   └── Core/
│       ├── ViewerFactory.h     # Backend factory
│       ├── ViewerFactory.cpp
│       ├── BackendType.h       # Backend enumeration
│       └── RenderStats.h       # Statistics struct
│
├── View3DInventor.h            # MDI view container
├── View3DInventor.cpp
├── View3DInventorViewer.h      # Coin3D viewer (legacy)
└── View3DInventorViewer.cpp
```

## Class Relationships

### Inheritance Hierarchy

```
                    QObject
                       │
                       ▼
               Gui::ViewProvider
                       │
                       ▼
         Gui::ViewProviderDocumentObject
               │              │
               ▼              ▼
    PartGui::ViewProviderPart  Mesh::ViewProviderMesh
               │
               ▼
    PartGui::ViewProviderPartExt
```

### Composition

```
View3DInventor ────────────────────────────────────┐
    │                                               │
    │ owns                                          │
    ▼                                               │
IViewer3D (interface)                               │
    △                                               │
    │ implements                                    │
    ├──────────────────┐                           │
    │                  │                           │
CoinViewer        OsgVerseViewer                   │
    │                  │                           │
    │ owns             │ owns                      │
    ▼                  ▼                           │
Quarter::         OsgVerseWidget                   │
QuarterWidget          │                           │
                       │ owns                      │
                       ▼                           │
               osgViewer::Viewer                   │
                       │                           │
                       │ references                │
                       ▼                           │
               osg::Group (scene root)             │
                       │                           │
                       │ contains                  │
                       ▼                           │
               osg::Geode (per ViewProvider)       │
                                                   │
◄──────────────────────────────────────────────────┘
```

## Threading Model

```
┌─────────────────────────────────────────────────────────────────┐
│                        Main Thread                               │
│                                                                  │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐                │
│  │ Qt Event   │  │ FreeCAD    │  │ Rendering  │                │
│  │ Loop       │  │ Commands   │  │ Updates    │                │
│  └────────────┘  └────────────┘  └────────────┘                │
│         │              │               │                        │
│         ▼              ▼               ▼                        │
│  ┌──────────────────────────────────────────────┐              │
│  │            IViewer3D Methods                  │              │
│  │  (addViewProvider, updateViewProvider, etc.)  │              │
│  └──────────────────────────────────────────────┘              │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ OpenGL context
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        GPU Thread                                │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐                │
│  │ Vertex     │  │ Fragment   │  │ Display    │                │
│  │ Processing │  │ Processing │  │ Output     │                │
│  └────────────┘  └────────────┘  └────────────┘                │
└─────────────────────────────────────────────────────────────────┘
```

**Key Threading Rules:**
1. All IViewer3D method calls happen on the main thread
2. OpenGL rendering is handled by Qt's OpenGL context
3. Background tessellation should use worker threads
4. Signal/slot connections for cross-thread communication

## Extension Points

### Adding a New Backend

1. Implement `IViewer3D` interface
2. Create Qt widget for rendering
3. Register with `ViewerFactory`
4. Add to `BackendType` enum

### Adding New Geometry Types

1. Extend `GeometryConverter` for new types
2. Handle in `createNodeForViewProvider()`
3. Add type detection in ViewProvider handling

### Adding New Rendering Features

1. Add method to `IViewer3D` interface
2. Implement in all backends
3. Expose through `View3DInventor` if needed

## Configuration

### Build-Time Configuration

```cmake
# CMakeLists.txt options
option(BUILD_OSGVERSE_GUI "Build OsgVerse rendering backend" ON)
option(OSGVERSE_ENABLE_SHADOWS "Enable shadow rendering" ON)
```

### Runtime Configuration

```python
# FreeCAD preferences
FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetInt(
    "RenderBackend", 1  # 0=Coin3D, 1=OsgVerse
)
```

## Error Handling Strategy

```cpp
// Backend errors are logged but don't crash
void OsgVerseViewer::addViewProvider(Gui::ViewProvider* vp) {
    if (!vp) {
        Base::Console().Warning("OsgVerseViewer: null ViewProvider\n");
        return;
    }

    try {
        auto node = createNodeForViewProvider(vp);
        if (!node) {
            Base::Console().Warning(
                "OsgVerseViewer: Failed to create node for %s\n",
                vp->getTypeId().getName()
            );
            return;
        }
        _sceneRoot->addChild(node);
        _vpNodes[vp] = node;
    }
    catch (const std::exception& e) {
        Base::Console().Error(
            "OsgVerseViewer: Exception in addViewProvider: %s\n",
            e.what()
        );
    }
}
```

## Future Directions

1. **Vulkan Backend** - Modern graphics API support
2. **Ray Tracing** - Hardware-accelerated ray tracing
3. **WebGL Export** - Browser-based viewing
4. **VR Support** - Virtual reality rendering
5. **Multi-GPU** - Distributed rendering

## References

- [OpenSceneGraph](http://www.openscenegraph.org/)
- [osgVerse](https://github.com/xarray/osgVerse)
- [Qt OpenGL](https://doc.qt.io/qt-6/qtopengl-index.html)
- [FreeCAD Architecture](https://wiki.freecad.org/Developer_hub)
