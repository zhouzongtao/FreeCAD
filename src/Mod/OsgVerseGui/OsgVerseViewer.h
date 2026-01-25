// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_VIEWER_H
#define OSGVERSEGUI_VIEWER_H

#include "PreCompiled.h"
#include <Gui/View3D/IViewer3D.h>
#include <osg/ref_ptr>
#include <osg/Group>
#include <map>

// Forward declarations
namespace osgViewer {
    class Viewer;
}
class QWidget;
class QOpenGLWidget;

namespace OsgVerseGui {
    class OsgVerseWidget;
}

namespace OsgVerseGui {

/**
 * @brief OsgVerse implementation of Gui::View3D::IViewer3D
 * 
 * This viewer uses OpenSceneGraph (OSG) and OsgVerse for 3D rendering.
 * It implements the new unified IViewer3D interface.
 */
class OsgVerseGuiExport OsgVerseViewer : public Gui::View3D::IViewer3D {
public:
    explicit OsgVerseViewer(QWidget* parent = nullptr);
    virtual ~OsgVerseViewer();
    
    //-----------------------------------------------------------------------
    // 基础渲染接口
    //-----------------------------------------------------------------------
    void render() override;
    void resize(int width, int height) override;
    QWidget* getWidget() override;
    QOpenGLWidget* getGLWidget() override;
    
    //-----------------------------------------------------------------------
    // 场景管理
    //-----------------------------------------------------------------------
    void setSceneGraph(void* root) override;
    void* getSceneGraph() override;
    void updateScene() override;
    
    //-----------------------------------------------------------------------
    // 相机控制
    //-----------------------------------------------------------------------
    void setCamera(const Gui::View3D::CameraParams& params) override;
    Gui::View3D::CameraParams getCamera() const override;
    void viewAll() override;
    void resetCamera() override;
    void setCameraType(bool orthographic) override;
    bool isCameraOrthographic() const override;
    
    //-----------------------------------------------------------------------
    // 事件处理
    //-----------------------------------------------------------------------
    bool handleMouseEvent(QMouseEvent* event) override;
    bool handleKeyEvent(QKeyEvent* event) override;
    bool handleWheelEvent(QWheelEvent* event) override;
    
    //-----------------------------------------------------------------------
    // 拾取和选择
    //-----------------------------------------------------------------------
    Gui::View3D::PickResult pick(const QPoint& pos) override;
    void setSelectionMode(Gui::View3D::SelectionMode mode) override;
    Gui::View3D::SelectionMode getSelectionMode() const override;
    void startSelection(Gui::View3D::SelectionMode mode) override;
    void stopSelection() override;
    void abortSelection() override;
    bool isSelecting() const override;
    
    //-----------------------------------------------------------------------
    // ViewProvider 管理
    //-----------------------------------------------------------------------
    void addViewProvider(Gui::ViewProvider* vp) override;
    void removeViewProvider(Gui::ViewProvider* vp) override;
    bool hasViewProvider(Gui::ViewProvider* vp) const override;
    std::vector<Gui::ViewProvider*> getViewProviders() const override;
    
    //-----------------------------------------------------------------------
    // 渲染设置
    //-----------------------------------------------------------------------
    void setRenderMode(Gui::View3D::RenderMode mode) override;
    Gui::View3D::RenderMode getRenderMode() const override;
    void setBackgroundColor(const Base::Color& color) override;
    Base::Color getBackgroundColor() const override;
    void setBacklightEnabled(bool enabled) override;
    bool isBacklightEnabled() const override;
    
    //-----------------------------------------------------------------------
    // 导航和交互
    //-----------------------------------------------------------------------
    void setNavigationStyle(const std::string& style) override;
    std::string getNavigationStyle() const override;
    void setViewing(bool enable) override;
    bool isViewing() const override;
    
    //-----------------------------------------------------------------------
    // 后端信息
    //-----------------------------------------------------------------------
    Gui::Render::BackendType getBackendType() const override;
    std::string getBackendName() const override;
    std::string getBackendVersion() const override;
    
    //-----------------------------------------------------------------------
    // 统计和调试
    //-----------------------------------------------------------------------
    Gui::Render::RenderStats getStats() const override;
    void resetStats() override;
    void setFPSEnabled(bool enabled) override;
    bool isFPSEnabled() const override;
    
    //-----------------------------------------------------------------------
    // 高级功能
    //-----------------------------------------------------------------------
    QImage grabImage(int width = 0, int height = 0) override;
    bool saveScreenshot(const QString& filename, int width = 0, int height = 0) override;
    void setEditingViewProvider(Gui::ViewProvider* vp, int mode) override;
    Gui::ViewProvider* getEditingViewProvider() const override;
    bool isEditingViewProvider() const override;
    void resetEditingViewProvider() override;

private:
    /**
     * @brief Create a scene node for a ViewProvider
     */
    osg::ref_ptr<osg::Node> createNodeForViewProvider(Gui::ViewProvider* vp);
    
    /**
     * @brief Create a placeholder sphere for objects without geometry
     */
    osg::ref_ptr<osg::Node> createPlaceholderSphere();
    
    /**
     * @brief Apply material to a node
     */
    void applyMaterial(osg::Node* node, const Base::Color& color);

private:
    OsgVerseWidget* _widget;                              ///< Qt OpenGL widget
    osg::ref_ptr<osg::Group> _sceneRoot;                  ///< Scene root node
    std::map<Gui::ViewProvider*, osg::ref_ptr<osg::Node>> _vpNodes; ///< ViewProvider to node mapping
    
    // State
    std::string _navigationStyle;                         ///< Current navigation style
    Gui::View3D::SelectionMode _selectionMode;            ///< Current selection mode
    Gui::View3D::RenderMode _renderMode;                  ///< Current render mode
    Base::Color _backgroundColor;                         ///< Background color
    bool _backlightEnabled;                               ///< Backlight enabled
    bool _viewing;                                        ///< Viewing mode
    bool _fpsEnabled;                                     ///< FPS display enabled
    bool _orthographic;                                   ///< Orthographic camera
    Gui::ViewProvider* _editingVP;                        ///< Currently editing ViewProvider
    int _editingMode;                                     ///< Editing mode
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_VIEWER_H
