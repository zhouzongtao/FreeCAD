/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   MDI View for Skia-based Drawing canvas                               *
 *                                                                         *
 *   This class uses C++20 Concepts to define a unified interface for     *
 *   multiple canvas backends (CPU, OpenGL, Metal).                       *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWING_MDIVIEW_DRAWING_H
#define DRAWING_MDIVIEW_DRAWING_H

#include <Gui/MDIView.h>
#include <QPointer>
#include <concepts>
#include <type_traits>

#include "SkiaCanvasConcept.h"

namespace DrawingGui {

class SkiaCanvas;
class SkiaOpenGLCanvas;
#ifdef __APPLE__
class SkiaMetalCanvas;
#endif

// Forward declare geometry types
struct SkiaLine;
struct SkiaCircle;

/**
 * @brief GPU backend type for rendering
 * 
 * GPU 渲染后端类型枚举
 */
enum class GpuBackend {
    CPU,        // Software raster (cross-platform) / 软件光栅化（跨平台）
    OpenGL,     // OpenGL GPU (cross-platform) / OpenGL GPU加速（跨平台）
    Metal       // Metal GPU (macOS only) / Metal GPU加速（仅macOS）
};

/**
 * @brief MDI View window for 2D Drawing using Skia
 * 
 * 基于 Skia 的 2D 绘图 MDI 视图窗口
 * 
 * This class manages multiple canvas backends through a unified interface
 * defined by the SkiaCanvasConcept. It uses C++20 template features to
 * eliminate repetitive conditional code while maintaining type safety.
 * 
 * 此类通过 SkiaCanvasConcept 定义的统一接口管理多个画布后端。
 * 使用 C++20 模板特性消除重复的条件代码，同时保持类型安全。
 * 
 * Supported backends / 支持的后端:
 * - CPU: Software raster (all platforms) / 软件光栅化（所有平台）
 * - OpenGL: GPU accelerated (Windows, Linux, macOS) / GPU加速
 * - Metal: GPU accelerated (macOS only, best performance) / GPU加速（仅macOS）
 */
class MDIViewDrawing : public Gui::MDIView
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MDIViewDrawing(Gui::Document* doc, QWidget* parent = nullptr, 
#ifdef __APPLE__
                   GpuBackend backend = GpuBackend::Metal);  // Default to Metal on macOS
#else
                   GpuBackend backend = GpuBackend::OpenGL); // Default to OpenGL on other platforms
#endif
    ~MDIViewDrawing() override;

    // Check rendering backend / 检查渲染后端
    GpuBackend backend() const { return m_backend; }
    bool isGpuRendering() const { return m_backend != GpuBackend::CPU; }
    QString backendName() const;

    // MDIView interface / MDIView 接口
    const char* getName() const override { return "MDIViewDrawing"; }
    bool onMsg(const char* pMsg, const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;
    void onUpdate() override;

    // Drawing commands / 绘图命令
    void cmdDrawLine();
    void cmdDrawCircle();
    void cmdDrawRectangle();
    void cmdZoomFit();
    void cmdZoomIn();
    void cmdZoomOut();
    void cmdExportSVG();
    void cmdExportPNG();
    void cmdClear();

protected:
    void closeEvent(QCloseEvent* event) override;

private Q_SLOTS:
    void onLineCreated(const SkiaLine& line);
    void onCircleCreated(const SkiaCircle& circle);
    void onCursorPositionChanged(float x, float y);

private:
    void setupToolbar();
    void createActions();

    /**
     * @brief Execute an operation on the active canvas (C++20 template)
     * 
     * 在活跃画布上执行操作（C++20 模板）
     * 
     * This template method eliminates repetitive if-else chains by using
     * a generic lambda that works with any canvas type satisfying
     * SkiaCanvasConcept.
     * 
     * 此模板方法通过使用泛型 lambda 消除重复的 if-else 链，
     * 该 lambda 可与任何满足 SkiaCanvasConcept 的画布类型一起工作。
     * 
     * @tparam Op Callable type (usually a lambda)
     * @param operation The operation to perform on the canvas
     * 
     * Usage / 使用示例:
     * @code
     * withActiveCanvas([](auto* canvas) {
     *     canvas->setDrawMode(DrawMode::Line);
     * });
     * @endcode
     */
    template<typename Op>
    void withActiveCanvas(Op&& operation) {
#ifdef __APPLE__
        if (m_metalCanvas) { 
            operation(m_metalCanvas); 
            return; 
        }
#endif
        if (m_glCanvas) { 
            operation(m_glCanvas); 
            return; 
        }
        if (m_cpuCanvas) { 
            operation(m_cpuCanvas); 
        }
    }

    /**
     * @brief Execute an operation on the active canvas and return a value
     * 
     * 在活跃画布上执行操作并返回值
     * 
     * @tparam R Return type
     * @tparam Op Callable type
     * @param operation The operation to perform
     * @param defaultValue Value to return if no canvas is active
     * @return Result of the operation or defaultValue
     */
    template<typename R, typename Op>
    R withActiveCanvasReturn(Op&& operation, R defaultValue = R{}) {
#ifdef __APPLE__
        if (m_metalCanvas) { 
            return operation(m_metalCanvas); 
        }
#endif
        if (m_glCanvas) { 
            return operation(m_glCanvas); 
        }
        if (m_cpuCanvas) { 
            return operation(m_cpuCanvas); 
        }
        return defaultValue;
    }

    /**
     * @brief Connect canvas signals to MDIViewDrawing slots (C++20 template)
     * 
     * 连接画布信号到 MDIViewDrawing 槽函数（C++20 模板）
     * 
     * This template method connects all standard canvas signals to the
     * corresponding slots, eliminating repetitive connection code.
     * 
     * @tparam Canvas Canvas type satisfying SkiaCanvasConcept
     * @param canvas Pointer to the canvas instance
     */
    template<typename Canvas>
    void connectCanvasSignals(Canvas* canvas) {
        connect(canvas, &Canvas::lineCreated, 
                this, &MDIViewDrawing::onLineCreated);
        connect(canvas, &Canvas::circleCreated,
                this, &MDIViewDrawing::onCircleCreated);
        connect(canvas, &Canvas::cursorPositionChanged,
                this, &MDIViewDrawing::onCursorPositionChanged);
    }

    /**
     * @brief Get the active canvas as a QWidget pointer
     * 
     * 获取活跃画布的 QWidget 指针
     * 
     * @return Pointer to the active canvas widget
     */
    QWidget* getActiveCanvasWidget();

    GpuBackend m_backend;
    
    // Canvas widgets (only one is used at a time)
    // 画布控件（同一时间只使用一个）
    SkiaCanvas* m_cpuCanvas = nullptr;
    SkiaOpenGLCanvas* m_glCanvas = nullptr;
#ifdef __APPLE__
    SkiaMetalCanvas* m_metalCanvas = nullptr;
#endif
    
    // Actions / 动作
    QAction* m_actLine = nullptr;
    QAction* m_actCircle = nullptr;
    QAction* m_actRectangle = nullptr;
    QAction* m_actZoomFit = nullptr;
    QAction* m_actZoomIn = nullptr;
    QAction* m_actZoomOut = nullptr;
    QAction* m_actExportSVG = nullptr;
    QAction* m_actExportPNG = nullptr;
    QAction* m_actClear = nullptr;
};

} // namespace DrawingGui

#endif // DRAWING_MDIVIEW_DRAWING_H
