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
#include <Gui/View3D/IViewer3D.h>
#include <Gui/Render/Backends/OsgVerse/OsgVerseViewer.h>

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

private:
    std::unique_ptr<Render::OsgVerseViewer> _viewer;  ///< The wrapped OsgVerseViewer instance
};

} // namespace OsgVerse
} // namespace View3D
} // namespace Gui

#endif // RENDER_HAS_OSGVERSE_BACKEND

#endif // GUI_OSGVERSEVIEWERADAPTER_H
