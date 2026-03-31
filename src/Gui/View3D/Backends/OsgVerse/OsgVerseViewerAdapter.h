/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef GUI_OSGVERSEVIEWERADAPTER_H
#define GUI_OSGVERSEVIEWERADAPTER_H

#ifdef RENDER_HAS_OSGVERSE_BACKEND

#include <memory>
#include <string>
#include <Gui/View3D/IViewer3D.h>
#include <Gui/Render/Backends/OsgVerse/OsgVerseViewer.h>
#include <Gui/Render/Backends/OsgVerse/OsgVersePickingService.h>

class QOpenGLWidget;

namespace Gui {
namespace View3D {
namespace OsgVerse {

/**
 * @brief Adapter class that wraps OsgVerseViewer to implement IViewer3D interface
 *
 * This adapter allows OsgVerseViewer (which implements RenderViewer) to be used
 * wherever IViewer3D is expected, following the same pattern as CoinViewer.
 */
class GuiExport OsgVerseViewerAdapter : public IViewer3D {
public:
    /**
     * @brief Constructor
     *
     * @param parent Parent widget
     * @param shareWidget Widget to share OpenGL context with
     */
    explicit OsgVerseViewerAdapter(QWidget* parent = nullptr, const QOpenGLWidget* shareWidget = nullptr);

    /**
     * @brief Destructor
     */
    ~OsgVerseViewerAdapter() override;

    //-----------------------------------------------------------------------
    // IViewer3D interface implementation
    //-----------------------------------------------------------------------

    // Basic rendering
    void render() override;
    void resize(int width, int height) override;
    QWidget* getWidget() override;
    QOpenGLWidget* getGLWidget() override;

    // Scene management
    void setSceneGraph(void* root) override;
    void* getSceneGraph() override;
    void updateScene() override;

    // Camera control
    void setCamera(const CameraParams& params) override;
    CameraParams getCamera() const override;
    void viewAll() override;
    void fitSelection() override;
    void resetCamera() override;
    void setCameraType(bool orthographic) override;
    bool isCameraOrthographic() const override;

    // Event handling
    bool handleMouseEvent(QMouseEvent* event) override;
    bool handleKeyEvent(QKeyEvent* event) override;
    bool handleWheelEvent(QWheelEvent* event) override;

    // Picking and selection
    PickResult pick(const QPoint& pos) override;
    void setSelectionMode(SelectionMode mode) override;
    SelectionMode getSelectionMode() const override;
    void startSelection(SelectionMode mode) override;
    void stopSelection() override;
    void abortSelection() override;
    bool isSelecting() const override;

    // ViewProvider management
    void addViewProvider(ViewProvider* vp) override;
    void removeViewProvider(ViewProvider* vp) override;
    void updateViewProvider(ViewProvider* vp) override;
    bool hasViewProvider(ViewProvider* vp) const override;
    std::vector<ViewProvider*> getViewProviders() const override;
    void setEditingViewProvider(ViewProvider* vp, int mode) override;
    ViewProvider* getEditingViewProvider() const override;
    bool isEditingViewProvider() const override;
    void resetEditingViewProvider() override;

    // Phase G: 编辑模式扩展
    Base::Vector3d getPointOnFocalPlane(int x, int y) const override;
    void setupEditingRoot(void* node = nullptr, const Base::Matrix4D* mat = nullptr) override;
    void resetEditingRoot(bool updateLinks = true) override;
    void setEditingTransform(const Base::Matrix4D& mat) override;

    // Phase G: Seek
    bool seekToPoint(int screenX, int screenY) override;
    void seekToPoint(const Base::Vector3d& worldPos) override;

    // Phase G: Pick radius
    float getPickRadius() const override;
    void setPickRadius(float radius) override;

    // Phase H: Override mode
    void setOverrideMode(const std::string& mode) override;
    std::string getOverrideMode() const override;

    // Rendering settings
    void setRenderMode(RenderMode mode) override;
    RenderMode getRenderMode() const override;
    void setBackgroundColor(const Base::Color& color) override;
    Base::Color getBackgroundColor() const override;
    void setBackgroundGradient(const BackgroundGradient& gradient) override;
    BackgroundGradient getBackgroundGradient() const override;
    void setBacklightEnabled(bool enabled) override;
    bool isBacklightEnabled() const override;
    void setAmbientIntensity(float intensity) override;
    float getAmbientIntensity() const override;

    // Navigation and interaction
    void setNavigationStyle(const std::string& style) override;
    std::string getNavigationStyle() const override;
    void setViewing(bool enable) override;
    bool isViewing() const override;

    // Backend info
    Render::BackendType getBackendType() const override {
        return Render::BackendType::OsgVerse;
    }
    std::string getBackendName() const override {
        return "OsgVerse";
    }
    std::string getBackendVersion() const override;

    // Statistics and debugging
    Render::RenderStats getStats() const override;
    void resetStats() override;
    void setFPSEnabled(bool enabled) override;
    bool isFPSEnabled() const override;

    // Screenshots
    QImage grabImage(int width = 0, int height = 0) override;
    bool saveScreenshot(const QString& filename, int width = 0, int height = 0) override;

    // Event callbacks
    void addEventCallback(EventType type, EventCallbackFunc cb, void* userData = nullptr) override;
    void removeEventCallback(EventType type, EventCallbackFunc cb, void* userData = nullptr) override;

    // Selection polygon
    std::vector<std::pair<int,int>> getSelectionPolygon(bool* isClosed = nullptr) const override;
    std::vector<std::pair<float,float>> getSelectionPolygonNormalized(bool* isClosed = nullptr) const override;

    // Ray picking
    Base::Vector3d getPointOnRay(const QPoint& screenPos, const ViewProvider* vp) const override;
    Base::Vector3d getPointOnRay(const Base::Vector3d& rayOrigin, const Base::Vector3d& rayDir, const ViewProvider* vp) const override;

    // Viewport on placement plane
    Base::BoundBox2d getViewportOnXYPlaneOfPlacement(const Base::Placement& plc) const override;

    // Coordinate projection (forwarded to OsgVerseViewer)
    Base::Vector3d getViewDirection() const override;
    Base::Vector3d getUpDirection() const override;
    QPoint getPointOnViewport(const Base::Vector3d& pt) const override;
    Base::Vector3d getPointOnLine(const QPoint& screenPos, const Base::Vector3d& axisCenter, const Base::Vector3d& axis) const override;
    Base::Vector3d getPointOnXYPlaneOfPlacement(const QPoint& screenPos, const Base::Placement& plc) const override;
    void projectPointToLine(const QPoint& screenPos, Base::Vector3d& pt1, Base::Vector3d& pt2) const override;
    Base::Vector2d getNormalizedPosition(const QPoint& screenPos) const override;
    Base::Vector3d projectOnNearPlane(const Base::Vector2d& pt) const override;
    Base::Vector3d projectOnFarPlane(const Base::Vector2d& pt) const override;
    Base::Vector3d getCenterPointOnFocalPlane() const override;
    void getNearPlane(Base::Vector3d& pt, Base::Vector3d& normal) const override;
    void getFarPlane(Base::Vector3d& pt, Base::Vector3d& normal) const override;
    void getDimensions(float& height, float& width) const override;
    float getMaxDimension() const override;

    // Editing extensions
    void setEditing(bool edit) override;
    bool isEditing() const override;
    void setEditingCursor(const QCursor& cursor) override;
    void setComponentCursor(const QCursor& cursor) override;
    void setRedirectToSceneGraph(bool redirect) override;
    bool isRedirectedToSceneGraph() const override;
    void setSelectionEnabled(bool enable) override;
    bool isSelectionEnabled() const override;
    void boxZoom(int x1, int y1, int x2, int y2) override;
    void savePicture(int width, int height, int samples, const QColor& bg, QImage& img) const override;
    void alignToSelection() override;
    void setPopupMenuEnabled(bool on) override;
    bool isPopupMenuEnabled() const override;

    // Graphics overlay
    void addGraphicsItem(void* item) override;
    void removeGraphicsItem(void* item) override;
    void clearGraphicsItems() override;

    /**
     * @brief Rebuild ViewProviders that had no geometry when first added
     *
     * Called after document restore to retry geometry extraction for pending VPs.
     */
    void rebuildPendingViewProviders();

private:
    std::unique_ptr<Render::OsgVerseViewer> _viewer;  ///< The wrapped OsgVerseViewer instance
    std::unique_ptr<Render::OsgVersePickingService> _pickingService;  ///< Picking service

    // Selection state
    SelectionMode _selectionMode{SelectionMode::None};
    bool _isSelecting{false};

    // Editing state
    ViewProvider* _editingViewProvider{nullptr};
    int _editingMode{0};

    // Display state
    bool _isViewing{true};
    bool _fpsEnabled{false};
    bool _cameraOrthographic{false};
    std::string _navigationStyle{"Trackball"};
    BackgroundGradient _backgroundGradient;
};

} // namespace OsgVerse
} // namespace View3D
} // namespace Gui

#endif // RENDER_HAS_OSGVERSE_BACKEND

#endif // GUI_OSGVERSEVIEWERADAPTER_H
