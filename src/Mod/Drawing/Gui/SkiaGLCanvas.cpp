/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Skia-based 2D Canvas with OpenGL GPU acceleration                    *
 *   基于 Skia 的 2D 画布，支持 OpenGL GPU 加速渲染                          *
 *                                                                         *
 *   This file implements a high-performance 2D drawing canvas using       *
 *   Google's Skia graphics library with OpenGL GPU acceleration.          *
 *   本文件实现了一个高性能的 2D 绘图画布，使用 Google Skia 图形库           *
 *   并支持 OpenGL GPU 加速。                                               *
 *                                                                         *
 *   Architecture / 架构设计:                                               *
 *   ┌─────────────────────────────────────────────────────────────────┐   *
 *   │                    SkiaGLCanvas (QOpenGLWidget)                 │   *
 *   │  ┌─────────────────────────────────────────────────────────┐   │   *
 *   │  │              SkiaGLImpl (Private Implementation)         │   │   *
 *   │  │  ┌─────────────┐    ┌─────────────────────────────┐     │   │   *
 *   │  │  │ QOpenGL     │    │  Skia GPU Context           │     │   │   *
 *   │  │  │ Context     │───>│  (GrDirectContext)          │     │   │   *
 *   │  │  │ (Dedicated) │    │  ┌─────────────────────┐    │     │   │   *
 *   │  │  └─────────────┘    │  │ GPU Surface         │    │     │   │   *
 *   │  │                     │  │ (SkSurface)         │    │     │   │   *
 *   │  │  ┌─────────────┐    │  └─────────────────────┘    │     │   │   *
 *   │  │  │ Offscreen   │    └─────────────────────────────┘     │   │   *
 *   │  │  │ Surface     │                                         │   │   *
 *   │  │  └─────────────┘    ┌─────────────────────────────┐     │   │   *
 *   │  │                     │  Cached QImage (for blit)   │     │   │   *
 *   │  │                     └─────────────────────────────┘     │   │   *
 *   │  └─────────────────────────────────────────────────────────┘   │   *
 *   └─────────────────────────────────────────────────────────────────┘   *
 *                                                                         *
 *   Key Design Decisions / 关键设计决策:                                   *
 *   1. Uses a DEDICATED OpenGL context for Skia, separate from Qt's       *
 *      使用独立的 OpenGL 上下文给 Skia，与 Qt 的上下文分离                  *
 *   2. Renders to offscreen GPU surface, then blits to screen             *
 *      渲染到离屏 GPU 表面，然后 blit 到屏幕                               *
 *   3. Automatic fallback to CPU rendering if GPU fails                   *
 *      GPU 失败时自动回退到 CPU 渲染                                       *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "SkiaGLCanvas.h"

// ============================================================================
// Qt Headers / Qt 头文件
// ============================================================================
#include <QMouseEvent>      // 鼠标事件处理
#include <QWheelEvent>      // 滚轮事件处理
#include <QKeyEvent>        // 键盘事件处理
#include <QGuiApplication>  // GUI 应用程序
#include <QScreen>          // 屏幕信息
#include <QOpenGLContext>   // OpenGL 上下文管理
#include <QOffscreenSurface> // 离屏渲染表面
#include <QPainter>         // Qt 绑定器（用于最终 blit）
#include <cmath>            // 数学函数

// ============================================================================
// Skia Core Headers / Skia 核心头文件
// ============================================================================
#include "include/core/SkCanvas.h"      // Skia 画布 - 所有绑定操作的核心
#include "include/core/SkSurface.h"     // Skia 表面 - 渲染目标
#include "include/core/SkPath.h"        // 路径对象
#include "include/core/SkPathBuilder.h" // 路径构建器
#include "include/core/SkPaint.h"       // 绘制属性（颜色、线宽等）
#include "include/core/SkColor.h"       // 颜色定义
#include "include/core/SkFont.h"        // 字体
#include "include/core/SkImageInfo.h"   // 图像信息（格式、尺寸）
#include "include/core/SkStream.h"      // 流操作（文件读写）
#include "include/core/SkData.h"        // 数据容器
#include "include/core/SkColorSpace.h"  // 色彩空间
#include "include/core/SkBitmap.h"      // 位图（用于像素读取）
#include "include/core/SkImage.h"       // 图像对象
#include "include/encode/SkPngEncoder.h" // PNG 编码器
#include "include/svg/SkSVGCanvas.h"    // SVG 导出画布

// ============================================================================
// Skia GPU Headers / Skia GPU 头文件 (Ganesh 后端)
// ============================================================================
#include "include/gpu/ganesh/GrDirectContext.h"      // GPU 直接上下文
#include "include/gpu/ganesh/SkSurfaceGanesh.h"      // GPU 表面创建
#include "include/gpu/ganesh/gl/GrGLDirectContext.h" // OpenGL GPU 上下文
#include "include/gpu/ganesh/gl/GrGLInterface.h"     // OpenGL 接口
#include "include/gpu/GpuTypes.h"                    // GPU 类型定义

#include <Base/Console.h>  // FreeCAD 控制台日志

namespace DrawingGui {

// ============================================================================
// SkiaGLImpl - Private Implementation Class / 私有实现类
// ============================================================================
/**
 * @brief Private implementation of SkiaGLCanvas using the PIMPL idiom
 *        使用 PIMPL 惯用法的 SkiaGLCanvas 私有实现
 * 
 * This class manages:
 * 本类管理以下内容：
 * 
 * 1. Dedicated OpenGL context for Skia (独立的 Skia OpenGL 上下文)
 *    - Separate from Qt's context to avoid conflicts
 *    - 与 Qt 上下文分离以避免冲突
 * 
 * 2. Offscreen rendering surface (离屏渲染表面)
 *    - QOffscreenSurface for context binding
 *    - 用于上下文绑定的 QOffscreenSurface
 * 
 * 3. Skia GPU context and surface (Skia GPU 上下文和表面)
 *    - GrDirectContext for GPU operations
 *    - SkSurface for actual rendering
 * 
 * 4. Pixel readback and caching (像素回读和缓存)
 *    - Cached QImage for efficient blitting to screen
 *    - 缓存的 QImage 用于高效地 blit 到屏幕
 */
class SkiaGLCanvas::SkiaGLImpl {
public:
    // ========================================================================
    // OpenGL Context Management / OpenGL 上下文管理
    // ========================================================================
    
    /**
     * @brief Dedicated OpenGL context for Skia
     *        Skia 专用的 OpenGL 上下文
     * 
     * Why dedicated context? 为什么需要独立上下文？
     * - Qt's QOpenGLWidget has its own context that may conflict with Skia
     * - Qt 的 QOpenGLWidget 有自己的上下文，可能与 Skia 冲突
     * - Skia modifies GL state extensively during rendering
     * - Skia 在渲染过程中会大量修改 GL 状态
     * - Separate context ensures clean state management
     * - 独立上下文确保干净的状态管理
     */
    QOpenGLContext* skiaContext = nullptr;
    
    /**
     * @brief Offscreen surface for the dedicated context
     *        独立上下文的离屏表面
     * 
     * QOffscreenSurface is required because:
     * 需要 QOffscreenSurface 因为：
     * - OpenGL context needs a surface to be made current
     * - OpenGL 上下文需要一个表面才能被激活
     * - We don't render directly to screen, so offscreen is appropriate
     * - 我们不直接渲染到屏幕，所以离屏表面是合适的
     */
    QOffscreenSurface* offscreenSurface = nullptr;
    
    // ========================================================================
    // Skia Objects / Skia 对象
    // ========================================================================
    
    /**
     * @brief Skia GPU context (Ganesh)
     *        Skia GPU 上下文 (Ganesh 后端)
     * 
     * GrDirectContext is Skia's GPU abstraction that:
     * GrDirectContext 是 Skia 的 GPU 抽象层，它：
     * - Manages GPU resources (textures, buffers, shaders)
     * - 管理 GPU 资源（纹理、缓冲区、着色器）
     * - Handles command submission to GPU
     * - 处理向 GPU 提交命令
     * - Provides caching and resource reuse
     * - 提供缓存和资源重用
     */
    sk_sp<GrDirectContext> grContext;
    
    /**
     * @brief Skia rendering surface
     *        Skia 渲染表面
     * 
     * SkSurface is the render target that:
     * SkSurface 是渲染目标，它：
     * - Provides SkCanvas for drawing operations
     * - 提供 SkCanvas 用于绑定操作
     * - Can be GPU-backed (fast) or CPU-backed (fallback)
     * - 可以是 GPU 支持的（快速）或 CPU 支持的（回退）
     */
    sk_sp<SkSurface> surface;
    
    /**
     * @brief Image format information
     *        图像格式信息
     * 
     * Stores width, height, color type, alpha type for surface creation
     * 存储表面创建所需的宽度、高度、颜色类型、Alpha 类型
     */
    SkImageInfo imageInfo;
    
    // ========================================================================
    // Cached Output / 缓存输出
    // ========================================================================
    
    /**
     * @brief Cached QImage for display
     *        用于显示的缓存 QImage
     * 
     * After GPU rendering, pixels are read back to this QImage.
     * GPU 渲染后，像素被回读到这个 QImage。
     * This is then blitted to screen using QPainter.
     * 然后使用 QPainter 将其 blit 到屏幕。
     * 
     * Performance note / 性能说明:
     * - readPixels() is the main bottleneck in this architecture
     * - readPixels() 是这个架构中的主要瓶颈
     * - For simple scenes, CPU rendering might be faster
     * - 对于简单场景，CPU 渲染可能更快
     * - GPU shines with complex geometry and effects
     * - GPU 在复杂几何和效果时表现出色
     */
    QImage cachedImage;
    
    // ========================================================================
    // State Flags / 状态标志
    // ========================================================================
    
    bool initialized = false;  // 是否已初始化
    bool gpuOk = false;        // GPU 是否可用（false = 使用 CPU 回退）
    int surfaceWidth = 0;      // 表面宽度
    int surfaceHeight = 0;     // 表面高度

    // ========================================================================
    // Destructor / 析构函数
    // ========================================================================
    ~SkiaGLImpl() {
        cleanup();
    }
    
    // ========================================================================
    // cleanup() - Release all resources / 释放所有资源
    // ========================================================================
    /**
     * @brief Clean up all OpenGL and Skia resources
     *        清理所有 OpenGL 和 Skia 资源
     * 
     * Order matters! 顺序很重要！
     * 1. Make context current (需要先激活上下文)
     * 2. Release Skia surface (释放 Skia 表面)
     * 3. Abandon GPU context (放弃 GPU 上下文)
     * 4. Release OpenGL context (释放 OpenGL 上下文)
     * 5. Release offscreen surface (释放离屏表面)
     */
    void cleanup() {
        // Step 1: Make Skia context current before cleanup
        // 步骤 1：清理前先激活 Skia 上下文
        if (skiaContext && offscreenSurface) {
            skiaContext->makeCurrent(offscreenSurface);
        }
        
        // Step 2: Release Skia surface
        // 步骤 2：释放 Skia 表面
        surface.reset();
        
        // Step 3: Abandon GPU context (tells Skia to release GPU resources)
        // 步骤 3：放弃 GPU 上下文（告诉 Skia 释放 GPU 资源）
        if (grContext) {
            grContext->abandonContext();
            grContext.reset();
        }
        
        // Step 4: Release OpenGL context
        // 步骤 4：释放 OpenGL 上下文
        if (skiaContext) {
            skiaContext->doneCurrent();
            delete skiaContext;
            skiaContext = nullptr;
        }
        
        // Step 5: Release offscreen surface
        // 步骤 5：释放离屏表面
        if (offscreenSurface) {
            delete offscreenSurface;
            offscreenSurface = nullptr;
        }
        
        initialized = false;
        gpuOk = false;
    }
    
    // ========================================================================
    // initContext() - Initialize dedicated OpenGL context for Skia
    // 初始化 Skia 专用的 OpenGL 上下文
    // ========================================================================
    /**
     * @brief Initialize a dedicated OpenGL context for Skia
     *        为 Skia 初始化一个专用的 OpenGL 上下文
     * 
     * @param shareContext Qt's OpenGL context to share resources with
     *                     用于共享资源的 Qt OpenGL 上下文
     * @return true if initialization succeeded
     *         初始化成功返回 true
     * 
     * Process / 流程:
     * 1. Create new QOpenGLContext (创建新的 QOpenGLContext)
     * 2. Set up context sharing with Qt (设置与 Qt 的上下文共享)
     * 3. Configure OpenGL format (配置 OpenGL 格式)
     * 4. Create offscreen surface (创建离屏表面)
     * 5. Create Skia GPU context (创建 Skia GPU 上下文)
     */
    bool initContext(QOpenGLContext* shareContext) {
        // Step 1: Create dedicated OpenGL context for Skia
        // 步骤 1：为 Skia 创建专用的 OpenGL 上下文
        skiaContext = new QOpenGLContext();
        
        // Step 2: Share with Qt's context for potential resource sharing
        // 步骤 2：与 Qt 上下文共享以便潜在的资源共享
        // Note: This allows sharing textures between contexts if needed
        // 注意：如果需要，这允许在上下文之间共享纹理
        if (shareContext) {
            skiaContext->setShareContext(shareContext);
        }
        
        // Step 3: Configure OpenGL format
        // 步骤 3：配置 OpenGL 格式
        // OpenGL 3.3 Core Profile is widely supported and sufficient for Skia
        // OpenGL 3.3 Core Profile 被广泛支持且对 Skia 足够
        QSurfaceFormat format;
        format.setVersion(3, 3);                        // OpenGL 3.3
        format.setProfile(QSurfaceFormat::CoreProfile); // Core Profile (no deprecated features)
        format.setDepthBufferSize(24);                  // 24-bit depth buffer
        format.setStencilBufferSize(8);                 // 8-bit stencil buffer (needed for path rendering)
        skiaContext->setFormat(format);
        
        // Create the context
        // 创建上下文
        if (!skiaContext->create()) {
            Base::Console().error("SkiaGLCanvas: Failed to create dedicated GL context\n");
            delete skiaContext;
            skiaContext = nullptr;
            return false;
        }
        
        // Step 4: Create offscreen surface for the context
        // 步骤 4：为上下文创建离屏表面
        // QOffscreenSurface is needed because OpenGL context requires a surface
        // 需要 QOffscreenSurface 因为 OpenGL 上下文需要一个表面
        offscreenSurface = new QOffscreenSurface();
        offscreenSurface->setFormat(skiaContext->format());
        offscreenSurface->create();
        
        if (!offscreenSurface->isValid()) {
            Base::Console().error("SkiaGLCanvas: Failed to create offscreen surface\n");
            delete offscreenSurface;
            offscreenSurface = nullptr;
            delete skiaContext;
            skiaContext = nullptr;
            return false;
        }
        
        // Step 5: Make Skia context current and create GrDirectContext
        // 步骤 5：激活 Skia 上下文并创建 GrDirectContext
        if (!skiaContext->makeCurrent(offscreenSurface)) {
            Base::Console().error("SkiaGLCanvas: Failed to make Skia context current\n");
            cleanup();
            return false;
        }
        
        // Create Skia GPU context using OpenGL backend
        // 使用 OpenGL 后端创建 Skia GPU 上下文
        // GrDirectContexts::MakeGL() automatically detects GL version and extensions
        // GrDirectContexts::MakeGL() 自动检测 GL 版本和扩展
        grContext = GrDirectContexts::MakeGL();
        if (!grContext) {
            Base::Console().warning("SkiaGLCanvas: GPU context failed, will use CPU\n");
            gpuOk = false;
        } else {
            gpuOk = true;
            Base::Console().message("SkiaGLCanvas: Dedicated GPU context created\n");
        }
        
        // Release context (will be made current again during rendering)
        // 释放上下文（渲染时会再次激活）
        skiaContext->doneCurrent();
        initialized = true;
        return true;
    }

    // ========================================================================
    // createSurface() - Create or recreate the rendering surface
    // 创建或重新创建渲染表面
    // ========================================================================
    /**
     * @brief Create a Skia rendering surface of the specified size
     *        创建指定大小的 Skia 渲染表面
     * 
     * @param width  Surface width in pixels / 表面宽度（像素）
     * @param height Surface height in pixels / 表面高度（像素）
     * @return true if surface creation succeeded
     *         表面创建成功返回 true
     * 
     * This method:
     * 本方法：
     * 1. Tries to create GPU-backed surface first (优先尝试创建 GPU 表面)
     * 2. Falls back to CPU raster surface if GPU fails (GPU 失败则回退到 CPU)
     */
    bool createSurface(int width, int height) {
        // Validate parameters / 验证参数
        if (width <= 0 || height <= 0) return false;
        if (!initialized) return false;
        
        // Store dimensions / 存储尺寸
        surfaceWidth = width;
        surfaceHeight = height;
        
        // Create image info with RGBA8888 premultiplied alpha format
        // 创建 RGBA8888 预乘 Alpha 格式的图像信息
        // N32 = native 32-bit format (RGBA on most platforms)
        // Premul = premultiplied alpha (better for compositing)
        imageInfo = SkImageInfo::MakeN32Premul(width, height);
        
        // Make Skia context current for surface creation
        // 激活 Skia 上下文以创建表面
        if (skiaContext && offscreenSurface) {
            skiaContext->makeCurrent(offscreenSurface);
        }
        
        // Release old surface / 释放旧表面
        surface.reset();
        
        // Try GPU surface first / 优先尝试 GPU 表面
        if (gpuOk && grContext) {
            // Create GPU-backed offscreen surface
            // 创建 GPU 支持的离屏表面
            // skgpu::Budgeted::kNo = don't count against GPU memory budget
            // skgpu::Budgeted::kNo = 不计入 GPU 内存预算
            surface = SkSurfaces::RenderTarget(
                grContext.get(),
                skgpu::Budgeted::kNo,
                imageInfo
            );
            if (surface) {
                if (skiaContext) skiaContext->doneCurrent();
                return true;
            }
            // GPU surface creation failed, fall back to CPU
            // GPU 表面创建失败，回退到 CPU
            Base::Console().warning("SkiaGLCanvas: GPU surface failed, using CPU\n");
            gpuOk = false;
        }
        
        // CPU fallback: create raster surface
        // CPU 回退：创建光栅表面
        // This is always available and doesn't require GPU
        // 这始终可用且不需要 GPU
        surface = SkSurfaces::Raster(imageInfo);
        if (skiaContext) skiaContext->doneCurrent();
        return surface != nullptr;
    }
    
    // ========================================================================
    // beginRender() - Prepare for rendering
    // 准备渲染
    // ========================================================================
    /**
     * @brief Begin a rendering pass
     *        开始一个渲染过程
     * 
     * @return SkCanvas pointer for drawing, or nullptr if failed
     *         用于绑定的 SkCanvas 指针，失败返回 nullptr
     * 
     * This method:
     * 本方法：
     * 1. Makes Skia's OpenGL context current (激活 Skia 的 OpenGL 上下文)
     * 2. Returns the canvas for drawing operations (返回用于绑定操作的画布)
     * 
     * Must be paired with endRender()!
     * 必须与 endRender() 配对使用！
     */
    SkCanvas* beginRender() {
        if (!surface) return nullptr;
        
        // Make Skia context current for rendering
        // 激活 Skia 上下文以进行渲染
        // Only needed for GPU rendering; CPU doesn't need context
        // 仅 GPU 渲染需要；CPU 不需要上下文
        if (gpuOk && skiaContext && offscreenSurface) {
            skiaContext->makeCurrent(offscreenSurface);
        }
        
        return surface->getCanvas();
    }
    
    // ========================================================================
    // endRender() - Finish rendering and read back pixels
    // 完成渲染并回读像素
    // ========================================================================
    /**
     * @brief End a rendering pass and read pixels back to CPU
     *        结束渲染过程并将像素回读到 CPU
     * 
     * This is the CRITICAL method for the offscreen rendering approach:
     * 这是离屏渲染方法的关键方法：
     * 
     * 1. Flush GPU commands (刷新 GPU 命令)
     * 2. Read pixels from GPU to CPU memory (从 GPU 读取像素到 CPU 内存)
     * 3. Store in cachedImage for later blitting (存储到 cachedImage 供后续 blit)
     * 4. Release Skia context (释放 Skia 上下文)
     * 
     * Performance note / 性能说明:
     * readPixels() is expensive because it:
     * readPixels() 开销大因为它：
     * - Stalls the GPU pipeline (阻塞 GPU 流水线)
     * - Transfers data over PCIe bus (通过 PCIe 总线传输数据)
     * - For 1920x1080 RGBA: ~8MB per frame (~8MB 每帧)
     */
    void endRender() {
        if (!surface) return;
        
        if (gpuOk && grContext) {
            // Flush all pending GPU commands
            // 刷新所有待处理的 GPU 命令
            grContext->flushAndSubmit();
            
            // Read pixels back to CPU
            // 将像素回读到 CPU
            // This is the main performance bottleneck!
            // 这是主要的性能瓶颈！
            SkBitmap bitmap;
            if (bitmap.tryAllocPixels(imageInfo)) {
                if (surface->readPixels(bitmap.pixmap(), 0, 0)) {
                    // Create QImage and copy pixel data
                    // 创建 QImage 并复制像素数据
                    cachedImage = QImage(
                        bitmap.width(), bitmap.height(),
                        QImage::Format_RGBA8888_Premultiplied
                    );
                    // Copy row by row (handles potential stride differences)
                    // 逐行复制（处理潜在的步幅差异）
                    for (int y = 0; y < bitmap.height(); ++y) {
                        memcpy(cachedImage.scanLine(y), 
                               bitmap.getAddr(0, y), 
                               bitmap.width() * 4);  // 4 bytes per pixel (RGBA)
                    }
                }
            }
        } else {
            // CPU surface - can peek pixels directly (no copy needed initially)
            // CPU 表面 - 可以直接查看像素（初始不需要复制）
            SkPixmap pixmap;
            if (surface->peekPixels(&pixmap)) {
                QImage img(
                    static_cast<const uchar*>(pixmap.addr()),
                    pixmap.width(), pixmap.height(),
                    static_cast<qsizetype>(pixmap.rowBytes()),
                    QImage::Format_RGBA8888_Premultiplied
                );
                // Must copy because pixmap data is owned by Skia surface
                // 必须复制因为 pixmap 数据由 Skia 表面拥有
                cachedImage = img.copy();
            }
        }
        
        // Release Skia context (return to Qt's context)
        // 释放 Skia 上下文（返回到 Qt 的上下文）
        if (gpuOk && skiaContext) {
            skiaContext->doneCurrent();
        }
    }
    
    /**
     * @brief Get the cached image for display
     *        获取用于显示的缓存图像
     */
    const QImage& getCachedImage() const {
        return cachedImage;
    }
};


// ============================================================================
// SkiaGLCanvas - Public Implementation / 公共实现
// ============================================================================

// ============================================================================
// Constructor / 构造函数
// ============================================================================
/**
 * @brief Construct a new SkiaGLCanvas
 *        构造一个新的 SkiaGLCanvas
 * 
 * @param parent Parent widget / 父窗口部件
 * 
 * Initializes:
 * 初始化：
 * - Mouse tracking for cursor position updates (鼠标跟踪以更新光标位置)
 * - Strong focus policy for keyboard input (强焦点策略以接收键盘输入)
 * - Private implementation object (私有实现对象)
 */
SkiaGLCanvas::SkiaGLCanvas(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_impl(std::make_unique<SkiaGLImpl>())
{
    // Enable mouse tracking to receive mouseMoveEvent even without button pressed
    // 启用鼠标跟踪以在没有按下按钮时也能接收 mouseMoveEvent
    setMouseTracking(true);
    
    // Strong focus allows receiving keyboard events
    // 强焦点允许接收键盘事件
    setFocusPolicy(Qt::StrongFocus);
    
    Base::Console().message("SkiaGLCanvas: Created\n");
}

// ============================================================================
// Destructor / 析构函数
// ============================================================================
SkiaGLCanvas::~SkiaGLCanvas()
{
    m_timer.stop();
    // Private implementation handles its own cleanup
    // 私有实现处理自己的清理
    m_impl->cleanup();
}

// ============================================================================
// initializeGL() - Called once when OpenGL context is ready
// OpenGL 上下文就绪时调用一次
// ============================================================================
/**
 * @brief Initialize OpenGL resources
 *        初始化 OpenGL 资源
 * 
 * Called by Qt when the OpenGL context is first made current.
 * 当 OpenGL 上下文首次被激活时由 Qt 调用。
 * 
 * This is where we:
 * 在这里我们：
 * 1. Initialize OpenGL functions (初始化 OpenGL 函数)
 * 2. Create Skia's dedicated context (创建 Skia 的专用上下文)
 * 3. Create initial rendering surface (创建初始渲染表面)
 */
void SkiaGLCanvas::initializeGL()
{
    // Initialize OpenGL functions from QOpenGLFunctions
    // 从 QOpenGLFunctions 初始化 OpenGL 函数
    initializeOpenGLFunctions();
    
    // Log the OpenGL renderer for debugging
    // 记录 OpenGL 渲染器以便调试
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    Base::Console().message("SkiaGLCanvas: Qt GL Renderer: %s\n", renderer ? renderer : "?");
    
    // Initialize Skia with its own dedicated context
    // 使用专用上下文初始化 Skia
    // Share with Qt's context for potential resource sharing
    // 与 Qt 上下文共享以便潜在的资源共享
    QOpenGLContext* qtContext = QOpenGLContext::currentContext();
    if (!m_impl->initContext(qtContext)) {
        Base::Console().error("SkiaGLCanvas: Failed to init Skia context\n");
        return;
    }
    
    // Create initial surface with current widget size
    // 使用当前窗口部件大小创建初始表面
    int w = width() > 0 ? width() : 800;
    int h = height() > 0 ? height() : 600;
    if (!m_impl->createSurface(w, h)) {
        Base::Console().error("SkiaGLCanvas: Failed to create initial surface\n");
    }
    
    // Start elapsed timer for potential animations
    // 启动计时器以支持潜在的动画
    m_elapsedTimer.start();
    
    Base::Console().message("SkiaGLCanvas: Init done (GPU=%s)\n", m_impl->gpuOk ? "yes" : "no");
}

// ============================================================================
// resizeGL() - Called when widget is resized
// 窗口部件大小改变时调用
// ============================================================================
/**
 * @brief Handle widget resize
 *        处理窗口部件大小改变
 * 
 * @param w New width / 新宽度
 * @param h New height / 新高度
 * 
 * Recreates the Skia surface to match the new size.
 * 重新创建 Skia 表面以匹配新大小。
 */
void SkiaGLCanvas::resizeGL(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (!m_impl || !m_impl->initialized) return;
    
    // Update OpenGL viewport
    // 更新 OpenGL 视口
    glViewport(0, 0, w, h);
    
    // Recreate Skia surface with new size
    // 使用新大小重新创建 Skia 表面
    m_impl->createSurface(w, h);
}

/**
 * @brief Manual Skia surface initialization
 *        手动 Skia 表面初始化
 */
void SkiaGLCanvas::initSkiaGL(int w, int h)
{
    m_impl->createSurface(w, h);
}

/**
 * @brief Manual Skia cleanup
 *        手动 Skia 清理
 */
void SkiaGLCanvas::cleanupSkia()
{
    m_impl->cleanup();
}

// ============================================================================
// paintGL() - Main rendering entry point
// 主渲染入口点
// ============================================================================
/**
 * @brief Render the canvas content
 *        渲染画布内容
 * 
 * This is the main rendering method called by Qt.
 * 这是 Qt 调用的主渲染方法。
 * 
 * Rendering pipeline / 渲染流水线:
 * 
 * ┌─────────────────────────────────────────────────────────────┐
 * │ 1. beginRender() - Switch to Skia's OpenGL context          │
 * │    切换到 Skia 的 OpenGL 上下文                              │
 * ├─────────────────────────────────────────────────────────────┤
 * │ 2. render(canvas) - Draw all content to Skia surface        │
 * │    将所有内容绘制到 Skia 表面                                │
 * │    - Clear background (清除背景)                             │
 * │    - Apply view transform (应用视图变换)                     │
 * │    - Draw grid (绘制网格)                                    │
 * │    - Draw geometry (绘制几何图形)                            │
 * │    - Draw temp geometry (绘制临时几何图形)                   │
 * │    - Draw cursor info (绘制光标信息)                         │
 * ├─────────────────────────────────────────────────────────────┤
 * │ 3. endRender() - Read pixels back to CPU                    │
 * │    将像素回读到 CPU                                          │
 * ├─────────────────────────────────────────────────────────────┤
 * │ 4. QPainter::drawImage() - Blit to screen                   │
 * │    将图像 blit 到屏幕                                        │
 * └─────────────────────────────────────────────────────────────┘
 */
void SkiaGLCanvas::paintGL()
{
    // Early exit if not ready
    // 如果未就绪则提前退出
    if (!isVisible() || !m_impl || !m_impl->initialized) return;
    
    // Ensure we have a valid surface
    // 确保我们有一个有效的表面
    if (!m_impl->surface) {
        int w = width() > 0 ? width() : 100;
        int h = height() > 0 ? height() : 100;
        if (!m_impl->createSurface(w, h)) return;
    }
    
    // Step 1: Begin rendering with Skia's dedicated context
    // 步骤 1：使用 Skia 的专用上下文开始渲染
    SkCanvas* canvas = m_impl->beginRender();
    if (!canvas) return;
    
    // Step 2: Render all content to Skia surface
    // 步骤 2：将所有内容渲染到 Skia 表面
    render(canvas);
    
    // Step 3: End rendering and read back pixels
    // 步骤 3：结束渲染并回读像素
    m_impl->endRender();
    
    // Step 4: Blit the cached image to screen using Qt
    // 步骤 4：使用 Qt 将缓存图像 blit 到屏幕
    // Now we're back in Qt's context
    // 现在我们回到了 Qt 的上下文
    const QImage& img = m_impl->getCachedImage();
    if (!img.isNull()) {
        QPainter painter(this);
        painter.drawImage(0, 0, img);
    }
}


// ============================================================================
// render() - Main rendering logic
// 主渲染逻辑
// ============================================================================
/**
 * @brief Render all canvas content
 *        渲染所有画布内容
 * 
 * @param canvas Skia canvas to draw on / 要绑定的 Skia 画布
 * 
 * Coordinate system / 坐标系统:
 * 
 * Screen coordinates (屏幕坐标):
 * ┌─────────────────────┐
 * │ (0,0)          (w,0)│  Y increases downward
 * │                     │  Y 向下增加
 * │       center        │
 * │                     │
 * │ (0,h)          (w,h)│
 * └─────────────────────┘
 * 
 * World coordinates after transform (变换后的世界坐标):
 * ┌─────────────────────┐
 * │                     │  Y increases upward (standard math)
 * │       (0,+)         │  Y 向上增加（标准数学坐标系）
 * │ (-,0) origin (+,0)  │
 * │       (0,-)         │
 * │                     │
 * └─────────────────────┘
 */
void SkiaGLCanvas::render(SkCanvas* canvas)
{
    if (!canvas) return;
    
    // Clear background with background color
    // 使用背景色清除背景
    canvas->clear(m_backgroundColor);
    
    // Save canvas state before applying transforms
    // 在应用变换前保存画布状态
    canvas->save();
    
    // Apply view transformation:
    // 应用视图变换：
    // 1. Translate origin to center of widget + pan offset
    //    将原点平移到窗口部件中心 + 平移偏移
    // 2. Scale by zoom factor, flip Y axis (negative scale)
    //    按缩放因子缩放，翻转 Y 轴（负缩放）
    canvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    canvas->scale(m_zoom, -m_zoom);  // Negative Y to flip coordinate system
    
    // Draw layers in order (back to front)
    // 按顺序绘制图层（从后到前）
    if (m_showGrid) drawGrid(canvas);  // Grid is behind everything
    drawGeometry(canvas);               // Permanent geometry
    drawTempGeometry(canvas);           // Temporary/preview geometry
    
    // Restore canvas state (removes transforms)
    // 恢复画布状态（移除变换）
    canvas->restore();
    
    // Draw cursor info in screen coordinates (not affected by transforms)
    // 在屏幕坐标中绘制光标信息（不受变换影响）
    drawCursor(canvas);
}

// ============================================================================
// drawGrid() - Draw background grid
// 绘制背景网格
// ============================================================================
/**
 * @brief Draw the background grid
 *        绘制背景网格
 * 
 * @param canvas Skia canvas / Skia 画布
 * 
 * Features / 特性:
 * - Draws vertical and horizontal grid lines (绘制垂直和水平网格线)
 * - Highlights X and Y axes (高亮 X 和 Y 轴)
 * - Only draws visible portion for performance (仅绘制可见部分以提高性能)
 * - Limits max lines to prevent performance issues (限制最大线数以防止性能问题)
 */
void SkiaGLCanvas::drawGrid(SkCanvas* canvas)
{
    if (!canvas) return;
    
    // Set up paint for grid lines
    // 设置网格线的绑定属性
    SkPaint paint;
    paint.setColor(m_gridColor);
    paint.setStrokeWidth(1.0f / m_zoom);  // Constant screen-space width
    paint.setAntiAlias(true);
    
    // Calculate visible area in world coordinates
    // 计算世界坐标中的可见区域
    float halfW = (width() / 2.0f) / m_zoom;
    float halfH = (height() / 2.0f) / m_zoom;
    float left = -m_pan.x / m_zoom - halfW;
    float right = -m_pan.x / m_zoom + halfW;
    float bottom = m_pan.y / m_zoom - halfH;
    float top = m_pan.y / m_zoom + halfH;
    
    // Snap to grid spacing
    // 对齐到网格间距
    float startX = std::floor(left / m_gridSpacing) * m_gridSpacing;
    float startY = std::floor(bottom / m_gridSpacing) * m_gridSpacing;
    
    // Draw grid lines with limit to prevent performance issues
    // 绘制网格线，限制数量以防止性能问题
    int maxLines = 200, lineCount = 0;
    
    // Vertical lines / 垂直线
    for (float x = startX; x <= right && lineCount < maxLines; x += m_gridSpacing, lineCount++)
        canvas->drawLine(x, bottom, x, top, paint);
    
    // Horizontal lines / 水平线
    for (float y = startY; y <= top && lineCount < maxLines; y += m_gridSpacing, lineCount++)
        canvas->drawLine(left, y, right, y, paint);
    
    // Draw axes with different style (thicker, different color)
    // 用不同样式绘制坐标轴（更粗，不同颜色）
    paint.setColor(0xFFCCCCCC);  // Light gray
    paint.setStrokeWidth(2.0f / m_zoom);
    canvas->drawLine(left, 0, right, 0, paint);   // X axis / X 轴
    canvas->drawLine(0, bottom, 0, top, paint);   // Y axis / Y 轴
}

// ============================================================================
// drawGeometry() - Draw all permanent geometry
// 绘制所有永久几何图形
// ============================================================================
/**
 * @brief Draw all stored geometry (lines, circles, rectangles)
 *        绘制所有存储的几何图形（线、圆、矩形）
 * 
 * @param canvas Skia canvas / Skia 画布
 */
void SkiaGLCanvas::drawGeometry(SkCanvas* canvas)
{
    if (!canvas) return;
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    
    // Draw all lines / 绘制所有线
    for (const auto& line : m_lines) {
        paint.setColor(line.color);
        paint.setStrokeWidth(line.strokeWidth / m_zoom);
        canvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, paint);
    }
    
    // Draw all circles / 绘制所有圆
    for (const auto& circle : m_circles) {
        paint.setColor(circle.color);
        paint.setStrokeWidth(circle.strokeWidth / m_zoom);
        paint.setStyle(circle.filled ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
        canvas->drawCircle(circle.center.x, circle.center.y, circle.radius, paint);
    }
    
    // Draw all rectangles / 绘制所有矩形
    for (const auto& rect : m_rects) {
        paint.setColor(rect.color);
        paint.setStrokeWidth(rect.strokeWidth / m_zoom);
        paint.setStyle(rect.filled ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
        canvas->drawRect(SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height), paint);
    }
}

// ============================================================================
// drawTempGeometry() - Draw temporary/preview geometry
// 绘制临时/预览几何图形
// ============================================================================
/**
 * @brief Draw temporary geometry during drawing operations
 *        在绑定操作期间绘制临时几何图形
 * 
 * @param canvas Skia canvas / Skia 画布
 * 
 * This shows the "rubber band" preview while user is drawing.
 * 这显示用户绑定时的"橡皮筋"预览。
 */
void SkiaGLCanvas::drawTempGeometry(SkCanvas* canvas)
{
    if (!canvas || m_tempPoints.empty()) return;
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xFF0066FF);  // Blue for preview / 预览用蓝色
    paint.setStrokeWidth(2.0f / m_zoom);
    
    // Draw preview based on current draw mode
    // 根据当前绑定模式绘制预览
    switch (m_drawMode) {
        case DrawMode::Line:
            // Line from first point to cursor
            // 从第一个点到光标的线
            if (!m_tempPoints.empty())
                canvas->drawLine(m_tempPoints[0].x, m_tempPoints[0].y, m_cursorWorld.x, m_cursorWorld.y, paint);
            break;
            
        case DrawMode::Circle:
            // Circle with center at first point, radius to cursor
            // 以第一个点为圆心，到光标的距离为半径的圆
            if (!m_tempPoints.empty()) {
                float r = std::sqrt(std::pow(m_cursorWorld.x - m_tempPoints[0].x, 2) + 
                                   std::pow(m_cursorWorld.y - m_tempPoints[0].y, 2));
                if (r > 0.001f) canvas->drawCircle(m_tempPoints[0].x, m_tempPoints[0].y, r, paint);
            }
            break;
            
        case DrawMode::Rectangle:
            // Rectangle from first point to cursor
            // 从第一个点到光标的矩形
            if (!m_tempPoints.empty()) {
                float x = std::min(m_tempPoints[0].x, m_cursorWorld.x);
                float y = std::min(m_tempPoints[0].y, m_cursorWorld.y);
                float w = std::abs(m_cursorWorld.x - m_tempPoints[0].x);
                float h = std::abs(m_cursorWorld.y - m_tempPoints[0].y);
                if (w > 0.001f && h > 0.001f) canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), paint);
            }
            break;
            
        default: break;
    }
    
    // Draw control points as red dots
    // 将控制点绘制为红点
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(0xFFFF0000);  // Red / 红色
    for (const auto& pt : m_tempPoints)
        canvas->drawCircle(pt.x, pt.y, 4.0f / m_zoom, paint);
}

// ============================================================================
// drawCursor() - Draw cursor position info
// 绘制光标位置信息
// ============================================================================
/**
 * @brief Draw cursor position and status info
 *        绘制光标位置和状态信息
 * 
 * @param canvas Skia canvas / Skia 画布
 * 
 * Displays: X, Y coordinates and GPU/CPU status
 * 显示：X、Y 坐标和 GPU/CPU 状态
 */
void SkiaGLCanvas::drawCursor(SkCanvas* canvas)
{
    if (!canvas) return;
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(0xFF333333);  // Dark gray text / 深灰色文字
    
    SkFont font;
    font.setSize(12);
    
    // Format status text / 格式化状态文本
    char text[64];
    snprintf(text, sizeof(text), "X: %.2f  Y: %.2f  [%s]", 
             m_cursorWorld.x, m_cursorWorld.y, m_impl->gpuOk ? "GPU" : "CPU");
    
    // Draw at bottom-left corner / 在左下角绘制
    canvas->drawString(text, 10, height() - 10, font, paint);
}


// ============================================================================
// Coordinate Transformation / 坐标变换
// ============================================================================

/**
 * @brief Convert screen coordinates to world coordinates
 *        将屏幕坐标转换为世界坐标
 * 
 * @param p Screen position (pixels) / 屏幕位置（像素）
 * @return World position / 世界位置
 * 
 * Formula / 公式:
 * worldX = (screenX - centerX - panX) / zoom
 * worldY = -(screenY - centerY - panY) / zoom  (negative because Y is flipped)
 */
SkiaPoint SkiaGLCanvas::screenToWorld(const QPoint& p) const
{
    return SkiaPoint(
        (p.x() - width() / 2.0f - m_pan.x) / m_zoom,
        -(p.y() - height() / 2.0f - m_pan.y) / m_zoom  // Negative for Y flip
    );
}

/**
 * @brief Convert world coordinates to screen coordinates
 *        将世界坐标转换为屏幕坐标
 * 
 * @param p World position / 世界位置
 * @return Screen position (pixels) / 屏幕位置（像素）
 * 
 * Inverse of screenToWorld()
 * screenToWorld() 的逆运算
 */
QPoint SkiaGLCanvas::worldToScreen(const SkiaPoint& p) const
{
    return QPoint(
        static_cast<int>(p.x * m_zoom + width() / 2.0f + m_pan.x),
        static_cast<int>(-p.y * m_zoom + height() / 2.0f + m_pan.y)
    );
}

// ============================================================================
// Mouse Event Handlers / 鼠标事件处理器
// ============================================================================

/**
 * @brief Handle mouse press events
 *        处理鼠标按下事件
 * 
 * @param event Mouse event / 鼠标事件
 * 
 * Actions / 动作:
 * - Middle button: Start panning (中键：开始平移)
 * - Left button: Add point or finish drawing (左键：添加点或完成绘制)
 * - Right button: Cancel drawing (右键：取消绘制)
 */
void SkiaGLCanvas::mousePressEvent(QMouseEvent* event)
{
    if (!event) return;
    
    // Update cursor position / 更新光标位置
    m_lastMousePos = event->pos();
    m_cursorWorld = screenToWorld(event->pos());
    
    // Middle button: Start panning / 中键：开始平移
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    
    // Left button: Drawing operations / 左键：绑定操作
    if (event->button() == Qt::LeftButton) {
        if (m_drawMode == DrawMode::Line || m_drawMode == DrawMode::Circle || m_drawMode == DrawMode::Rectangle) {
            if (m_tempPoints.empty()) {
                // First click: Start drawing / 第一次点击：开始绘制
                m_tempPoints.push_back(m_cursorWorld);
                m_isDrawing = true;
            } else {
                // Second click: Finish drawing / 第二次点击：完成绘制
                finishCurrentDrawing();
            }
        } else if (m_drawMode == DrawMode::Polyline) {
            // Polyline: Add point / 折线：添加点
            m_tempPoints.push_back(m_cursorWorld);
            m_isDrawing = true;
        }
    }
    
    // Right button: Cancel drawing / 右键：取消绘制
    if (event->button() == Qt::RightButton && m_isDrawing) {
        cancelCurrentDrawing();
    }
    
    update();  // Request repaint / 请求重绑定
}

/**
 * @brief Handle mouse move events
 *        处理鼠标移动事件
 * 
 * @param event Mouse event / 鼠标事件
 * 
 * Actions / 动作:
 * - Update cursor world position (更新光标世界位置)
 * - If panning, update pan offset (如果正在平移，更新平移偏移)
 * - Emit cursor position signal (发射光标位置信号)
 */
void SkiaGLCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (!event) return;
    
    // Calculate mouse delta / 计算鼠标增量
    QPoint delta = event->pos() - m_lastMousePos;
    
    // Update cursor world position / 更新光标世界位置
    m_cursorWorld = screenToWorld(event->pos());
    
    // Handle panning / 处理平移
    if (m_isPanning) {
        m_pan.x += delta.x();
        m_pan.y += delta.y();
        Q_EMIT viewChanged();
    }
    
    m_lastMousePos = event->pos();
    
    // Emit cursor position for status bar etc.
    // 发射光标位置供状态栏等使用
    Q_EMIT cursorPositionChanged(m_cursorWorld.x, m_cursorWorld.y);
    
    update();  // Request repaint for rubber band update
}

/**
 * @brief Handle mouse release events
 *        处理鼠标释放事件
 * 
 * @param event Mouse event / 鼠标事件
 */
void SkiaGLCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (!event) return;
    
    // End panning / 结束平移
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

/**
 * @brief Handle mouse wheel events for zooming
 *        处理鼠标滚轮事件以进行缩放
 * 
 * @param event Wheel event / 滚轮事件
 * 
 * Zoom is centered on cursor position.
 * 缩放以光标位置为中心。
 */
void SkiaGLCanvas::wheelEvent(QWheelEvent* event)
{
    if (!event) return;
    
    // Calculate zoom factor / 计算缩放因子
    float factor = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    
    // Get cursor position / 获取光标位置
    QPoint pos = event->position().toPoint();
    
    // Zoom towards cursor: remember world position before zoom
    // 向光标缩放：记住缩放前的世界位置
    SkiaPoint before = screenToWorld(pos);
    
    // Apply zoom with limits / 应用缩放并限制范围
    m_zoom = std::clamp(m_zoom * factor, 0.01f, 100.0f);
    
    // Calculate world position after zoom / 计算缩放后的世界位置
    SkiaPoint after = screenToWorld(pos);
    
    // Adjust pan to keep cursor at same world position
    // 调整平移以保持光标在相同的世界位置
    m_pan.x += (after.x - before.x) * m_zoom;
    m_pan.y -= (after.y - before.y) * m_zoom;
    
    Q_EMIT viewChanged();
    update();
}

/**
 * @brief Handle keyboard events
 *        处理键盘事件
 * 
 * @param event Key event / 键盘事件
 * 
 * Escape: Cancel current drawing and reset mode
 * Escape：取消当前绘制并重置模式
 */
void SkiaGLCanvas::keyPressEvent(QKeyEvent* event)
{
    if (!event) return;
    
    if (event->key() == Qt::Key_Escape) {
        cancelCurrentDrawing();
        setDrawMode(DrawMode::None);
    }
    
    QOpenGLWidget::keyPressEvent(event);
}


// ============================================================================
// Drawing Operations / 绑定操作
// ============================================================================

/**
 * @brief Finish the current drawing operation
 *        完成当前绘制操作
 * 
 * Creates the appropriate geometry object based on draw mode
 * and adds it to the geometry storage.
 * 根据绘制模式创建相应的几何对象并添加到几何存储中。
 */
void SkiaGLCanvas::finishCurrentDrawing()
{
    if (m_drawMode == DrawMode::Line && !m_tempPoints.empty()) {
        // Create line from first point to cursor
        // 从第一个点到光标创建线
        SkiaLine line{m_tempPoints[0], m_cursorWorld, m_currentStrokeWidth, m_currentColor};
        m_lines.push_back(line);
        Q_EMIT lineCreated(line);
        
    } else if (m_drawMode == DrawMode::Circle && !m_tempPoints.empty()) {
        // Create circle with center at first point
        // 以第一个点为圆心创建圆
        float r = std::sqrt(std::pow(m_cursorWorld.x - m_tempPoints[0].x, 2) + 
                           std::pow(m_cursorWorld.y - m_tempPoints[0].y, 2));
        SkiaCircle circle{m_tempPoints[0], r, m_currentStrokeWidth, m_currentColor, false};
        m_circles.push_back(circle);
        Q_EMIT circleCreated(circle);
        
    } else if (m_drawMode == DrawMode::Rectangle && !m_tempPoints.empty()) {
        // Create rectangle from first point to cursor
        // 从第一个点到光标创建矩形
        SkiaRect rect;
        rect.topLeft = SkiaPoint(std::min(m_tempPoints[0].x, m_cursorWorld.x),
                                  std::min(m_tempPoints[0].y, m_cursorWorld.y));
        rect.width = std::abs(m_cursorWorld.x - m_tempPoints[0].x);
        rect.height = std::abs(m_cursorWorld.y - m_tempPoints[0].y);
        rect.strokeWidth = m_currentStrokeWidth;
        rect.color = m_currentColor;
        rect.filled = false;
        m_rects.push_back(rect);
        Q_EMIT rectCreated(rect);
    }
    
    // Clear temporary state / 清除临时状态
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

/**
 * @brief Cancel the current drawing operation
 *        取消当前绘制操作
 */
void SkiaGLCanvas::cancelCurrentDrawing()
{
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

// ============================================================================
// Public API - Drawing Mode / 公共 API - 绘制模式
// ============================================================================

/**
 * @brief Set the current drawing mode
 *        设置当前绘制模式
 * 
 * @param mode New drawing mode / 新的绘制模式
 * 
 * Modes / 模式:
 * - None: Selection/navigation (选择/导航)
 * - Line: Draw lines (绘制线)
 * - Circle: Draw circles (绘制圆)
 * - Rectangle: Draw rectangles (绘制矩形)
 * - Polyline: Draw polylines (绘制折线)
 * - Pan: Pan view (平移视图)
 */
void SkiaGLCanvas::setDrawMode(DrawMode mode)
{
    // Cancel any in-progress drawing / 取消任何进行中的绘制
    if (m_isDrawing) {
        cancelCurrentDrawing();
    }
    
    m_drawMode = mode;
    
    // Set appropriate cursor / 设置适当的光标
    switch (mode) {
        case DrawMode::None:
            setCursor(Qt::ArrowCursor);
            break;
        case DrawMode::Pan:
            setCursor(Qt::OpenHandCursor);
            break;
        default:
            setCursor(Qt::CrossCursor);  // Crosshair for drawing
            break;
    }
    
    Base::Console().message("SkiaGLCanvas: setDrawMode %d\n", static_cast<int>(mode));
}

// ============================================================================
// Public API - View Transformation / 公共 API - 视图变换
// ============================================================================

/**
 * @brief Set zoom level
 *        设置缩放级别
 * 
 * @param zoom New zoom level (clamped to 0.01-100)
 *             新的缩放级别（限制在 0.01-100）
 */
void SkiaGLCanvas::setZoom(float zoom)
{
    m_zoom = std::clamp(zoom, 0.01f, 100.0f);
    Q_EMIT viewChanged();
    update();
}

/**
 * @brief Set pan offset
 *        设置平移偏移
 * 
 * @param x X offset in screen pixels / X 偏移（屏幕像素）
 * @param y Y offset in screen pixels / Y 偏移（屏幕像素）
 */
void SkiaGLCanvas::setPan(float x, float y)
{
    m_pan.x = x;
    m_pan.y = y;
    Q_EMIT viewChanged();
    update();
}

/**
 * @brief Reset view to default (zoom=1, pan=0)
 *        重置视图到默认值（zoom=1, pan=0）
 */
void SkiaGLCanvas::resetView()
{
    m_zoom = 1.0f;
    m_pan = SkiaPoint(0, 0);
    Q_EMIT viewChanged();
    update();
}

/**
 * @brief Zoom to fit all geometry in view
 *        缩放以使所有几何图形适合视图
 * 
 * Calculates bounding box of all geometry and adjusts
 * zoom and pan to show everything with 80% margin.
 * 计算所有几何图形的边界框，并调整缩放和平移
 * 以 80% 的边距显示所有内容。
 */
void SkiaGLCanvas::zoomToFit()
{
    // If no geometry, just reset view / 如果没有几何图形，只重置视图
    if (m_lines.empty() && m_circles.empty() && m_rects.empty()) {
        resetView();
        return;
    }
    
    // Calculate bounding box / 计算边界框
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    // Include all lines / 包含所有线
    for (const auto& line : m_lines) {
        minX = std::min({minX, line.start.x, line.end.x});
        maxX = std::max({maxX, line.start.x, line.end.x});
        minY = std::min({minY, line.start.y, line.end.y});
        maxY = std::max({maxY, line.start.y, line.end.y});
    }
    
    // Include all circles / 包含所有圆
    for (const auto& circle : m_circles) {
        minX = std::min(minX, circle.center.x - circle.radius);
        maxX = std::max(maxX, circle.center.x + circle.radius);
        minY = std::min(minY, circle.center.y - circle.radius);
        maxY = std::max(maxY, circle.center.y + circle.radius);
    }
    
    // Include all rectangles / 包含所有矩形
    for (const auto& rect : m_rects) {
        minX = std::min(minX, rect.topLeft.x);
        maxX = std::max(maxX, rect.topLeft.x + rect.width);
        minY = std::min(minY, rect.topLeft.y);
        maxY = std::max(maxY, rect.topLeft.y + rect.height);
    }
    
    // Calculate zoom and pan / 计算缩放和平移
    float contentW = maxX - minX;
    float contentH = maxY - minY;
    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    
    if (contentW > 0 && contentH > 0) {
        // Fit with 80% of view area / 适合 80% 的视图区域
        float zoomX = (width() * 0.8f) / contentW;
        float zoomY = (height() * 0.8f) / contentH;
        m_zoom = std::min(zoomX, zoomY);
        m_pan.x = -centerX * m_zoom;
        m_pan.y = centerY * m_zoom;
    }
    
    Q_EMIT viewChanged();
    update();
}


// ============================================================================
// Public API - Geometry Management / 公共 API - 几何管理
// ============================================================================

/**
 * @brief Add a line to the canvas
 *        向画布添加一条线
 */
void SkiaGLCanvas::addLine(const SkiaLine& line)
{
    m_lines.push_back(line);
    update();
}

/**
 * @brief Add a circle to the canvas
 *        向画布添加一个圆
 */
void SkiaGLCanvas::addCircle(const SkiaCircle& circle)
{
    m_circles.push_back(circle);
    update();
}

/**
 * @brief Add a rectangle to the canvas
 *        向画布添加一个矩形
 */
void SkiaGLCanvas::addRect(const SkiaRect& rect)
{
    m_rects.push_back(rect);
    update();
}

/**
 * @brief Clear all geometry from the canvas
 *        清除画布上的所有几何图形
 */
void SkiaGLCanvas::clearGeometry()
{
    m_lines.clear();
    m_circles.clear();
    m_rects.clear();
    update();
}

// ============================================================================
// Public API - Display Settings / 公共 API - 显示设置
// ============================================================================

/**
 * @brief Show or hide the grid
 *        显示或隐藏网格
 */
void SkiaGLCanvas::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

/**
 * @brief Set grid line spacing
 *        设置网格线间距
 */
void SkiaGLCanvas::setGridSpacing(float spacing)
{
    m_gridSpacing = spacing;
    update();
}

/**
 * @brief Set background color
 *        设置背景颜色
 * 
 * @param color ARGB color (0xAARRGGBB format)
 *              ARGB 颜色（0xAARRGGBB 格式）
 */
void SkiaGLCanvas::setBackgroundColor(uint32_t color)
{
    m_backgroundColor = color;
    update();
}

/**
 * @brief Set grid color
 *        设置网格颜色
 */
void SkiaGLCanvas::setGridColor(uint32_t color)
{
    m_gridColor = color;
    update();
}

/**
 * @brief Set current drawing color
 *        设置当前绘制颜色
 */
void SkiaGLCanvas::setCurrentColor(uint32_t color)
{
    m_currentColor = color;
}

/**
 * @brief Set current stroke width
 *        设置当前线宽
 */
void SkiaGLCanvas::setCurrentStrokeWidth(float width)
{
    m_currentStrokeWidth = width;
}

// ============================================================================
// Public API - Export Functions / 公共 API - 导出功能
// ============================================================================

/**
 * @brief Export canvas content to SVG file
 *        将画布内容导出为 SVG 文件
 * 
 * @param filename Output file path / 输出文件路径
 * @return true if export succeeded / 导出成功返回 true
 * 
 * SVG export uses Skia's SVG canvas backend.
 * SVG 导出使用 Skia 的 SVG 画布后端。
 */
bool SkiaGLCanvas::exportToSVG(const QString& filename)
{
    if (!m_impl->surface) return false;
    
    // Create SVG output stream / 创建 SVG 输出流
    SkRect bounds = SkRect::MakeWH(width(), height());
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) {
        Base::Console().error("SkiaGLCanvas: Failed to open SVG file\n");
        return false;
    }
    
    // Create SVG canvas / 创建 SVG 画布
    std::unique_ptr<SkCanvas> svgCanvas = SkSVGCanvas::Make(bounds, &stream);
    if (!svgCanvas) {
        Base::Console().error("SkiaGLCanvas: Failed to create SVG canvas\n");
        return false;
    }
    
    // Apply same transforms as render() / 应用与 render() 相同的变换
    svgCanvas->clear(m_backgroundColor);
    svgCanvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    svgCanvas->scale(m_zoom, -m_zoom);
    
    // Draw all geometry / 绘制所有几何图形
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    
    for (const auto& line : m_lines) {
        paint.setColor(line.color);
        paint.setStrokeWidth(line.strokeWidth / m_zoom);
        svgCanvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, paint);
    }
    
    for (const auto& circle : m_circles) {
        paint.setColor(circle.color);
        paint.setStrokeWidth(circle.strokeWidth / m_zoom);
        svgCanvas->drawCircle(circle.center.x, circle.center.y, circle.radius, paint);
    }
    
    for (const auto& rect : m_rects) {
        paint.setColor(rect.color);
        paint.setStrokeWidth(rect.strokeWidth / m_zoom);
        svgCanvas->drawRect(SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height), paint);
    }
    
    Base::Console().message("SkiaGLCanvas: Exported to SVG: %s\n", filename.toStdString().c_str());
    return true;
}

/**
 * @brief Export canvas content to PNG file
 *        将画布内容导出为 PNG 文件
 * 
 * @param filename Output file path / 输出文件路径
 * @return true if export succeeded / 导出成功返回 true
 * 
 * PNG export captures the current surface content.
 * PNG 导出捕获当前表面内容。
 */
bool SkiaGLCanvas::exportToPNG(const QString& filename)
{
    if (!m_impl->surface) return false;
    
    // Create image snapshot from surface / 从表面创建图像快照
    sk_sp<SkImage> image = m_impl->surface->makeImageSnapshot();
    if (!image) {
        Base::Console().error("SkiaGLCanvas: Failed to create image snapshot\n");
        return false;
    }
    
    // Encode to PNG / 编码为 PNG
    sk_sp<SkData> pngData = SkPngEncoder::Encode(nullptr, image.get(), {});
    if (!pngData) {
        Base::Console().error("SkiaGLCanvas: Failed to encode PNG\n");
        return false;
    }
    
    // Write to file / 写入文件
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) {
        Base::Console().error("SkiaGLCanvas: Failed to open PNG file\n");
        return false;
    }
    
    stream.write(pngData->data(), pngData->size());
    Base::Console().message("SkiaGLCanvas: Exported to PNG: %s\n", filename.toStdString().c_str());
    return true;
}

} // namespace DrawingGui
