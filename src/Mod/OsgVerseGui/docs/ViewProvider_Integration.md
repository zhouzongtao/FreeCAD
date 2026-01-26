# ViewProvider Integration Guide

This guide explains how the render abstraction layer integrates with FreeCAD's ViewProvider system.

## Overview

ViewProviders are FreeCAD's mechanism for visual representation of document objects. Each `App::DocumentObject` has a corresponding `Gui::ViewProvider` that handles:

- Visual appearance (shape, color, transparency)
- Selection behavior
- Edit mode
- Context menus
- Properties panel

## ViewProvider Hierarchy

```
Gui::ViewProvider
└── Gui::ViewProviderDocumentObject
    └── PartGui::ViewProviderPart
        └── PartGui::ViewProviderPartExt
            ├── Part::Box (ViewProvider)
            ├── Part::Sphere (ViewProvider)
            └── ... other Part features
```

## Key ViewProvider Properties

### Visual Properties

| Property | Type | Description |
|----------|------|-------------|
| `ShapeColor` | PropertyColor | Object diffuse color |
| `Transparency` | PropertyPercent | Transparency (0-100) |
| `LineColor` | PropertyColor | Edge color |
| `PointColor` | PropertyColor | Vertex color |
| `LineWidth` | PropertyFloat | Edge line width |
| `PointSize` | PropertyFloat | Vertex point size |
| `Visibility` | PropertyBool | Show/hide object |
| `DisplayMode` | PropertyEnumeration | Display mode (Shaded, etc.) |

### Accessing Properties in Backend

```cpp
void MyViewer::applyViewProviderProperties(
    Gui::ViewProvider* vp,
    SceneNode* node
) {
    // Get ShapeColor
    App::Property* colorProp = vp->getPropertyByName("ShapeColor");
    if (colorProp && colorProp->isDerivedFrom(App::PropertyColor::getClassTypeId())) {
        auto* propColor = static_cast<App::PropertyColor*>(colorProp);
        unsigned long packed = propColor->getValue().getPackedValue();
        float r = ((packed >> 24) & 0xFF) / 255.0f;
        float g = ((packed >> 16) & 0xFF) / 255.0f;
        float b = ((packed >> 8) & 0xFF) / 255.0f;
        node->setColor(r, g, b);
    }

    // Get Transparency
    App::Property* transProp = vp->getPropertyByName("Transparency");
    if (transProp && transProp->isDerivedFrom(App::PropertyPercent::getClassTypeId())) {
        auto* propTrans = static_cast<App::PropertyPercent*>(transProp);
        float alpha = 1.0f - (propTrans->getValue() / 100.0f);
        node->setAlpha(alpha);
    }

    // Get Visibility
    if (vp->isVisible()) {
        node->show();
    } else {
        node->hide();
    }
}
```

## ViewProvider Lifecycle

### Creation

When a document object is created:

```
1. App::DocumentObject created
2. Gui::ViewProvider created automatically
3. ViewProvider::attach() called
4. ViewProvider added to Gui::Document
5. IViewer3D::addViewProvider() called
```

### Update

When an object changes:

```
1. DocumentObject property changes
2. Document::recompute() called
3. ViewProvider::updateData() called
4. IViewer3D::updateViewProvider() called
```

### Deletion

When an object is deleted:

```
1. Document::removeObject() called
2. IViewer3D::removeViewProvider() called
3. ViewProvider removed from Gui::Document
4. ViewProvider destructor called
```

## Implementing ViewProvider Support

### Step 1: Store ViewProvider Mapping

```cpp
class MyViewer : public IViewer3D {
private:
    // Map ViewProvider to scene node
    std::map<Gui::ViewProvider*, SceneNodePtr> _vpNodes;
};
```

### Step 2: Implement addViewProvider

```cpp
void MyViewer::addViewProvider(Gui::ViewProvider* vp) {
    if (!vp || _vpNodes.count(vp) > 0) {
        return; // Already added or null
    }

    // Create scene representation
    SceneNodePtr node = createNodeForViewProvider(vp);

    if (node) {
        // Store mapping for picking and updates
        _vpNodes[vp] = node;

        // Add to scene
        _sceneRoot->addChild(node);

        // Apply initial properties
        applyViewProviderProperties(vp, node.get());
    }

    render();
}
```

### Step 3: Implement updateViewProvider

```cpp
void MyViewer::updateViewProvider(Gui::ViewProvider* vp) {
    auto it = _vpNodes.find(vp);
    if (it == _vpNodes.end()) {
        // Not in scene yet, add it
        addViewProvider(vp);
        return;
    }

    // Remove old representation
    _sceneRoot->removeChild(it->second);

    // Create new representation
    SceneNodePtr newNode = createNodeForViewProvider(vp);

    if (newNode) {
        _vpNodes[vp] = newNode;
        _sceneRoot->addChild(newNode);
        applyViewProviderProperties(vp, newNode.get());
    } else {
        _vpNodes.erase(it);
    }

    render();
}
```

### Step 4: Implement removeViewProvider

```cpp
void MyViewer::removeViewProvider(Gui::ViewProvider* vp) {
    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        _sceneRoot->removeChild(it->second);
        _vpNodes.erase(it);
        render();
    }
}
```

### Step 5: Support Picking

Store ViewProvider reference in scene nodes for picking:

```cpp
// Custom user data for picking
class ViewProviderUserData {
public:
    ViewProviderUserData(Gui::ViewProvider* vp) : viewProvider(vp) {}
    Gui::ViewProvider* viewProvider;
};

SceneNodePtr MyViewer::createNodeForViewProvider(Gui::ViewProvider* vp) {
    auto node = createGeometryNode(vp);
    if (node) {
        // Attach ViewProvider reference for picking
        node->setUserData(new ViewProviderUserData(vp));
    }
    return node;
}

Gui::ViewProvider* MyViewer::findViewProviderFromNode(SceneNode* node) {
    // Walk up node hierarchy looking for ViewProvider data
    while (node) {
        auto* userData = node->getUserData<ViewProviderUserData>();
        if (userData) {
            return userData->viewProvider;
        }
        node = node->getParent();
    }
    return nullptr;
}
```

## Handling ViewProvider Changes

### Property Change Notification

ViewProviders emit signals when properties change:

```cpp
// In your viewer initialization
void MyViewer::connectViewProviderSignals(Gui::ViewProvider* vp) {
    // Connect to property change signal
    vp->signalChangeProperty.connect(
        [this, vp](const App::Property& prop) {
            onViewProviderPropertyChanged(vp, prop);
        }
    );
}

void MyViewer::onViewProviderPropertyChanged(
    Gui::ViewProvider* vp,
    const App::Property& prop
) {
    auto it = _vpNodes.find(vp);
    if (it == _vpNodes.end()) return;

    const char* name = prop.getName();

    if (strcmp(name, "ShapeColor") == 0) {
        updateNodeColor(it->second.get(), vp);
    }
    else if (strcmp(name, "Transparency") == 0) {
        updateNodeTransparency(it->second.get(), vp);
    }
    else if (strcmp(name, "Visibility") == 0) {
        updateNodeVisibility(it->second.get(), vp);
    }
    // ... handle other properties

    render();
}
```

## ViewProvider Types

### Part::Feature ViewProviders

Most common type - have TopoDS_Shape:

```cpp
if (obj->isDerivedFrom(Part::Feature::getClassTypeId())) {
    Part::TopoShape topoShape = Part::Feature::getTopoShape(obj);
    const TopoDS_Shape& shape = topoShape.getShape();
    // Convert shape to render geometry
}
```

### Mesh::Feature ViewProviders

Mesh objects:

```cpp
if (obj->isDerivedFrom(Mesh::Feature::getClassTypeId())) {
    const Mesh::MeshObject& mesh =
        static_cast<Mesh::Feature*>(obj)->Mesh.getValue();
    // Convert mesh to render geometry
}
```

### Points::Feature ViewProviders

Point clouds:

```cpp
if (obj->isDerivedFrom(Points::Feature::getClassTypeId())) {
    const Points::PointKernel& points =
        static_cast<Points::Feature*>(obj)->Points.getValue();
    // Convert points to render geometry
}
```

## Selection Integration

### Highlight on Selection

```cpp
void MyViewer::onSelectionChanged(
    const Gui::SelectionChanges& msg
) {
    switch (msg.Type) {
        case Gui::SelectionChanges::AddSelection:
            highlightViewProvider(msg.pDocName, msg.pObjectName, true);
            break;
        case Gui::SelectionChanges::RmvSelection:
            highlightViewProvider(msg.pDocName, msg.pObjectName, false);
            break;
        case Gui::SelectionChanges::ClrSelection:
            clearAllHighlights();
            break;
    }
    render();
}

void MyViewer::highlightViewProvider(
    const char* docName,
    const char* objName,
    bool highlight
) {
    App::Document* doc = App::GetApplication().getDocument(docName);
    if (!doc) return;

    App::DocumentObject* obj = doc->getObject(objName);
    if (!obj) return;

    Gui::ViewProvider* vp = Gui::Application::Instance
        ->getDocument(doc)->getViewProvider(obj);
    if (!vp) return;

    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        if (highlight) {
            it->second->setHighlightColor(1.0f, 0.8f, 0.0f); // Orange
        } else {
            it->second->clearHighlight();
        }
    }
}
```

## Best Practices

1. **Lazy Updates**: Don't update geometry for every property change
2. **Batch Operations**: Combine multiple ViewProvider updates
3. **Memory Management**: Clean up when ViewProvider is removed
4. **Thread Safety**: ViewProvider changes come from main thread
5. **Null Checks**: Always validate ViewProvider and object pointers
