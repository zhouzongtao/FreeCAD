# Geometry Converter Development Guide

This guide explains how to convert FreeCAD's OCCT geometry to rendering formats.

## Overview

FreeCAD uses OpenCASCADE Technology (OCCT) for geometric modeling. The GeometryConverter class converts OCCT `TopoDS_Shape` objects to rendering-ready triangle meshes.

## OCCT Shape Hierarchy

```
TopoDS_Shape (base class)
├── TopoDS_Compound (collection of shapes)
├── TopoDS_CompSolid (collection of solids)
├── TopoDS_Solid (bounded 3D region)
│   └── TopoDS_Shell (bounding surfaces)
│       └── TopoDS_Face (single surface)
│           ├── TopoDS_Wire (boundary loop)
│           │   └── TopoDS_Edge (curve)
│           │       └── TopoDS_Vertex (point)
│           └── Poly_Triangulation (mesh data)
└── ... other types
```

## Conversion Pipeline

```
TopoDS_Shape
    │
    ▼
BRepMesh_IncrementalMesh (tessellation)
    │
    ▼
Poly_Triangulation (per face)
    │
    ▼
Extract vertices, triangles, normals
    │
    ▼
Create rendering geometry
```

## GeometryConverter API

### Header

```cpp
#include "GeometryConverter.h"

namespace OsgVerseGui {

class GeometryConverter {
public:
    // Conversion options
    struct ConversionOptions {
        double deflection = 0.1;      // Tessellation accuracy
        double angle = 0.5;           // Angular deviation (radians)
        bool computeNormals = true;   // Calculate normals
        bool relative = false;        // Use relative deflection
        bool perVertexNormals = true; // Per-vertex normals
        bool smoothNormals = false;   // Average normals
        double smoothAngle = 45.0;    // Smooth angle threshold
        QualityLevel quality = QualityLevel::Normal;
    };

    // Quality presets
    enum class QualityLevel {
        Draft,   // Fast preview
        Normal,  // Standard quality
        Fine,    // High quality
        Custom   // User-defined
    };

    // Statistics
    struct ConversionStats {
        int vertexCount = 0;
        int triangleCount = 0;
        int faceCount = 0;
        double conversionTime = 0.0;
    };

    // Main conversion method
    static osg::ref_ptr<osg::Geode> convertShape(
        const TopoDS_Shape& shape,
        const ConversionOptions& options = ConversionOptions(),
        ConversionStats* stats = nullptr
    );
};

}
```

## Step-by-Step Implementation

### Step 1: Tessellation

Use `BRepMesh_IncrementalMesh` to triangulate the shape:

```cpp
bool tessellateShape(
    const TopoDS_Shape& shape,
    double deflection,
    double angle,
    bool relative
) {
    try {
        BRepMesh_IncrementalMesh mesh(shape, deflection, relative, angle);

        if (!mesh.IsDone()) {
            return false;
        }

        return true;
    }
    catch (const Standard_Failure& e) {
        // Handle error
        return false;
    }
}
```

**Parameters:**
- `deflection`: Maximum distance from surface to mesh (smaller = finer)
- `angle`: Maximum angular deviation in radians
- `relative`: If true, deflection is relative to edge length

### Step 2: Extract Face Triangles

Iterate over faces and extract triangulation:

```cpp
bool extractFaceTriangles(
    const TopoDS_Face& face,
    std::vector<Vec3>& vertices,
    std::vector<Vec3>& normals,
    std::vector<unsigned int>& indices,
    unsigned int& vertexOffset
) {
    // Get triangulation
    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation =
        BRep_Tool::Triangulation(face, location);

    if (triangulation.IsNull()) {
        return false;
    }

    // Get transformation
    gp_Trsf transform = location.Transformation();

    // Check face orientation
    bool reversed = (face.Orientation() == TopAbs_REVERSED);

    // Extract vertices
    int nodeCount = triangulation->NbNodes();
    for (int i = 1; i <= nodeCount; i++) {
        gp_Pnt point = triangulation->Node(i).Transformed(transform);
        vertices.push_back(Vec3(point.X(), point.Y(), point.Z()));
    }

    // Extract triangles
    int triCount = triangulation->NbTriangles();
    for (int i = 1; i <= triCount; i++) {
        const Poly_Triangle& tri = triangulation->Triangle(i);

        int n1, n2, n3;
        tri.Get(n1, n2, n3);

        // Adjust winding order based on face orientation
        if (reversed) {
            indices.push_back(vertexOffset + n1 - 1);
            indices.push_back(vertexOffset + n3 - 1);
            indices.push_back(vertexOffset + n2 - 1);
        } else {
            indices.push_back(vertexOffset + n1 - 1);
            indices.push_back(vertexOffset + n2 - 1);
            indices.push_back(vertexOffset + n3 - 1);
        }
    }

    vertexOffset += nodeCount;
    return true;
}
```

### Step 3: Calculate Normals

#### Simple Face Normal

```cpp
Vec3 calculateFaceNormal(const TopoDS_Face& face, const gp_Trsf& transform) {
    BRepGProp_Face faceProp(face);
    gp_Pnt center;
    gp_Vec normal;

    faceProp.Normal(0.5, 0.5, center, normal);

    if (normal.Magnitude() > 1e-7) {
        normal.Normalize();
        normal.Transform(transform);

        if (face.Orientation() == TopAbs_REVERSED) {
            normal.Reverse();
        }

        return Vec3(normal.X(), normal.Y(), normal.Z());
    }

    return Vec3(0, 0, 1); // Default
}
```

#### Per-Vertex Normals (Smooth Surfaces)

```cpp
Vec3 calculateSurfaceNormalAtUV(
    const TopoDS_Face& face,
    double u, double v,
    const gp_Trsf& transform,
    bool reversed
) {
    BRepAdaptor_Surface surface(face);
    BRepLProp_SLProps props(surface, u, v, 1, 1e-7);

    if (props.IsNormalDefined()) {
        gp_Dir dir = props.Normal();
        gp_Vec normal(dir);

        normal.Transform(transform);

        if (reversed) {
            normal.Reverse();
        }

        normal.Normalize();
        return Vec3(normal.X(), normal.Y(), normal.Z());
    }

    return Vec3(0, 0, 1);
}

// Use UV nodes from triangulation
bool hasUVNodes = triangulation->HasUVNodes();
if (hasUVNodes) {
    for (int i = 1; i <= nodeCount; i++) {
        gp_Pnt2d uv = triangulation->UVNode(i);
        Vec3 normal = calculateSurfaceNormalAtUV(
            face, uv.X(), uv.Y(), transform, reversed
        );
        normals.push_back(normal);
    }
}
```

### Step 4: Create Render Geometry

Example for OpenSceneGraph:

```cpp
osg::ref_ptr<osg::Geometry> createOsgGeometry(
    const std::vector<Vec3>& vertices,
    const std::vector<Vec3>& normals,
    const std::vector<unsigned int>& indices
) {
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    // Vertex array
    osg::ref_ptr<osg::Vec3Array> vertexArray =
        new osg::Vec3Array(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
        (*vertexArray)[i] = osg::Vec3(
            vertices[i].x, vertices[i].y, vertices[i].z
        );
    }
    geometry->setVertexArray(vertexArray);

    // Normal array
    if (!normals.empty()) {
        osg::ref_ptr<osg::Vec3Array> normalArray =
            new osg::Vec3Array(normals.size());
        for (size_t i = 0; i < normals.size(); i++) {
            (*normalArray)[i] = osg::Vec3(
                normals[i].x, normals[i].y, normals[i].z
            );
        }
        geometry->setNormalArray(normalArray);
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    // Index array
    osg::ref_ptr<osg::DrawElementsUInt> indexArray =
        new osg::DrawElementsUInt(
            osg::PrimitiveSet::TRIANGLES, indices.size()
        );
    for (size_t i = 0; i < indices.size(); i++) {
        (*indexArray)[i] = indices[i];
    }
    geometry->addPrimitiveSet(indexArray);

    return geometry;
}
```

## Quality Settings

### Draft Quality

Fast preview, suitable for large models:

```cpp
ConversionOptions::Draft()
// deflection = 0.5
// angle = 1.0 radians
// perVertexNormals = false
```

### Normal Quality

Standard quality for interactive viewing:

```cpp
ConversionOptions::Normal()
// deflection = 0.1
// angle = 0.5 radians
// perVertexNormals = true
```

### Fine Quality

High quality for final rendering:

```cpp
ConversionOptions::Fine()
// deflection = 0.01
// angle = 0.1 radians
// perVertexNormals = true
// smoothNormals = true
```

## Performance Optimization

### 1. Batch Face Processing

Process all faces together to reduce overhead:

```cpp
void extractAllFaces(const TopoDS_Shape& shape) {
    TopExp_Explorer explorer(shape, TopAbs_FACE);

    while (explorer.More()) {
        const TopoDS_Face& face = TopoDS::Face(explorer.Current());
        // Process face
        explorer.Next();
    }
}
```

### 2. Reserve Memory

Pre-allocate containers:

```cpp
// Estimate vertex count
int estimatedVertices = estimateVertexCount(shape);
vertices.reserve(estimatedVertices);
normals.reserve(estimatedVertices);
indices.reserve(estimatedVertices * 3);
```

### 3. Use VBOs

Enable vertex buffer objects for GPU efficiency:

```cpp
geometry->setUseDisplayList(false);
geometry->setUseVertexBufferObjects(true);
```

### 4. Level of Detail (LOD)

Create multiple quality levels:

```cpp
osg::ref_ptr<osg::LOD> createLOD(const TopoDS_Shape& shape) {
    osg::ref_ptr<osg::LOD> lod = new osg::LOD();

    // High detail (close)
    auto fine = convertShape(shape, ConversionOptions::Fine());
    lod->addChild(fine, 0.0f, 50.0f);

    // Medium detail
    auto normal = convertShape(shape, ConversionOptions::Normal());
    lod->addChild(normal, 50.0f, 200.0f);

    // Low detail (far)
    auto draft = convertShape(shape, ConversionOptions::Draft());
    lod->addChild(draft, 200.0f, FLT_MAX);

    return lod;
}
```

## Common Issues

### Issue: Missing Faces

**Cause:** Tessellation failed for some faces

**Solution:**
```cpp
// Check tessellation status per face
if (triangulation.IsNull()) {
    // Try with increased deflection
    BRepMesh_IncrementalMesh mesh(face, deflection * 2);
}
```

### Issue: Inverted Normals

**Cause:** Face orientation not handled

**Solution:**
```cpp
bool reversed = (face.Orientation() == TopAbs_REVERSED);
if (reversed) {
    normal.Reverse();
    // Also swap triangle winding
}
```

### Issue: Faceted Appearance

**Cause:** Using face normals instead of vertex normals

**Solution:** Enable per-vertex normals:
```cpp
options.perVertexNormals = true;
```

### Issue: Slow Conversion

**Cause:** Too fine deflection

**Solution:** Adjust quality:
```cpp
options.deflection = 0.1; // Increase from 0.01
```

## Edge Extraction

For wireframe rendering:

```cpp
void extractEdges(
    const TopoDS_Shape& shape,
    std::vector<Vec3>& edgeVertices
) {
    TopExp_Explorer explorer(shape, TopAbs_EDGE);

    while (explorer.More()) {
        const TopoDS_Edge& edge = TopoDS::Edge(explorer.Current());

        TopLoc_Location loc;
        Handle(Poly_Polygon3D) poly =
            BRep_Tool::Polygon3D(edge, loc);

        if (!poly.IsNull()) {
            gp_Trsf transform = loc.Transformation();
            const TColgp_Array1OfPnt& nodes = poly->Nodes();

            for (int i = nodes.Lower(); i <= nodes.Upper(); i++) {
                gp_Pnt pt = nodes(i).Transformed(transform);
                edgeVertices.push_back(Vec3(pt.X(), pt.Y(), pt.Z()));
            }
        }

        explorer.Next();
    }
}
```
