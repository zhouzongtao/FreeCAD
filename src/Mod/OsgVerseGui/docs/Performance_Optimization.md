# Performance Optimization Guide

This guide provides best practices for optimizing rendering performance in FreeCAD's render abstraction layer.

## Overview

Rendering performance depends on several factors:
- Geometry complexity (vertex/triangle count)
- Material and shader complexity
- Scene graph structure
- GPU capabilities
- Driver efficiency

## Geometry Optimization

### 1. Use Appropriate Tessellation Quality

Choose the right quality level for your use case:

```cpp
// Fast preview - minimal detail
GeometryConverter::ConversionOptions opts =
    GeometryConverter::ConversionOptions::Draft();

// Standard interactive viewing
GeometryConverter::ConversionOptions opts =
    GeometryConverter::ConversionOptions::Normal();

// High-quality final rendering
GeometryConverter::ConversionOptions opts =
    GeometryConverter::ConversionOptions::Fine();
```

**Quality Level Comparison:**

| Level  | Deflection | Angle | Use Case |
|--------|------------|-------|----------|
| Draft  | 0.5        | 1.0   | Large models, preview |
| Normal | 0.1        | 0.5   | Interactive viewing |
| Fine   | 0.01       | 0.1   | Final renders, exports |

### 2. Level of Detail (LOD)

Implement LOD for complex scenes:

```cpp
osg::ref_ptr<osg::LOD> createLODNode(const TopoDS_Shape& shape) {
    osg::ref_ptr<osg::LOD> lod = new osg::LOD();

    // High detail - close range
    auto fineGeom = GeometryConverter::convertShape(
        shape, GeometryConverter::ConversionOptions::Fine());
    lod->addChild(fineGeom, 0.0f, 50.0f);

    // Medium detail - mid range
    auto normalGeom = GeometryConverter::convertShape(
        shape, GeometryConverter::ConversionOptions::Normal());
    lod->addChild(normalGeom, 50.0f, 200.0f);

    // Low detail - far range
    auto draftGeom = GeometryConverter::convertShape(
        shape, GeometryConverter::ConversionOptions::Draft());
    lod->addChild(draftGeom, 200.0f, FLT_MAX);

    return lod;
}
```

### 3. Geometry Caching

Cache converted geometry to avoid repeated tessellation:

```cpp
class GeometryCache {
public:
    struct CacheKey {
        const TopoDS_Shape* shape;
        double deflection;
        size_t shapeHash;

        bool operator<(const CacheKey& other) const {
            return std::tie(shapeHash, deflection) <
                   std::tie(other.shapeHash, other.deflection);
        }
    };

    osg::ref_ptr<osg::Geode> getOrCreate(
        const TopoDS_Shape& shape,
        const GeometryConverter::ConversionOptions& opts
    ) {
        CacheKey key{&shape, opts.deflection, computeShapeHash(shape)};

        auto it = _cache.find(key);
        if (it != _cache.end()) {
            return it->second;  // Return cached
        }

        // Convert and cache
        auto geode = GeometryConverter::convertShape(shape, opts);
        _cache[key] = geode;
        return geode;
    }

    void clear() { _cache.clear(); }

private:
    std::map<CacheKey, osg::ref_ptr<osg::Geode>> _cache;
};
```

### 4. Vertex Buffer Objects (VBOs)

Enable VBOs for GPU-side geometry:

```cpp
void enableVBOs(osg::Geometry* geometry) {
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);

    // Optionally enable vertex array objects
    geometry->setUseVertexArrayObject(true);
}
```

## Scene Graph Optimization

### 1. Spatial Organization

Use spatial partitioning for large scenes:

```cpp
// Example: Octree-based organization
class SpatialPartitioner {
public:
    void organize(osg::Group* root) {
        // Collect all geometries with bounding boxes
        std::vector<std::pair<osg::Node*, osg::BoundingBox>> nodes;
        collectNodes(root, nodes);

        // Build octree structure
        buildOctree(nodes);
    }
};
```

### 2. State Sorting

Group objects by material to minimize state changes:

```cpp
void sortByMaterial(osg::Group* root) {
    // Collect nodes by material
    std::map<osg::StateSet*, std::vector<osg::Node*>> byMaterial;

    for (unsigned int i = 0; i < root->getNumChildren(); i++) {
        osg::Node* child = root->getChild(i);
        osg::StateSet* ss = child->getStateSet();
        byMaterial[ss].push_back(child);
    }

    // Reorganize scene graph
    root->removeChildren(0, root->getNumChildren());
    for (auto& pair : byMaterial) {
        osg::ref_ptr<osg::Group> materialGroup = new osg::Group();
        materialGroup->setStateSet(pair.first);
        for (osg::Node* node : pair.second) {
            materialGroup->addChild(node);
        }
        root->addChild(materialGroup);
    }
}
```

### 3. Culling Optimization

Enable and tune culling:

```cpp
void optimizeCulling(osg::Node* node) {
    // Enable small feature culling
    osg::CullStack::CullingMode mode =
        osg::CullStack::VIEW_FRUSTUM_CULLING |
        osg::CullStack::SMALL_FEATURE_CULLING;

    node->setCullingActive(true);

    // Set small feature threshold
    // Objects smaller than this pixel size are culled
    osg::CullSettings* settings = viewer->getCamera();
    settings->setSmallFeatureCullingPixelSize(2.0f);
}
```

## Rendering Optimization

### 1. Batching Draw Calls

Combine multiple objects into single draw calls:

```cpp
osg::ref_ptr<osg::Geometry> batchGeometries(
    const std::vector<osg::Geometry*>& geometries
) {
    osg::ref_ptr<osg::Geometry> combined = new osg::Geometry();

    // Calculate total sizes
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (auto* geom : geometries) {
        auto* verts = dynamic_cast<osg::Vec3Array*>(
            geom->getVertexArray());
        if (verts) totalVertices += verts->size();

        for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); i++) {
            totalIndices += geom->getPrimitiveSet(i)->getNumIndices();
        }
    }

    // Pre-allocate arrays
    osg::ref_ptr<osg::Vec3Array> vertices =
        new osg::Vec3Array();
    vertices->reserve(totalVertices);

    osg::ref_ptr<osg::Vec3Array> normals =
        new osg::Vec3Array();
    normals->reserve(totalVertices);

    osg::ref_ptr<osg::DrawElementsUInt> indices =
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
    indices->reserve(totalIndices);

    // Merge geometries
    unsigned int vertexOffset = 0;
    for (auto* geom : geometries) {
        // Copy vertices and normals
        auto* srcVerts = dynamic_cast<osg::Vec3Array*>(
            geom->getVertexArray());
        auto* srcNorms = dynamic_cast<osg::Vec3Array*>(
            geom->getNormalArray());

        if (srcVerts) {
            vertices->insert(vertices->end(),
                srcVerts->begin(), srcVerts->end());
        }
        if (srcNorms) {
            normals->insert(normals->end(),
                srcNorms->begin(), srcNorms->end());
        }

        // Copy indices with offset
        for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); i++) {
            auto* srcIndices = dynamic_cast<osg::DrawElementsUInt*>(
                geom->getPrimitiveSet(i));
            if (srcIndices) {
                for (unsigned int j = 0; j < srcIndices->size(); j++) {
                    indices->push_back((*srcIndices)[j] + vertexOffset);
                }
            }
        }

        if (srcVerts) vertexOffset += srcVerts->size();
    }

    combined->setVertexArray(vertices);
    combined->setNormalArray(normals);
    combined->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    combined->addPrimitiveSet(indices);
    combined->setUseVertexBufferObjects(true);

    return combined;
}
```

### 2. Occlusion Culling

Enable occlusion queries for complex scenes:

```cpp
void enableOcclusionCulling(osgViewer::Viewer* viewer) {
    osg::ref_ptr<osg::OcclusionQueryNode> oqn =
        new osg::OcclusionQueryNode();

    // Configure query parameters
    oqn->setVisibilityThreshold(100);  // Minimum visible pixels
    oqn->setQueryFrameCount(5);         // Frames between queries

    // Wrap expensive geometry
    oqn->addChild(expensiveGeometry);
}
```

### 3. Shadow Optimization

Optimize shadow rendering:

```cpp
void optimizeShadows(OsgVerseViewer* viewer) {
    // Use appropriate shadow quality
    // Lower quality for dynamic scenes
    viewer->setShadowQuality(OsgVerseViewer::ShadowQuality::Medium);

    // Enable soft shadows only when needed
    viewer->setSoftShadowEnabled(false);

    // Limit shadow distance
    // Objects beyond this distance don't cast shadows
    viewer->setShadowDistance(1000.0f);
}
```

## Memory Optimization

### 1. Shared Resources

Share materials and textures:

```cpp
class MaterialCache {
public:
    osg::ref_ptr<osg::Material> getMaterial(
        float r, float g, float b, float a
    ) {
        uint32_t key = packColor(r, g, b, a);

        auto it = _materials.find(key);
        if (it != _materials.end()) {
            return it->second;
        }

        osg::ref_ptr<osg::Material> material = new osg::Material();
        material->setDiffuse(osg::Material::FRONT_AND_BACK,
            osg::Vec4(r, g, b, a));
        material->setAmbient(osg::Material::FRONT_AND_BACK,
            osg::Vec4(r * 0.2f, g * 0.2f, b * 0.2f, a));

        _materials[key] = material;
        return material;
    }

private:
    std::map<uint32_t, osg::ref_ptr<osg::Material>> _materials;

    uint32_t packColor(float r, float g, float b, float a) {
        return (uint32_t(r * 255) << 24) |
               (uint32_t(g * 255) << 16) |
               (uint32_t(b * 255) << 8) |
               uint32_t(a * 255);
    }
};
```

### 2. Lazy Loading

Load geometry on demand:

```cpp
class LazyGeometryNode : public osg::Group {
public:
    LazyGeometryNode(const TopoDS_Shape& shape)
        : _shape(shape), _loaded(false) {}

    void traverse(osg::NodeVisitor& nv) override {
        if (!_loaded && nv.getVisitorType() ==
            osg::NodeVisitor::CULL_VISITOR) {
            loadGeometry();
        }
        osg::Group::traverse(nv);
    }

private:
    void loadGeometry() {
        auto geode = GeometryConverter::convertShape(_shape);
        addChild(geode);
        _loaded = true;
    }

    TopoDS_Shape _shape;
    bool _loaded;
};
```

### 3. Memory Monitoring

Track memory usage:

```cpp
struct MemoryStats {
    size_t vertexMemory = 0;
    size_t indexMemory = 0;
    size_t textureMemory = 0;

    void update(osg::Node* root) {
        vertexMemory = 0;
        indexMemory = 0;
        textureMemory = 0;

        collectStats(root);
    }

    void collectStats(osg::Node* node) {
        if (auto* geode = dynamic_cast<osg::Geode*>(node)) {
            for (unsigned int i = 0; i < geode->getNumDrawables(); i++) {
                if (auto* geom = dynamic_cast<osg::Geometry*>(
                    geode->getDrawable(i))) {

                    if (auto* verts = geom->getVertexArray()) {
                        vertexMemory += verts->getTotalDataSize();
                    }
                    for (unsigned int j = 0;
                         j < geom->getNumPrimitiveSets(); j++) {
                        indexMemory +=
                            geom->getPrimitiveSet(j)->getTotalDataSize();
                    }
                }
            }
        }

        if (auto* group = dynamic_cast<osg::Group*>(node)) {
            for (unsigned int i = 0; i < group->getNumChildren(); i++) {
                collectStats(group->getChild(i));
            }
        }
    }
};
```

## Profiling and Debugging

### 1. Enable Statistics

```cpp
void enableStats(osgViewer::Viewer* viewer) {
    // Add stats handler
    viewer->addEventHandler(new osgViewer::StatsHandler);

    // Press 's' to cycle through stats displays:
    // - Frame rate
    // - Frame time breakdown
    // - Scene statistics
    // - GPU statistics
}
```

### 2. Frame Time Analysis

```cpp
class FrameTimeAnalyzer : public osg::Camera::DrawCallback {
public:
    void operator()(osg::RenderInfo& info) const override {
        auto now = std::chrono::high_resolution_clock::now();

        if (_lastFrame.time_since_epoch().count() > 0) {
            auto duration = std::chrono::duration_cast<
                std::chrono::microseconds>(now - _lastFrame);

            _frameTimes.push_back(duration.count() / 1000.0);

            if (_frameTimes.size() > 100) {
                _frameTimes.erase(_frameTimes.begin());
            }
        }

        _lastFrame = now;
    }

    double getAverageFrameTime() const {
        if (_frameTimes.empty()) return 0.0;
        double sum = 0.0;
        for (double t : _frameTimes) sum += t;
        return sum / _frameTimes.size();
    }

    double getFPS() const {
        double avg = getAverageFrameTime();
        return avg > 0.0 ? 1000.0 / avg : 0.0;
    }

private:
    mutable std::chrono::high_resolution_clock::time_point _lastFrame;
    mutable std::vector<double> _frameTimes;
};
```

### 3. Bottleneck Identification

Common bottlenecks and solutions:

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| Low FPS, high CPU | Too many draw calls | Batch geometry |
| Low FPS, high GPU | Complex shaders/shadows | Reduce quality |
| Frame drops | Geometry updates | Cache tessellation |
| Memory growth | Geometry leaks | Check ref_ptr usage |
| Long load times | Large tessellation | Use progressive loading |

## Best Practices Summary

1. **Choose appropriate quality** - Match tessellation to use case
2. **Use LOD** - Reduce detail for distant objects
3. **Enable VBOs** - Store geometry on GPU
4. **Batch draw calls** - Combine similar objects
5. **Cache geometry** - Avoid re-tessellation
6. **Share materials** - Reduce state changes
7. **Profile regularly** - Identify bottlenecks early
8. **Optimize shadows** - Use appropriate quality
9. **Lazy load** - Load geometry on demand
10. **Monitor memory** - Track resource usage
