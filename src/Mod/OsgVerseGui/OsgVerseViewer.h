// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_VIEWER_H
#define OSGVERSEGUI_VIEWER_H

#include "PreCompiled.h"
#include "OsgVerseBackground.h"
#include <Gui/View3D/IViewer3D.h>
#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Node>
#include <osg/LightSource>
#include <osg/Camera>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgShadow/ShadowedScene>
#include <osgText/Text>
#include <osgUtil/LineSegmentIntersector>
#include <map>
#include <vector>
#include <memory>

// Forward declarations
namespace osgViewer {
    class Viewer;
}
class QWidget;
class QOpenGLWidget;

namespace OsgVerseGui {
    class OsgVerseWidget;
    class PostProcessManager;
    class OsgVerseNaviCube;
}

namespace OsgVerseGui {

/**
 * @brief UserData class to store ViewProvider reference in OSG nodes
 *
 * This allows efficient lookup of ViewProvider from picked nodes
 */
class ViewProviderUserData : public osg::Referenced {
public:
    ViewProviderUserData(Gui::ViewProvider* vp) : viewProvider(vp) {}
    Gui::ViewProvider* viewProvider;
};

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
    void fitSelection() override;
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

    /**
     * @brief 区域拾取 - 选取矩形区域内的所有对象
     *
     * 使用 osgUtil::PolytopeIntersector 实现视锥体拾取
     *
     * @param x1 矩形左上角X坐标 (Qt坐标系)
     * @param y1 矩形左上角Y坐标 (Qt坐标系)
     * @param x2 矩形右下角X坐标 (Qt坐标系)
     * @param y2 矩形右下角Y坐标 (Qt坐标系)
     * @param pickVisibleOnly 仅选择可见对象
     * @param pickSelectableOnly 仅选择可选择对象
     * @return 区域内所有 ViewProvider 的列表
     */
    std::vector<Gui::ViewProvider*> pickRegion(int x1, int y1, int x2, int y2,
                                                bool pickVisibleOnly = true,
                                                bool pickSelectableOnly = true);

    /**
     * @brief 区域拾取 - 返回详细的拾取结果
     *
     * @param x1 矩形左上角X坐标 (Qt坐标系)
     * @param y1 矩形左上角Y坐标 (Qt坐标系)
     * @param x2 矩形右下角X坐标 (Qt坐标系)
     * @param y2 矩形右下角Y坐标 (Qt坐标系)
     * @return 详细的拾取结果列表
     */
    std::vector<Gui::View3D::PickResult> pickRegionDetailed(int x1, int y1, int x2, int y2);

    /**
     * @brief 多目标拾取 - 返回射线穿过的所有对象
     *
     * 与 pick() 不同，此方法返回射线路径上的所有交点，
     * 而不仅仅是最近的一个。结果按距离排序（最近的在前）。
     *
     * @param pos 屏幕坐标 (Qt坐标系)
     * @param maxHits 最大返回数量 (0 = 无限制)
     * @param pickVisibleOnly 仅拾取可见对象
     * @param pickSelectableOnly 仅拾取可选择对象
     * @return 所有命中的 PickResult 列表，按距离排序
     */
    std::vector<Gui::View3D::PickResult> pickAll(const QPoint& pos,
                                                  int maxHits = 0,
                                                  bool pickVisibleOnly = true,
                                                  bool pickSelectableOnly = true);

    /**
     * @brief 多目标拾取 - 仅返回 ViewProvider 列表
     *
     * 简化版本，仅返回 ViewProvider 指针列表
     *
     * @param pos 屏幕坐标 (Qt坐标系)
     * @param maxHits 最大返回数量 (0 = 无限制)
     * @return 所有命中的 ViewProvider 列表，按距离排序
     */
    std::vector<Gui::ViewProvider*> pickAllViewProviders(const QPoint& pos,
                                                          int maxHits = 0);

    //-----------------------------------------------------------------------
    // ViewProvider 管理
    //-----------------------------------------------------------------------
    void addViewProvider(Gui::ViewProvider* vp) override;
    void removeViewProvider(Gui::ViewProvider* vp) override;
    void updateViewProvider(Gui::ViewProvider* vp) override;
    bool hasViewProvider(Gui::ViewProvider* vp) const override;
    std::vector<Gui::ViewProvider*> getViewProviders() const override;
    
    //-----------------------------------------------------------------------
    // 渲染设置
    //-----------------------------------------------------------------------
    void setRenderMode(Gui::View3D::RenderMode mode) override;
    Gui::View3D::RenderMode getRenderMode() const override;
    void setBackgroundColor(const Base::Color& color) override;
    Base::Color getBackgroundColor() const override;
    void setBackgroundGradient(const Gui::View3D::BackgroundGradient& gradient) override;
    Gui::View3D::BackgroundGradient getBackgroundGradient() const override;
    void setBacklightEnabled(bool enabled) override;
    bool isBacklightEnabled() const override;
    void setAmbientIntensity(float intensity) override;
    float getAmbientIntensity() const override;

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

    //-----------------------------------------------------------------------
    // NaviCube (Navigation Cube)
    //-----------------------------------------------------------------------

    /**
     * @brief Enable or disable the NaviCube
     */
    void setNaviCubeEnabled(bool enabled);

    /**
     * @brief Check if NaviCube is enabled
     */
    bool isNaviCubeEnabled() const;

    /**
     * @brief Set the NaviCube corner position
     * @param corner 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight
     */
    void setNaviCubeCorner(int corner);

    /**
     * @brief Get the current NaviCube corner
     */
    int getNaviCubeCorner() const;

    /**
     * @brief Get the NaviCube object for advanced configuration
     */
    OsgVerseNaviCube* getNaviCube();

    //-----------------------------------------------------------------------
    // 阴影渲染 (Shadow Rendering)
    //-----------------------------------------------------------------------

    /**
     * @brief Shadow quality level
     */
    enum class ShadowQuality {
        Low,        ///< 512x512 shadow map
        Medium,     ///< 1024x1024 shadow map
        High,       ///< 2048x2048 shadow map
        Ultra       ///< 4096x4096 shadow map
    };

    /**
     * @brief Enable or disable shadow rendering
     */
    void setShadowEnabled(bool enabled);

    /**
     * @brief Check if shadow rendering is enabled
     */
    bool isShadowEnabled() const;

    /**
     * @brief Set shadow quality level
     */
    void setShadowQuality(ShadowQuality quality);

    /**
     * @brief Get current shadow quality level
     */
    ShadowQuality getShadowQuality() const;

    /**
     * @brief Enable soft shadows (more realistic but slower)
     */
    void setSoftShadowEnabled(bool enabled);

    /**
     * @brief Check if soft shadows are enabled
     */
    bool isSoftShadowEnabled() const;

    //-----------------------------------------------------------------------
    // 后处理效果 (Post-Processing Effects)
    //-----------------------------------------------------------------------

    /**
     * @brief Enable or disable SSAO (Screen Space Ambient Occlusion)
     */
    void setSSAOEnabled(bool enabled);

    /**
     * @brief Check if SSAO is enabled
     */
    bool isSSAOEnabled() const;

    /**
     * @brief Set SSAO radius (sampling radius)
     */
    void setSSAORadius(float radius);

    /**
     * @brief Get SSAO radius
     */
    float getSSAORadius() const;

    /**
     * @brief Set SSAO intensity
     */
    void setSSAOIntensity(float intensity);

    /**
     * @brief Get SSAO intensity
     */
    float getSSAOIntensity() const;

    /**
     * @brief Enable or disable Bloom effect
     */
    void setBloomEnabled(bool enabled);

    /**
     * @brief Check if Bloom is enabled
     */
    bool isBloomEnabled() const;

    /**
     * @brief Set Bloom threshold (brightness threshold for bloom)
     */
    void setBloomThreshold(float threshold);

    /**
     * @brief Get Bloom threshold
     */
    float getBloomThreshold() const;

    /**
     * @brief Set Bloom intensity
     */
    void setBloomIntensity(float intensity);

    /**
     * @brief Get Bloom intensity
     */
    float getBloomIntensity() const;

private:
    /**
     * @brief Create a scene node for a ViewProvider
     */
    osg::ref_ptr<osg::Node> createNodeForViewProvider(Gui::ViewProvider* vp);

    /**
     * @brief Apply material to a node
     */
    void applyMaterial(osg::Node* node, const Base::Color& color);

    /**
     * @brief Apply material with transparency to a node
     *
     * @param node The node to apply material to
     * @param color The material color
     * @param transparency Transparency value (0.0 = fully transparent, 1.0 = fully opaque)
     */
    void applyMaterialWithTransparency(osg::Node* node, const Base::Color& color, float transparency);

    /**
     * @brief Get shadow map size for quality level
     */
    int getShadowMapSize(ShadowQuality quality) const;

    /**
     * @brief Find ViewProvider from OSG node path
     *
     * Traverses the node path from leaf to root looking for ViewProviderUserData
     */
    Gui::ViewProvider* findViewProviderFromNodePath(const osg::NodePath& nodePath);

    /**
     * @brief Determine pick type from OSG intersection
     *
     * Analyzes the intersection to determine if it hit a Face, Edge, or Vertex
     * based on the geometry's primitive type and intersection details.
     *
     * @param intersection The OSG intersection result
     * @param result The PickResult to populate with type information
     */
    void determinePickType(const osgUtil::LineSegmentIntersector::Intersection& intersection,
                           Gui::View3D::PickResult& result);

    /**
     * @brief Generate sub-element name from pick type and index
     *
     * @param pickType The type of element picked (Face, Edge, Vertex)
     * @param index The index of the element
     * @return A string like "Face1", "Edge5", "Vertex3"
     */
    std::string generateSubElementName(Gui::View3D::PickType pickType, int index);

    /**
     * @brief Create the HUD camera for selection overlays
     */
    void createSelectionHUD();

    /**
     * @brief Update the selection rectangle visualization
     */
    void updateSelectionRectangle(int x1, int y1, int x2, int y2);

    /**
     * @brief Update the selection lasso visualization
     */
    void updateSelectionLasso(const std::vector<QPoint>& points);

    /**
     * @brief Clear the selection visualization
     */
    void clearSelectionVisualization();

    /**
     * @brief Setup Hidden Line rendering mode
     *
     * Implements two-pass rendering:
     * Pass 1: Render filled polygons with background color (for depth)
     * Pass 2: Render wireframe edges on top
     */
    void setupHiddenLineMode(osg::StateSet* stateSet);

    /**
     * @brief Clear Hidden Line mode resources
     */
    void clearHiddenLineMode();

public:
    /**
     * @brief Set the selection start point (called by widget on mouse press)
     */
    void setSelectionStart(const QPoint& pos);

    /**
     * @brief Update the selection end point (called by widget on mouse move)
     */
    void updateSelectionEnd(const QPoint& pos);

    /**
     * @brief Finish the selection (called by widget on mouse release)
     * @return List of ViewProviders selected in the region
     */
    std::vector<Gui::ViewProvider*> finishSelection();

private:
    OsgVerseWidget* _widget;                              ///< Qt OpenGL widget
    osg::ref_ptr<osg::Group> _sceneRoot;                  ///< Scene root node
    osg::ref_ptr<osg::LightSource> _backlightSource;      ///< Backlight source
    std::map<Gui::ViewProvider*, osg::ref_ptr<osg::Node>> _vpNodes; ///< ViewProvider to node mapping

    // State
    std::string _navigationStyle;                         ///< Current navigation style
    Gui::View3D::SelectionMode _selectionMode;            ///< Current selection mode
    Gui::View3D::RenderMode _renderMode;                  ///< Current render mode
    Base::Color _backgroundColor;                         ///< Background color
    bool _backlightEnabled;                               ///< Backlight enabled
    float _ambientIntensity{0.2f};                        ///< Ambient light intensity (0.0-1.0)
    bool _viewing;                                        ///< Viewing mode
    bool _fpsEnabled;                                     ///< FPS display enabled
    bool _orthographic;                                   ///< Orthographic camera
    Gui::ViewProvider* _editingVP;                        ///< Currently editing ViewProvider
    int _editingMode;                                     ///< Editing mode

    // Shadow rendering
    osg::ref_ptr<osgShadow::ShadowedScene> _shadowedScene;  ///< Shadowed scene container
    bool _shadowEnabled;                                  ///< Shadow enabled flag
    ShadowQuality _shadowQuality;                         ///< Shadow quality level
    bool _softShadowEnabled;                              ///< Soft shadow flag

    // Selection visualization
    osg::ref_ptr<osg::Camera> _hudCamera;                 ///< HUD camera for selection overlays
    osg::ref_ptr<osg::Geode> _selectionGeode;             ///< Geode for selection geometry
    osg::ref_ptr<osg::Geometry> _selectionGeometry;       ///< Selection shape geometry
    QPoint _selectionStart;                               ///< Selection start point
    QPoint _selectionCurrent;                             ///< Current selection point
    std::vector<QPoint> _lassoPoints;                     ///< Lasso selection points

    // FPS display
    osg::ref_ptr<osg::Camera> _fpsCamera;                 ///< HUD camera for FPS display
    osg::ref_ptr<osg::Geode> _fpsGeode;                   ///< Geode for FPS text
    osg::ref_ptr<osgText::Text> _fpsText;                 ///< FPS text node
    osg::ref_ptr<osgText::Text> _statsText;               ///< Detailed stats text node

    // Hidden Line mode
    osg::ref_ptr<osg::Group> _hiddenLinePass1;            ///< First pass node for hidden line mode
    bool _hiddenLineModeActive{false};                    ///< Whether hidden line mode is active

    // Post-processing
    std::unique_ptr<PostProcessManager> _postProcessManager;  ///< Post-processing effects manager

    // NaviCube
    std::unique_ptr<OsgVerseNaviCube> _naviCube;              ///< Navigation cube
    bool _naviCubeEnabled{true};                              ///< NaviCube enabled flag

    // Background
    std::unique_ptr<OsgVerseBackground> _background;          ///< Background renderer
    Gui::View3D::BackgroundGradient _backgroundGradient;      ///< Background gradient settings

    /**
     * @brief Create the FPS display HUD
     */
    void createFPSDisplay();

    /**
     * @brief Update the FPS display text
     */
    void updateFPSDisplay();

    /**
     * @brief Show or hide FPS display
     */
    void showFPSDisplay(bool visible);
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_VIEWER_H
