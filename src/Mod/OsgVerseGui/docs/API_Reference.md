# IViewer3D API Reference

The `IViewer3D` interface defines the contract that all rendering backends must implement.

## Header Location

```cpp
#include <Gui/View3D/IViewer3D.h>
```

## Namespace

```cpp
namespace Gui::View3D
```

## Interface Definition

### Basic Rendering

#### `void render()`
Trigger a render/repaint of the 3D view.

```cpp
virtual void render() = 0;
```

#### `void resize(int width, int height)`
Resize the viewer to the specified dimensions.

```cpp
virtual void resize(int width, int height) = 0;
```

**Parameters:**
- `width` - New width in pixels
- `height` - New height in pixels

#### `QWidget* getWidget()`
Get the Qt widget for embedding in the UI.

```cpp
virtual QWidget* getWidget() = 0;
```

**Returns:** Pointer to the viewer's Qt widget

#### `QOpenGLWidget* getGLWidget()`
Get the OpenGL widget (if applicable).

```cpp
virtual QOpenGLWidget* getGLWidget() = 0;
```

**Returns:** Pointer to the OpenGL widget, or nullptr if not available

---

### Scene Management

#### `void setSceneGraph(void* root)`
Set the root scene graph node.

```cpp
virtual void setSceneGraph(void* root) = 0;
```

**Parameters:**
- `root` - Pointer to the root node (backend-specific type)

#### `void* getSceneGraph()`
Get the current scene graph root.

```cpp
virtual void* getSceneGraph() = 0;
```

**Returns:** Pointer to the root scene graph node

#### `void updateScene()`
Request a scene update/refresh.

```cpp
virtual void updateScene() = 0;
```

---

### Camera Control

#### `void setCamera(const CameraParams& params)`
Set camera parameters.

```cpp
virtual void setCamera(const CameraParams& params) = 0;
```

**CameraParams Structure:**
```cpp
struct CameraParams {
    Base::Vector3d position;      // Camera position
    Base::Vector3d target;        // Look-at target
    Base::Vector3d upVector;      // Up vector
    double fieldOfView;           // FOV in degrees (perspective)
    double nearPlane;             // Near clipping plane
    double farPlane;              // Far clipping plane
    double aspectRatio;           // Aspect ratio (0 = auto)
    double height;                // View height (orthographic)
    bool orthographic;            // Orthographic mode flag
};
```

#### `CameraParams getCamera() const`
Get current camera parameters.

```cpp
virtual CameraParams getCamera() const = 0;
```

**Returns:** Current camera configuration

#### `void viewAll()`
Fit all objects in view.

```cpp
virtual void viewAll() = 0;
```

#### `void resetCamera()`
Reset camera to default position.

```cpp
virtual void resetCamera() = 0;
```

#### `void setCameraType(bool orthographic)`
Switch between perspective and orthographic projection.

```cpp
virtual void setCameraType(bool orthographic) = 0;
```

**Parameters:**
- `orthographic` - true for orthographic, false for perspective

#### `bool isCameraOrthographic() const`
Check if camera is in orthographic mode.

```cpp
virtual bool isCameraOrthographic() const = 0;
```

**Returns:** true if orthographic, false if perspective

---

### Event Handling

#### `bool handleMouseEvent(QMouseEvent* event)`
Handle mouse events.

```cpp
virtual bool handleMouseEvent(QMouseEvent* event) = 0;
```

**Parameters:**
- `event` - Qt mouse event

**Returns:** true if event was handled

#### `bool handleKeyEvent(QKeyEvent* event)`
Handle keyboard events.

```cpp
virtual bool handleKeyEvent(QKeyEvent* event) = 0;
```

**Parameters:**
- `event` - Qt key event

**Returns:** true if event was handled

#### `bool handleWheelEvent(QWheelEvent* event)`
Handle mouse wheel events.

```cpp
virtual bool handleWheelEvent(QWheelEvent* event) = 0;
```

**Parameters:**
- `event` - Qt wheel event

**Returns:** true if event was handled

---

### Picking and Selection

#### `PickResult pick(const QPoint& pos)`
Perform picking at screen coordinates.

```cpp
virtual PickResult pick(const QPoint& pos) = 0;
```

**Parameters:**
- `pos` - Screen position (Qt coordinates)

**PickResult Structure:**
```cpp
struct PickResult {
    bool valid;                   // Whether pick hit something
    Base::Vector3d point;         // 3D intersection point
    Base::Vector3d normal;        // Surface normal at hit
    double distance;              // Distance from camera
    Gui::ViewProvider* viewProvider; // Hit ViewProvider
};
```

**Returns:** Pick result containing hit information

#### `void setSelectionMode(SelectionMode mode)`
Set the selection mode.

```cpp
virtual void setSelectionMode(SelectionMode mode) = 0;
```

**SelectionMode Enum:**
```cpp
enum class SelectionMode {
    None,           // No selection active
    Single,         // Single object selection
    Rectangle,      // Rectangle selection
    Rubberband,     // Rubberband selection
    Lasso,          // Lasso/freeform selection
    Clip            // Clipping selection
};
```

#### `SelectionMode getSelectionMode() const`
Get current selection mode.

```cpp
virtual SelectionMode getSelectionMode() const = 0;
```

#### `void startSelection(SelectionMode mode)`
Start a selection operation.

```cpp
virtual void startSelection(SelectionMode mode) = 0;
```

#### `void stopSelection()`
Stop/complete selection operation.

```cpp
virtual void stopSelection() = 0;
```

#### `void abortSelection()`
Abort selection without completing.

```cpp
virtual void abortSelection() = 0;
```

#### `bool isSelecting() const`
Check if selection is in progress.

```cpp
virtual bool isSelecting() const = 0;
```

---

### ViewProvider Management

#### `void addViewProvider(Gui::ViewProvider* vp)`
Add a ViewProvider to the scene.

```cpp
virtual void addViewProvider(Gui::ViewProvider* vp) = 0;
```

**Parameters:**
- `vp` - ViewProvider to add

#### `void removeViewProvider(Gui::ViewProvider* vp)`
Remove a ViewProvider from the scene.

```cpp
virtual void removeViewProvider(Gui::ViewProvider* vp) = 0;
```

**Parameters:**
- `vp` - ViewProvider to remove

#### `void updateViewProvider(Gui::ViewProvider* vp)`
Update a ViewProvider's representation.

```cpp
virtual void updateViewProvider(Gui::ViewProvider* vp) = 0;
```

**Parameters:**
- `vp` - ViewProvider to update

#### `bool hasViewProvider(Gui::ViewProvider* vp) const`
Check if ViewProvider is in scene.

```cpp
virtual bool hasViewProvider(Gui::ViewProvider* vp) const = 0;
```

**Returns:** true if ViewProvider is present

#### `std::vector<Gui::ViewProvider*> getViewProviders() const`
Get all ViewProviders in scene.

```cpp
virtual std::vector<Gui::ViewProvider*> getViewProviders() const = 0;
```

**Returns:** Vector of all ViewProviders

---

### Rendering Settings

#### `void setRenderMode(RenderMode mode)`
Set the rendering mode.

```cpp
virtual void setRenderMode(RenderMode mode) = 0;
```

**RenderMode Enum:**
```cpp
enum class RenderMode {
    AsIs,           // Use object's default
    Points,         // Point cloud
    Wireframe,      // Wireframe only
    Shaded,         // Shaded surfaces
    FlatLines,      // Shaded with edges
    HiddenLine,     // Hidden line removal
    NoShading       // Flat shading (no lighting)
};
```

#### `RenderMode getRenderMode() const`
Get current render mode.

```cpp
virtual RenderMode getRenderMode() const = 0;
```

#### `void setBackgroundColor(const Base::Color& color)`
Set background color.

```cpp
virtual void setBackgroundColor(const Base::Color& color) = 0;
```

**Parameters:**
- `color` - Background color (RGB, 0.0-1.0)

#### `Base::Color getBackgroundColor() const`
Get background color.

```cpp
virtual Base::Color getBackgroundColor() const = 0;
```

#### `void setBacklightEnabled(bool enabled)`
Enable/disable backlight.

```cpp
virtual void setBacklightEnabled(bool enabled) = 0;
```

#### `bool isBacklightEnabled() const`
Check if backlight is enabled.

```cpp
virtual bool isBacklightEnabled() const = 0;
```

---

### Navigation

#### `void setNavigationStyle(const std::string& style)`
Set navigation style.

```cpp
virtual void setNavigationStyle(const std::string& style) = 0;
```

**Supported Styles:**
- `"Gui::CADNavigationStyle"` - CAD-style navigation
- `"Gui::BlenderNavigationStyle"` - Blender-style
- `"Gui::TouchpadNavigationStyle"` - Touchpad-friendly
- `"Gui::GestureNavigationStyle"` - Gesture-based
- `"Gui::OpenInventorNavigationStyle"` - Open Inventor style
- `"Gui::MayaGestureNavigationStyle"` - Maya-style

#### `std::string getNavigationStyle() const`
Get current navigation style.

```cpp
virtual std::string getNavigationStyle() const = 0;
```

#### `void setViewing(bool enable)`
Enable/disable viewing mode.

```cpp
virtual void setViewing(bool enable) = 0;
```

#### `bool isViewing() const`
Check if in viewing mode.

```cpp
virtual bool isViewing() const = 0;
```

---

### Backend Information

#### `Gui::Render::BackendType getBackendType() const`
Get backend type identifier.

```cpp
virtual Gui::Render::BackendType getBackendType() const = 0;
```

**BackendType Enum:**
```cpp
enum class BackendType {
    Coin3D = 0,     // Traditional Coin3D backend
    OsgVerse = 1    // OsgVerse/OSG backend
};
```

#### `std::string getBackendName() const`
Get backend name string.

```cpp
virtual std::string getBackendName() const = 0;
```

**Returns:** e.g., "Coin3D" or "OsgVerse"

#### `std::string getBackendVersion() const`
Get backend version string.

```cpp
virtual std::string getBackendVersion() const = 0;
```

---

### Statistics and Debugging

#### `Gui::Render::RenderStats getStats() const`
Get rendering statistics.

```cpp
virtual Gui::Render::RenderStats getStats() const = 0;
```

**RenderStats Structure:**
```cpp
struct RenderStats {
    double fps;              // Frames per second
    double frameTime;        // Frame time in milliseconds
    int triangleCount;       // Number of triangles
    int vertexCount;         // Number of vertices
    int drawCalls;           // Number of draw calls
    int frameCount;          // Total frames rendered
};
```

#### `void resetStats()`
Reset statistics counters.

```cpp
virtual void resetStats() = 0;
```

#### `void setFPSEnabled(bool enabled)`
Enable/disable FPS display.

```cpp
virtual void setFPSEnabled(bool enabled) = 0;
```

#### `bool isFPSEnabled() const`
Check if FPS display is enabled.

```cpp
virtual bool isFPSEnabled() const = 0;
```

---

### Advanced Features

#### `QImage grabImage(int width = 0, int height = 0)`
Capture framebuffer as image.

```cpp
virtual QImage grabImage(int width = 0, int height = 0) = 0;
```

**Parameters:**
- `width` - Target width (0 = current)
- `height` - Target height (0 = current)

**Returns:** Captured image

#### `bool saveScreenshot(const QString& filename, int width = 0, int height = 0)`
Save screenshot to file.

```cpp
virtual bool saveScreenshot(const QString& filename, int width = 0, int height = 0) = 0;
```

**Parameters:**
- `filename` - Output file path
- `width`, `height` - Image dimensions

**Returns:** true on success

#### `void setEditingViewProvider(Gui::ViewProvider* vp, int mode)`
Set ViewProvider being edited.

```cpp
virtual void setEditingViewProvider(Gui::ViewProvider* vp, int mode) = 0;
```

#### `Gui::ViewProvider* getEditingViewProvider() const`
Get currently editing ViewProvider.

```cpp
virtual Gui::ViewProvider* getEditingViewProvider() const = 0;
```

#### `bool isEditingViewProvider() const`
Check if editing a ViewProvider.

```cpp
virtual bool isEditingViewProvider() const = 0;
```

#### `void resetEditingViewProvider()`
Clear editing state.

```cpp
virtual void resetEditingViewProvider() = 0;
```

---

## OsgVerseViewer Extended API

The `OsgVerseViewer` class provides additional shadow rendering features:

### Shadow Rendering

#### `void setShadowEnabled(bool enabled)`
Enable/disable shadow rendering.

#### `bool isShadowEnabled() const`
Check if shadows are enabled.

#### `void setShadowQuality(ShadowQuality quality)`
Set shadow map quality.

**ShadowQuality Enum:**
```cpp
enum class ShadowQuality {
    Low,     // 512x512
    Medium,  // 1024x1024
    High,    // 2048x2048
    Ultra    // 4096x4096
};
```

#### `void setSoftShadowEnabled(bool enabled)`
Enable/disable soft shadows.
