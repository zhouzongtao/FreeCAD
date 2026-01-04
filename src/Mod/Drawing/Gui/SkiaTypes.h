/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Common types for Skia-based Drawing canvas                           *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWING_SKIA_TYPES_H
#define DRAWING_SKIA_TYPES_H

#include <cstdint>

namespace DrawingGui {

// Geometry types for drawing
struct SkiaPoint {
    float x, y;
    SkiaPoint(float _x = 0, float _y = 0) : x(_x), y(_y) {}
};

struct SkiaLine {
    SkiaPoint start, end;
    float strokeWidth = 2.0f;
    uint32_t color = 0xFF000000; // ARGB black
};

struct SkiaCircle {
    SkiaPoint center;
    float radius;
    float strokeWidth = 2.0f;
    uint32_t color = 0xFF000000;
    bool filled = false;
};

struct SkiaRect {
    SkiaPoint topLeft;
    float width, height;
    float strokeWidth = 2.0f;
    uint32_t color = 0xFF000000;
    bool filled = false;
};

// Drawing mode enumeration
enum class DrawMode {
    None,
    Line,
    Circle,
    Rectangle,
    Polyline,
    Pan,
    Zoom
};

} // namespace DrawingGui

#endif // DRAWING_SKIA_TYPES_H
