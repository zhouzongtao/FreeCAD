/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   C++20 Concept definition for Skia Canvas interface                   *
 *                                                                         *
 *   This file defines the SkiaCanvasConcept which specifies the          *
 *   compile-time interface requirements for all canvas implementations.  *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWING_SKIA_CANVAS_CONCEPT_H
#define DRAWING_SKIA_CANVAS_CONCEPT_H

#include <concepts>
#include <type_traits>
#include <QString>
#include "SkiaTypes.h"

namespace DrawingGui {

/**
 * @brief C++20 Concept defining the interface contract for Skia canvas classes
 * 
 * C++20 Concept 定义 Skia 画布类的接口契约
 * 
 * This concept ensures that any canvas type used in the Drawing workbench
 * implements all required methods with correct signatures. The compiler
 * will verify compliance at compile-time, providing clear error messages
 * if a type doesn't satisfy the requirements.
 * 
 * 此 Concept 确保 Drawing 工作台使用的任何画布类型都实现了所有必需的方法，
 * 并具有正确的签名。编译器将在编译时验证合规性，如果类型不满足要求，
 * 将提供清晰的错误信息。
 * 
 * Benefits / 优点:
 * - Compile-time interface checking / 编译期接口检查
 * - Clear error messages when interface is not satisfied / 接口不满足时提供清晰错误信息
 * - Self-documenting interface requirements / 自文档化的接口要求
 * - Zero runtime overhead / 零运行时开销
 * 
 * Usage / 使用方法:
 * @code
 * // Verify a class satisfies the concept / 验证类满足 Concept
 * static_assert(SkiaCanvasConcept<SkiaCanvas>);
 * 
 * // Use in template constraints / 在模板约束中使用
 * template<SkiaCanvasConcept Canvas>
 * void drawOnCanvas(Canvas* canvas) { ... }
 * @endcode
 */
template<typename T>
concept SkiaCanvasConcept = requires(
    T canvas,                      // Canvas instance / 画布实例
    T* canvasPtr,                  // Canvas pointer / 画布指针
    DrawMode mode,                 // Drawing mode / 绘图模式
    float f,                       // Float parameter / 浮点参数
    bool b,                        // Boolean parameter / 布尔参数
    uint32_t color,                // Color value (ARGB) / 颜色值
    const QString& filename,       // File name / 文件名
    const SkiaLine& line,          // Line geometry / 直线几何
    const SkiaCircle& circle,      // Circle geometry / 圆几何
    const SkiaRect& rect           // Rectangle geometry / 矩形几何
) {
    // ==================== Drawing Mode / 绘图模式 ====================
    // Set current drawing mode (None, Line, Circle, Rectangle, etc.)
    // 设置当前绘图模式
    { canvas.setDrawMode(mode) } -> std::same_as<void>;
    
    // Get current drawing mode
    // 获取当前绘图模式
    { canvas.drawMode() } -> std::same_as<DrawMode>;
    
    // ==================== View Transformation / 视图变换 ====================
    // Set zoom level (1.0 = 100%)
    // 设置缩放级别
    { canvas.setZoom(f) } -> std::same_as<void>;
    
    // Get current zoom level
    // 获取当前缩放级别
    { canvas.zoom() } -> std::convertible_to<float>;
    
    // Set pan offset in world coordinates
    // 设置世界坐标系中的平移偏移
    { canvas.setPan(f, f) } -> std::same_as<void>;
    
    // Get current pan offset
    // 获取当前平移偏移
    { canvas.pan() } -> std::same_as<SkiaPoint>;
    
    // Reset view to default (zoom=1, pan=0,0)
    // 重置视图到默认状态
    { canvas.resetView() } -> std::same_as<void>;
    
    // Zoom to fit all geometry in view
    // 缩放以适应所有几何图形
    { canvas.zoomToFit() } -> std::same_as<void>;
    
    // ==================== Geometry Management / 几何管理 ====================
    // Add a line to the canvas
    // 添加直线到画布
    { canvas.addLine(line) } -> std::same_as<void>;
    
    // Add a circle to the canvas
    // 添加圆到画布
    { canvas.addCircle(circle) } -> std::same_as<void>;
    
    // Add a rectangle to the canvas
    // 添加矩形到画布
    { canvas.addRect(rect) } -> std::same_as<void>;
    
    // Clear all geometry from canvas
    // 清除画布上的所有几何图形
    { canvas.clearGeometry() } -> std::same_as<void>;
    
    // ==================== Grid Settings / 网格设置 ====================
    // Show or hide the grid
    // 显示或隐藏网格
    { canvas.setShowGrid(b) } -> std::same_as<void>;
    
    // Check if grid is visible
    // 检查网格是否可见
    { canvas.showGrid() } -> std::convertible_to<bool>;
    
    // Set grid spacing in world units
    // 设置网格间距（世界单位）
    { canvas.setGridSpacing(f) } -> std::same_as<void>;
    
    // ==================== Color Settings / 颜色设置 ====================
    // Set background color (ARGB format)
    // 设置背景颜色（ARGB格式）
    { canvas.setBackgroundColor(color) } -> std::same_as<void>;
    
    // Set grid color
    // 设置网格颜色
    { canvas.setGridColor(color) } -> std::same_as<void>;
    
    // Set current drawing color
    // 设置当前绘图颜色
    { canvas.setCurrentColor(color) } -> std::same_as<void>;
    
    // Set current stroke width
    // 设置当前线宽
    { canvas.setCurrentStrokeWidth(f) } -> std::same_as<void>;
    
    // ==================== Export Functions / 导出功能 ====================
    // Export canvas to SVG file
    // 导出画布到SVG文件
    { canvas.exportToSVG(filename) } -> std::same_as<bool>;
    
    // Export canvas to PNG file
    // 导出画布到PNG文件
    { canvas.exportToPNG(filename) } -> std::same_as<bool>;
    
    // ==================== Qt Widget Interface / Qt Widget 接口 ====================
    // Request widget repaint (inherited from QWidget)
    // 请求重绘（继承自QWidget）
    { canvasPtr->update() } -> std::same_as<void>;
};

/**
 * @brief Helper to check if a type satisfies SkiaCanvasConcept
 * 
 * 辅助模板，用于检查类型是否满足 SkiaCanvasConcept
 */
template<typename T>
inline constexpr bool is_skia_canvas_v = SkiaCanvasConcept<T>;

} // namespace DrawingGui

#endif // DRAWING_SKIA_CANVAS_CONCEPT_H
