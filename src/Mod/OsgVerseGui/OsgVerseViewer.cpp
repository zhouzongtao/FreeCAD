// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseViewer.h"
#include "OsgVerseWidget.h"
#include "GeometryConverter.h"
#include <Base/Console.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <App/DocumentObject.h>
#include <App/Property.h>
#include <App/PropertyStandard.h>
#include <Gui/Render/Core/RenderTypes.h>

// Part module - we can include this because OsgVerseGui links Part!
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

// OSG includes
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osg/Camera>
#include <osg/Light>
#include <osg/LightSource>

// OSG picking includes
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>

// OSG rendering state includes
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/LineWidth>
#include <osg/BlendFunc>
#include <osg/Texture2D>
#include <osg/TexEnv>

// OSG stats includes
#include <osg/Stats>

// OSG shadow includes
#include <osgShadow/ShadowedScene>
#include <osgShadow/ShadowMap>
#include <osgShadow/SoftShadowMap>

using namespace OsgVerseGui;

OsgVerseViewer::OsgVerseViewer(QWidget* parent)
    : _widget(nullptr)
    , _sceneRoot(nullptr)
    , _navigationStyle("Trackball")
    , _selectionMode(Gui::View3D::SelectionMode::None)
    , _renderMode(Gui::View3D::RenderMode::Shaded)
    , _backgroundColor(0.2f, 0.2f, 0.3f)
    , _backlightEnabled(false)
    , _viewing(true)
    , _fpsEnabled(false)
    , _orthographic(false)
    , _editingVP(nullptr)
    , _editingMode(0)
    , _shadowEnabled(false)
    , _shadowQuality(ShadowQuality::Medium)
    , _softShadowEnabled(false)
{
    try {
        // Create Qt OpenGL widget
        _widget = new OsgVerseWidget(parent);

        // Set back-reference for selection support
        _widget->setViewer(this);

        // Get OSG viewer from widget
        osgViewer::Viewer* viewer = _widget->getViewer();
        
        if (!viewer) {
            Base::Console().warning("OsgVerseViewer: Failed to get viewer from widget\n");
        }
        
        // Create scene root
        _sceneRoot = new osg::Group();
        
        // Set scene data
        if (viewer) {
            viewer->setSceneData(_sceneRoot.get());
        }
        
        // Set initial background color
        setBackgroundColor(_backgroundColor);
        
        // Add light source
        osg::ref_ptr<osg::Light> light = new osg::Light();
        light->setLightNum(0);
        light->setPosition(osg::Vec4(10.0f, 10.0f, 10.0f, 1.0f));  // Positional light
        light->setAmbient(osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f));
        light->setDiffuse(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
        light->setSpecular(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        
        osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource();
        lightSource->setLight(light.get());
        _sceneRoot->addChild(lightSource.get());
        
        // Enable lighting
        _sceneRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        
        // Set default camera position
        if (viewer) {
            viewer->getCameraManipulator()->setHomePosition(
                osg::Vec3d(0, -100, 50),  // Eye - further back for typical CAD models
                osg::Vec3d(0, 0, 0),      // Center
                osg::Vec3d(0, 0, 1)       // Up
            );
            viewer->home();
        }
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseViewer: Exception in constructor: %s\n", e.what());
        throw;
    }
    catch (...) {
        Base::Console().error("OsgVerseViewer: Unknown exception in constructor\n");
        throw;
    }
}

OsgVerseViewer::~OsgVerseViewer()
{
    // Clean up view provider nodes
    _vpNodes.clear();
    
    // Clean up scene
    _sceneRoot = nullptr;
    
    // Delete widget (which will delete the viewer)
    if (_widget) {
        delete _widget;
        _widget = nullptr;
    }
}

//===========================================================================
// 基础渲染接口
//===========================================================================

void OsgVerseViewer::render()
{
    if (_widget) {
        _widget->update(); // Triggers paintGL()
    }
}

void OsgVerseViewer::resize(int width, int height)
{
    if (_widget) {
        _widget->resize(width, height);
    }
}

QWidget* OsgVerseViewer::getWidget()
{
    return _widget;
}

QOpenGLWidget* OsgVerseViewer::getGLWidget()
{
    return _widget;
}

//===========================================================================
// 场景管理
//===========================================================================

void OsgVerseViewer::setSceneGraph(void* root)
{
    if (!_widget) return;
    
    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) return;
    
    // Cast to OSG node
    osg::Node* node = static_cast<osg::Node*>(root);
    viewer->setSceneData(node);
    
    render();
}

void* OsgVerseViewer::getSceneGraph()
{
    return _sceneRoot.get();
}

void OsgVerseViewer::updateScene()
{
    render();
}

//===========================================================================
// 相机控制
//===========================================================================

void OsgVerseViewer::setCamera(const Gui::View3D::CameraParams& params)
{
    if (!_widget) return;
    
    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return;
    
    // Set projection
    double aspectRatio = params.aspectRatio;
    if (aspectRatio <= 0.0) {
        int width = _widget->width();
        int height = _widget->height();
        aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    }
    
    if (params.orthographic) {
        double halfHeight = params.height / 2.0;
        double halfWidth = halfHeight * aspectRatio;
        camera->setProjectionMatrixAsOrtho(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            params.nearPlane, params.farPlane
        );
    }
    else {
        camera->setProjectionMatrixAsPerspective(
            params.fieldOfView,
            aspectRatio,
            params.nearPlane,
            params.farPlane
        );
    }
    
    // Set view matrix
    osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
    osg::Vec3d center(params.target.x, params.target.y, params.target.z);
    osg::Vec3d up(params.upVector.x, params.upVector.y, params.upVector.z);
    
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    _orthographic = params.orthographic;
    
    render();
}

Gui::View3D::CameraParams OsgVerseViewer::getCamera() const
{
    Gui::View3D::CameraParams params;
    
    if (!_widget) return params;
    
    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) return params;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return params;
    
    // Get view matrix
    osg::Matrixd viewMatrix = camera->getViewMatrix();
    osg::Vec3d eye, center, up;
    viewMatrix.getLookAt(eye, center, up);
    
    params.position = Base::Vector3d(eye.x(), eye.y(), eye.z());
    params.target = Base::Vector3d(center.x(), center.y(), center.z());
    params.upVector = Base::Vector3d(up.x(), up.y(), up.z());
    
    // Get projection matrix
    osg::Matrixd projMatrix = camera->getProjectionMatrix();
    double left, right, bottom, top, nearPlane, farPlane;
    
    if (_orthographic) {
        projMatrix.getOrtho(left, right, bottom, top, nearPlane, farPlane);
        params.orthographic = true;
        params.height = top - bottom;
    }
    else {
        double fovy, aspectRatio;
        projMatrix.getPerspective(fovy, aspectRatio, nearPlane, farPlane);
        params.orthographic = false;
        params.fieldOfView = fovy;
        params.aspectRatio = aspectRatio;
    }
    
    params.nearPlane = nearPlane;
    params.farPlane = farPlane;
    
    return params;
}

void OsgVerseViewer::viewAll()
{
    if (_widget) {
        osgViewer::Viewer* viewer = _widget->getViewer();
        if (viewer) {
            viewer->home();
            render();
        }
    }
}

void OsgVerseViewer::resetCamera()
{
    viewAll();
}

void OsgVerseViewer::setCameraType(bool orthographic)
{
    _orthographic = orthographic;
    
    // Update projection
    Gui::View3D::CameraParams params = getCamera();
    params.orthographic = orthographic;
    setCamera(params);
}

bool OsgVerseViewer::isCameraOrthographic() const
{
    return _orthographic;
}

//===========================================================================
// 事件处理
//===========================================================================

bool OsgVerseViewer::handleMouseEvent(QMouseEvent* event)
{
    // Events are handled by OsgVerseWidget
    // This method is for additional processing if needed
    return false;
}

bool OsgVerseViewer::handleKeyEvent(QKeyEvent* event)
{
    // Events are handled by OsgVerseWidget
    // This method is for additional processing if needed
    return false;
}

bool OsgVerseViewer::handleWheelEvent(QWheelEvent* event)
{
    // Events are handled by OsgVerseWidget
    // This method is for additional processing if needed
    return false;
}

//===========================================================================
// 拾取和选择
//===========================================================================

Gui::View3D::PickResult OsgVerseViewer::pick(const QPoint& pos)
{
    Gui::View3D::PickResult result;
    result.valid = false;
    result.viewProvider = nullptr;
    result.distance = 0.0;

    // Get viewer
    if (!_widget) {
        return result;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        return result;
    }

    osg::Camera* camera = viewer->getCamera();
    if (!camera) {
        return result;
    }

    // Coordinate conversion: Qt (top-left origin, Y down) -> OSG (bottom-left origin, Y up)
    int windowHeight = _widget->height();
    float x = static_cast<float>(pos.x());
    float y = static_cast<float>(windowHeight - pos.y());

    // Create line segment intersector
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(
            osgUtil::Intersector::WINDOW,
            x, y
        );

    // Create intersection visitor
    osgUtil::IntersectionVisitor iv(intersector.get());
    iv.setTraversalMask(~0);  // Traverse all nodes

    // Execute intersection test from camera
    camera->accept(iv);

    // Check results
    if (!intersector->containsIntersections()) {
        return result;
    }

    // Get first (nearest) intersection
    const osgUtil::LineSegmentIntersector::Intersection& intersection =
        intersector->getFirstIntersection();

    result.valid = true;

    // Extract intersection point in world coordinates
    osg::Vec3d worldPoint = intersection.getWorldIntersectPoint();
    result.point = Base::Vector3d(worldPoint.x(), worldPoint.y(), worldPoint.z());

    // Extract surface normal
    osg::Vec3d worldNormal = intersection.getWorldIntersectNormal();
    result.normal = Base::Vector3d(worldNormal.x(), worldNormal.y(), worldNormal.z());

    // Calculate distance from camera eye to intersection point
    osg::Vec3d eye, center, up;
    camera->getViewMatrixAsLookAt(eye, center, up);
    result.distance = (worldPoint - eye).length();

    // Find ViewProvider from node path
    result.viewProvider = findViewProviderFromNodePath(intersection.nodePath);

    return result;
}

Gui::ViewProvider* OsgVerseViewer::findViewProviderFromNodePath(const osg::NodePath& nodePath)
{
    // Traverse from leaf node to root looking for ViewProviderUserData
    for (auto it = nodePath.rbegin(); it != nodePath.rend(); ++it) {
        osg::Referenced* userData = (*it)->getUserData();
        if (userData) {
            ViewProviderUserData* vpData = dynamic_cast<ViewProviderUserData*>(userData);
            if (vpData && vpData->viewProvider) {
                return vpData->viewProvider;
            }
        }
    }
    return nullptr;
}

void OsgVerseViewer::setSelectionMode(Gui::View3D::SelectionMode mode)
{
    _selectionMode = mode;
}

Gui::View3D::SelectionMode OsgVerseViewer::getSelectionMode() const
{
    return _selectionMode;
}

void OsgVerseViewer::startSelection(Gui::View3D::SelectionMode mode)
{
    _selectionMode = mode;
}

void OsgVerseViewer::stopSelection()
{
    clearSelectionVisualization();
    _selectionMode = Gui::View3D::SelectionMode::None;
}

void OsgVerseViewer::abortSelection()
{
    clearSelectionVisualization();
    _selectionMode = Gui::View3D::SelectionMode::None;
}

bool OsgVerseViewer::isSelecting() const
{
    return _selectionMode != Gui::View3D::SelectionMode::None;
}

//===========================================================================
// 选择可视化
//===========================================================================

void OsgVerseViewer::createSelectionHUD()
{
    if (_hudCamera.valid()) {
        return;  // Already created
    }

    if (!_widget) {
        return;
    }

    // Create HUD camera
    _hudCamera = new osg::Camera;
    _hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, _widget->width(), 0, _widget->height()));
    _hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _hudCamera->setViewMatrix(osg::Matrix::identity());
    _hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    _hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
    _hudCamera->setAllowEventFocus(false);

    // Create geode for selection geometry
    _selectionGeode = new osg::Geode;

    // Create selection geometry
    _selectionGeometry = new osg::Geometry;
    _selectionGeometry->setUseDisplayList(false);
    _selectionGeometry->setUseVertexBufferObjects(true);

    // Create vertices array
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    _selectionGeometry->setVertexArray(vertices);

    // Create color array (semi-transparent blue)
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(0.2f, 0.4f, 0.8f, 0.3f));  // Fill color
    _selectionGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);

    _selectionGeode->addDrawable(_selectionGeometry);

    // Set up state for transparency
    osg::StateSet* stateSet = _selectionGeode->getOrCreateStateSet();
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    _hudCamera->addChild(_selectionGeode);
    _sceneRoot->addChild(_hudCamera);
}

void OsgVerseViewer::updateSelectionRectangle(int x1, int y1, int x2, int y2)
{
    if (!_selectionGeometry.valid()) {
        createSelectionHUD();
    }

    if (!_selectionGeometry.valid() || !_widget) {
        return;
    }

    // Update HUD projection for current widget size
    int height = _widget->height();
    _hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, _widget->width(), 0, height));

    // Convert Qt coordinates (top-left origin) to OSG coordinates (bottom-left origin)
    float osgY1 = height - y1;
    float osgY2 = height - y2;

    // Create rectangle vertices
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(x1, osgY1, 0));  // Top-left
    vertices->push_back(osg::Vec3(x2, osgY1, 0));  // Top-right
    vertices->push_back(osg::Vec3(x2, osgY2, 0));  // Bottom-right
    vertices->push_back(osg::Vec3(x1, osgY2, 0));  // Bottom-left

    _selectionGeometry->setVertexArray(vertices);

    // Set up draw arrays for filled quad
    _selectionGeometry->removePrimitiveSet(0, _selectionGeometry->getNumPrimitiveSets());
    _selectionGeometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

    // Also draw outline
    osg::ref_ptr<osg::Vec3Array> outlineVertices = new osg::Vec3Array;
    outlineVertices->push_back(osg::Vec3(x1, osgY1, 0));
    outlineVertices->push_back(osg::Vec3(x2, osgY1, 0));
    outlineVertices->push_back(osg::Vec3(x2, osgY2, 0));
    outlineVertices->push_back(osg::Vec3(x1, osgY2, 0));
    _selectionGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_LOOP, 0, 4));

    _selectionGeometry->dirtyBound();

    render();
}

void OsgVerseViewer::updateSelectionLasso(const std::vector<QPoint>& points)
{
    if (!_selectionGeometry.valid()) {
        createSelectionHUD();
    }

    if (!_selectionGeometry.valid() || !_widget || points.size() < 2) {
        return;
    }

    // Update HUD projection for current widget size
    int height = _widget->height();
    _hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, _widget->width(), 0, height));

    // Create lasso vertices
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    for (const auto& pt : points) {
        float osgY = height - pt.y();
        vertices->push_back(osg::Vec3(pt.x(), osgY, 0));
    }

    _selectionGeometry->setVertexArray(vertices);

    // Draw as line strip
    _selectionGeometry->removePrimitiveSet(0, _selectionGeometry->getNumPrimitiveSets());
    _selectionGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, points.size()));

    _selectionGeometry->dirtyBound();

    render();
}

void OsgVerseViewer::clearSelectionVisualization()
{
    if (_selectionGeometry.valid()) {
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        _selectionGeometry->setVertexArray(vertices);
        _selectionGeometry->removePrimitiveSet(0, _selectionGeometry->getNumPrimitiveSets());
        _selectionGeometry->dirtyBound();
    }

    _lassoPoints.clear();

    render();
}

void OsgVerseViewer::setSelectionStart(const QPoint& pos)
{
    _selectionStart = pos;
    _selectionCurrent = pos;
    _lassoPoints.clear();
    _lassoPoints.push_back(pos);
}

void OsgVerseViewer::updateSelectionEnd(const QPoint& pos)
{
    _selectionCurrent = pos;

    if (_selectionMode == Gui::View3D::SelectionMode::Rectangle ||
        _selectionMode == Gui::View3D::SelectionMode::Rubberband) {
        updateSelectionRectangle(_selectionStart.x(), _selectionStart.y(),
                                  _selectionCurrent.x(), _selectionCurrent.y());
    }
    else if (_selectionMode == Gui::View3D::SelectionMode::Lasso) {
        _lassoPoints.push_back(pos);
        updateSelectionLasso(_lassoPoints);
    }
}

void OsgVerseViewer::finishSelection()
{
    clearSelectionVisualization();
    _selectionMode = Gui::View3D::SelectionMode::None;
}

//===========================================================================
// ViewProvider 管理
//===========================================================================

void OsgVerseViewer::addViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }

    // Create scene node for this ViewProvider
    osg::ref_ptr<osg::Node> node = createNodeForViewProvider(vp);

    if (!node) {
        // Shape may not be computed yet, use placeholder
        node = createPlaceholderSphere();
    }

    // Set UserData for picking - allows finding ViewProvider from picked node
    node->setUserData(new ViewProviderUserData(vp));

    // Store and add to scene
    _vpNodes[vp] = node;
    _sceneRoot->addChild(node.get());

    render();
}

void OsgVerseViewer::removeViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }

    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        _sceneRoot->removeChild(it->second.get());
        _vpNodes.erase(it);
        render();
    }
}

void OsgVerseViewer::updateViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }

    auto it = _vpNodes.find(vp);
    if (it == _vpNodes.end()) {
        // ViewProvider not in scene, try to add it now
        addViewProvider(vp);
        return;
    }

    // Remove old node
    osg::ref_ptr<osg::Node> oldNode = it->second;
    _sceneRoot->removeChild(oldNode.get());

    // Create new node with updated geometry
    osg::ref_ptr<osg::Node> newNode = createNodeForViewProvider(vp);

    if (!newNode) {
        newNode = createPlaceholderSphere();
    }

    // Set UserData for picking
    newNode->setUserData(new ViewProviderUserData(vp));

    // Update mapping and scene
    _vpNodes[vp] = newNode;
    _sceneRoot->addChild(newNode.get());

    render();
}

bool OsgVerseViewer::hasViewProvider(Gui::ViewProvider* vp) const
{
    return _vpNodes.find(vp) != _vpNodes.end();
}

std::vector<Gui::ViewProvider*> OsgVerseViewer::getViewProviders() const
{
    std::vector<Gui::ViewProvider*> result;
    result.reserve(_vpNodes.size());
    
    for (const auto& pair : _vpNodes) {
        result.push_back(pair.first);
    }
    
    return result;
}

//===========================================================================
// 渲染设置
//===========================================================================

void OsgVerseViewer::setRenderMode(Gui::View3D::RenderMode mode)
{
    _renderMode = mode;

    if (!_sceneRoot) {
        return;
    }

    osg::StateSet* stateSet = _sceneRoot->getOrCreateStateSet();

    // Remove existing polygon mode
    stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    stateSet->removeAttribute(osg::StateAttribute::POLYGONOFFSET);

    osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode;

    switch (mode) {
        case Gui::View3D::RenderMode::Wireframe:
            // Wireframe mode - render only edges
            pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            stateSet->setAttributeAndModes(pm.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
            break;

        case Gui::View3D::RenderMode::Points:
            // Points mode - render only vertices
            pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::POINT);
            stateSet->setAttributeAndModes(pm.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
            break;

        case Gui::View3D::RenderMode::FlatLines: {
            // FlatLines - shaded surfaces with wireframe overlay
            // First render filled polygons
            pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
            stateSet->setAttributeAndModes(pm.get(), osg::StateAttribute::ON);
            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);

            // Add polygon offset to prevent z-fighting with wireframe
            osg::ref_ptr<osg::PolygonOffset> po = new osg::PolygonOffset;
            po->setFactor(1.0f);
            po->setUnits(1.0f);
            stateSet->setAttributeAndModes(po.get(), osg::StateAttribute::ON);
            break;
        }

        case Gui::View3D::RenderMode::NoShading:
            // No shading - filled but without lighting
            pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
            stateSet->setAttributeAndModes(pm.get(), osg::StateAttribute::ON);
            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
            break;

        case Gui::View3D::RenderMode::HiddenLine:
            // Hidden line - wireframe with hidden line removal (similar to wireframe but with depth test)
            pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            stateSet->setAttributeAndModes(pm.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
            break;

        case Gui::View3D::RenderMode::Shaded:
        case Gui::View3D::RenderMode::AsIs:
        default:
            // Shaded mode - normal filled polygons with lighting
            pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
            stateSet->setAttributeAndModes(pm.get(), osg::StateAttribute::ON);
            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
            break;
    }

    render();
}

Gui::View3D::RenderMode OsgVerseViewer::getRenderMode() const
{
    return _renderMode;
}

void OsgVerseViewer::setBackgroundColor(const Base::Color& color)
{
    _backgroundColor = color;
    
    if (_widget) {
        osgViewer::Viewer* viewer = _widget->getViewer();
        if (viewer) {
            osg::Vec4 bgColor(color.r, color.g, color.b, 1.0f);
            viewer->getCamera()->setClearColor(bgColor);
            render();
        }
    }
}

Base::Color OsgVerseViewer::getBackgroundColor() const
{
    return _backgroundColor;
}

void OsgVerseViewer::setBacklightEnabled(bool enabled)
{
    _backlightEnabled = enabled;

    if (!_sceneRoot) {
        return;
    }

    if (enabled) {
        // Create backlight if it doesn't exist
        if (!_backlightSource) {
            osg::ref_ptr<osg::Light> backlight = new osg::Light();
            backlight->setLightNum(1);  // Use light 1 (light 0 is the main light)
            // Position behind the camera (negative Z in eye space)
            backlight->setPosition(osg::Vec4(0.0f, 0.0f, -1.0f, 0.0f));  // Directional light
            backlight->setAmbient(osg::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
            backlight->setDiffuse(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));    // Dimmer than main light
            backlight->setSpecular(osg::Vec4(0.1f, 0.1f, 0.1f, 1.0f));

            _backlightSource = new osg::LightSource();
            _backlightSource->setLight(backlight.get());
            _backlightSource->setLocalStateSetModes(osg::StateAttribute::ON);

            _sceneRoot->addChild(_backlightSource.get());
        }

        // Enable the backlight
        _backlightSource->setNodeMask(~0);  // Visible
    }
    else {
        // Disable the backlight (but keep the node)
        if (_backlightSource) {
            _backlightSource->setNodeMask(0);  // Hidden
        }
    }

    render();
}

bool OsgVerseViewer::isBacklightEnabled() const
{
    return _backlightEnabled;
}

//===========================================================================
// 导航和交互
//===========================================================================

void OsgVerseViewer::setNavigationStyle(const std::string& style)
{
    _navigationStyle = style;

    if (!_widget) {
        return;
    }

    // Map FreeCAD navigation style names to OsgVerseWidget navigation styles
    OsgVerseWidget::NavigationStyle navStyle = OsgVerseWidget::NavigationStyle::Trackball;

    if (style == "Gui::CADNavigationStyle" || style == "CAD" || style == "CADNavigationStyle") {
        navStyle = OsgVerseWidget::NavigationStyle::CAD;
    }
    else if (style == "Gui::BlenderNavigationStyle" || style == "Blender" || style == "BlenderNavigationStyle") {
        // Blender uses trackball-like navigation
        navStyle = OsgVerseWidget::NavigationStyle::Trackball;
    }
    else if (style == "Gui::TouchpadNavigationStyle" || style == "Touchpad" || style == "TouchpadNavigationStyle") {
        navStyle = OsgVerseWidget::NavigationStyle::Touchpad;
    }
    else if (style == "Gui::GestureNavigationStyle" || style == "Gesture" || style == "GestureNavigationStyle") {
        navStyle = OsgVerseWidget::NavigationStyle::Touchpad;
    }
    else if (style == "Gui::OpenInventorNavigationStyle" || style == "OpenInventor" || style == "InventorNavigationStyle") {
        navStyle = OsgVerseWidget::NavigationStyle::Trackball;
    }
    else if (style == "Gui::MayaGestureNavigationStyle" || style == "Maya" || style == "MayaNavigationStyle") {
        navStyle = OsgVerseWidget::NavigationStyle::Trackball;
    }
    else if (style == "Gui::OpenCascadeNavigationStyle" || style == "OpenCascade") {
        navStyle = OsgVerseWidget::NavigationStyle::CAD;
    }
    else if (style == "Gui::OpenSCADNavigationStyle" || style == "OpenSCAD") {
        navStyle = OsgVerseWidget::NavigationStyle::CAD;
    }
    else if (style == "Gui::RevitNavigationStyle" || style == "Revit") {
        navStyle = OsgVerseWidget::NavigationStyle::CAD;
    }
    else if (style == "Gui::TinkerCADNavigationStyle" || style == "TinkerCAD") {
        navStyle = OsgVerseWidget::NavigationStyle::Trackball;
    }
    // Default to Trackball for unknown styles

    _widget->setNavigationStyle(navStyle);
}

std::string OsgVerseViewer::getNavigationStyle() const
{
    return _navigationStyle;
}

void OsgVerseViewer::setViewing(bool enable)
{
    _viewing = enable;
}

bool OsgVerseViewer::isViewing() const
{
    return _viewing;
}

//===========================================================================
// 后端信息
//===========================================================================

Gui::Render::BackendType OsgVerseViewer::getBackendType() const
{
    return Gui::Render::BackendType::OsgVerse;
}

std::string OsgVerseViewer::getBackendName() const
{
    return "OsgVerse";
}

std::string OsgVerseViewer::getBackendVersion() const
{
    return "OsgVerse + OSG 3.6+";
}

//===========================================================================
// 统计和调试
//===========================================================================

Gui::Render::RenderStats OsgVerseViewer::getStats() const
{
    Gui::Render::RenderStats stats;
    stats.fps = 0.0;
    stats.frameTime = 0.0;
    stats.triangleCount = 0;
    stats.vertexCount = 0;
    stats.drawCalls = 0;
    stats.frameCount = 0;

    if (!_widget) {
        return stats;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        return stats;
    }

    // Get frame number from frame stamp
    const osg::FrameStamp* frameStamp = viewer->getFrameStamp();
    if (frameStamp) {
        stats.frameCount = static_cast<int>(frameStamp->getFrameNumber());
    }

    // Get frame statistics from viewer stats
    osg::Stats* viewerStats = viewer->getViewerStats();
    if (viewerStats && frameStamp) {
        unsigned int frameNumber = frameStamp->getFrameNumber();
        double refTime = 0.0, prevRefTime = 0.0;

        // Try to get frame timing from reference times
        if (frameNumber > 0 &&
            viewerStats->getAttribute(frameNumber, "Reference time", refTime) &&
            viewerStats->getAttribute(frameNumber - 1, "Reference time", prevRefTime)) {
            double frameTime = refTime - prevRefTime;
            if (frameTime > 0.0) {
                stats.frameTime = frameTime * 1000.0;  // Convert to milliseconds
                stats.fps = 1.0 / frameTime;
            }
        }
    }

    // Estimate geometry count from ViewProviders
    // Actual detailed geometry stats would require traversing the scene graph
    stats.drawCalls = static_cast<int>(_vpNodes.size());

    return stats;
}

void OsgVerseViewer::resetStats()
{
    // OSG stats are frame-based and managed internally
    // This method is a placeholder for any custom stat tracking reset
    // The viewer stats are automatically maintained per-frame by OSG
}

void OsgVerseViewer::setFPSEnabled(bool enabled)
{
    _fpsEnabled = enabled;

    // Note: For visual FPS display overlay, we would need to add osgViewer::StatsHandler
    // This is tracked as a future enhancement
    // For now, the _fpsEnabled flag controls whether getStats() should be called
}

bool OsgVerseViewer::isFPSEnabled() const
{
    return _fpsEnabled;
}

//===========================================================================
// 高级功能
//===========================================================================

QImage OsgVerseViewer::grabImage(int width, int height)
{
    if (!_widget) {
        return QImage();
    }
    
    if (width <= 0) width = _widget->width();
    if (height <= 0) height = _widget->height();
    
    return _widget->grabFramebuffer();
}

bool OsgVerseViewer::saveScreenshot(const QString& filename, int width, int height)
{
    QImage image = grabImage(width, height);
    if (image.isNull()) {
        return false;
    }
    
    return image.save(filename);
}

void OsgVerseViewer::setEditingViewProvider(Gui::ViewProvider* vp, int mode)
{
    _editingVP = vp;
    _editingMode = mode;
}

Gui::ViewProvider* OsgVerseViewer::getEditingViewProvider() const
{
    return _editingVP;
}

bool OsgVerseViewer::isEditingViewProvider() const
{
    return _editingVP != nullptr;
}

void OsgVerseViewer::resetEditingViewProvider()
{
    _editingVP = nullptr;
    _editingMode = 0;
}

//===========================================================================
// Private methods
//===========================================================================

osg::ref_ptr<osg::Node> OsgVerseViewer::createNodeForViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        Base::Console().message("createNodeForViewProvider: vp is null\n");
        return nullptr;
    }

    // Check if this is a ViewProviderDocumentObject
    auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);
    if (!vpDoc) {
        Base::Console().message("createNodeForViewProvider: not a ViewProviderDocumentObject\n");
        return nullptr;
    }

    App::DocumentObject* obj = vpDoc->getObject();
    if (!obj) {
        Base::Console().message("createNodeForViewProvider: getObject() returned null\n");
        return nullptr;
    }

    Base::Console().message("createNodeForViewProvider: obj type=%s name=%s\n",
                           obj->getTypeId().getName(), obj->getNameInDocument());

    // Check if this is a Part::Feature
    if (!obj->isDerivedFrom(Part::Feature::getClassTypeId())) {
        Base::Console().message("createNodeForViewProvider: not a Part::Feature\n");
        return nullptr;
    }

    // Extract TopoDS_Shape and convert to OSG geometry
    try {
        Base::Console().message("createNodeForViewProvider: Getting TopoShape...\n");
        Part::TopoShape topoShape = Part::Feature::getTopoShape(
            obj,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );

        const TopoDS_Shape& shape = topoShape.getShape();

        if (shape.IsNull()) {
            Base::Console().message("createNodeForViewProvider: shape is null\n");
            return nullptr;
        }

        Base::Console().message("createNodeForViewProvider: Shape is valid, converting...\n");
        
        // Convert using GeometryConverter
        GeometryConverter::ConversionOptions options;
        options.deflection = 0.1;
        options.angle = 0.5;
        options.computeNormals = true;
        
        GeometryConverter::ConversionStats stats;
        osg::ref_ptr<osg::Geode> geode = GeometryConverter::convertShape(shape, options, &stats);
        
        if (!geode) {
            Base::Console().error("OsgVerseViewer: GeometryConverter failed\n");
            return nullptr;
        }

        // Extract color and transparency from ViewProvider
        Base::Color shapeColor(0.8f, 0.8f, 0.8f);  // Default gray
        float transparency = 1.0f;  // 1.0 = fully opaque

        // Try to get ShapeColor property
        App::Property* colorProp = vp->getPropertyByName("ShapeColor");
        if (colorProp && colorProp->isDerivedFrom(App::PropertyColor::getClassTypeId())) {
            App::PropertyColor* propColor = static_cast<App::PropertyColor*>(colorProp);
            unsigned long packed = propColor->getValue().getPackedValue();
            // Extract RGB from packed value (format: 0xRRGGBB)
            float r = static_cast<float>((packed >> 24) & 0xFF) / 255.0f;
            float g = static_cast<float>((packed >> 16) & 0xFF) / 255.0f;
            float b = static_cast<float>((packed >> 8) & 0xFF) / 255.0f;
            shapeColor = Base::Color(r, g, b);
        }

        // Try to get Transparency property
        App::Property* transProp = vp->getPropertyByName("Transparency");
        if (transProp && transProp->isDerivedFrom(App::PropertyPercent::getClassTypeId())) {
            App::PropertyPercent* propTrans = static_cast<App::PropertyPercent*>(transProp);
            // Transparency is typically 0-100, convert to alpha (0=opaque, 100=transparent)
            int transPercent = propTrans->getValue();
            transparency = 1.0f - (static_cast<float>(transPercent) / 100.0f);
        }
        else if (transProp && transProp->isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            App::PropertyInteger* propTrans = static_cast<App::PropertyInteger*>(transProp);
            int transPercent = propTrans->getValue();
            transparency = 1.0f - (static_cast<float>(transPercent) / 100.0f);
        }

        // Apply material with transparency
        applyMaterialWithTransparency(geode.get(), shapeColor, transparency);

        return geode;
    }
    catch (const Standard_Failure& e) {
        Base::Console().error("OsgVerseViewer: OCCT exception: %s\n", e.GetMessageString());
        return nullptr;
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseViewer: Exception: %s\n", e.what());
        return nullptr;
    }
    catch (...) {
        Base::Console().error("OsgVerseViewer: Unknown exception\n");
        return nullptr;
    }
}

osg::ref_ptr<osg::Node> OsgVerseViewer::createPlaceholderSphere()
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    
    // Create a green sphere (radius 1.0)
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0, 0, 0), 1.0f);
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere.get());
    drawable->setColor(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f));  // Bright green
    
    geode->addDrawable(drawable.get());
    
    Base::Console().log("OsgVerseViewer: Created placeholder sphere (radius=1.0, green)\n");
    
    return geode;
}

void OsgVerseViewer::applyMaterial(osg::Node* node, const Base::Color& color)
{
    applyMaterialWithTransparency(node, color, 1.0f);
}

void OsgVerseViewer::applyMaterialWithTransparency(osg::Node* node, const Base::Color& color, float transparency)
{
    if (!node) {
        return;
    }

    // Clamp transparency to valid range [0.0, 1.0]
    // 0.0 = fully transparent, 1.0 = fully opaque
    float alpha = std::max(0.0f, std::min(1.0f, transparency));

    osg::ref_ptr<osg::StateSet> stateSet = node->getOrCreateStateSet();
    osg::ref_ptr<osg::Material> material = new osg::Material();

    osg::Vec4 diffuse(color.r, color.g, color.b, alpha);
    material->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(color.r * 0.3f, color.g * 0.3f, color.b * 0.3f, alpha));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
    material->setShininess(osg::Material::FRONT_AND_BACK, 32.0f);
    material->setTransparency(osg::Material::FRONT_AND_BACK, 1.0f - alpha);

    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);

    // Enable alpha blending for transparent materials
    if (alpha < 1.0f) {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        // Use standard alpha blending
        osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc(
            osg::BlendFunc::SRC_ALPHA,
            osg::BlendFunc::ONE_MINUS_SRC_ALPHA
        );
        stateSet->setAttributeAndModes(blendFunc.get(), osg::StateAttribute::ON);

        // Enable depth writing but use depth sorting
        stateSet->setRenderBinDetails(10, "DepthSortedBin");
    }
    else {
        // Opaque materials
        stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
        stateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    }
}

//===========================================================================
// 阴影渲染 (Shadow Rendering)
//===========================================================================

void OsgVerseViewer::setShadowEnabled(bool enabled)
{
    if (_shadowEnabled == enabled) {
        return;  // No change
    }

    _shadowEnabled = enabled;

    if (!_widget) {
        return;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        return;
    }

    if (enabled) {
        // Create shadowed scene if it doesn't exist
        if (!_shadowedScene) {
            _shadowedScene = new osgShadow::ShadowedScene;

            // Set up the shadow technique based on current settings
            int shadowMapSize = getShadowMapSize(_shadowQuality);

            if (_softShadowEnabled) {
                osg::ref_ptr<osgShadow::SoftShadowMap> ssm = new osgShadow::SoftShadowMap;
                ssm->setTextureSize(osg::Vec2s(shadowMapSize, shadowMapSize));
                ssm->setSoftnessWidth(0.005f);
                _shadowedScene->setShadowTechnique(ssm.get());
            }
            else {
                osg::ref_ptr<osgShadow::ShadowMap> sm = new osgShadow::ShadowMap;
                sm->setTextureSize(osg::Vec2s(shadowMapSize, shadowMapSize));
                _shadowedScene->setShadowTechnique(sm.get());
            }

            // Set receive and cast shadow masks
            _shadowedScene->setReceivesShadowTraversalMask(0x1);
            _shadowedScene->setCastsShadowTraversalMask(0x2);
        }

        // Move scene root into shadowed scene
        if (_sceneRoot) {
            // Ensure scene root has the right mask
            _sceneRoot->setNodeMask(0x1 | 0x2);  // Both receive and cast shadows

            _shadowedScene->addChild(_sceneRoot.get());
            viewer->setSceneData(_shadowedScene.get());
        }

        Base::Console().log("OsgVerseViewer: Shadows enabled\n");
    }
    else {
        // Disable shadows - set scene root directly
        if (_sceneRoot) {
            viewer->setSceneData(_sceneRoot.get());
        }

        Base::Console().log("OsgVerseViewer: Shadows disabled\n");
    }

    render();
}

bool OsgVerseViewer::isShadowEnabled() const
{
    return _shadowEnabled;
}

void OsgVerseViewer::setShadowQuality(ShadowQuality quality)
{
    if (_shadowQuality == quality) {
        return;  // No change
    }

    _shadowQuality = quality;

    // If shadows are enabled, update the shadow map size
    if (_shadowEnabled && _shadowedScene) {
        int shadowMapSize = getShadowMapSize(quality);

        osgShadow::ShadowTechnique* technique = _shadowedScene->getShadowTechnique();
        if (technique) {
            // Try to update texture size based on technique type
            osgShadow::ShadowMap* sm = dynamic_cast<osgShadow::ShadowMap*>(technique);
            if (sm) {
                sm->setTextureSize(osg::Vec2s(shadowMapSize, shadowMapSize));
            }

            osgShadow::SoftShadowMap* ssm = dynamic_cast<osgShadow::SoftShadowMap*>(technique);
            if (ssm) {
                ssm->setTextureSize(osg::Vec2s(shadowMapSize, shadowMapSize));
            }
        }

        render();
    }
}

OsgVerseViewer::ShadowQuality OsgVerseViewer::getShadowQuality() const
{
    return _shadowQuality;
}

void OsgVerseViewer::setSoftShadowEnabled(bool enabled)
{
    if (_softShadowEnabled == enabled) {
        return;  // No change
    }

    _softShadowEnabled = enabled;

    // If shadows are enabled, recreate the shadow technique
    if (_shadowEnabled && _shadowedScene) {
        int shadowMapSize = getShadowMapSize(_shadowQuality);

        if (enabled) {
            osg::ref_ptr<osgShadow::SoftShadowMap> ssm = new osgShadow::SoftShadowMap;
            ssm->setTextureSize(osg::Vec2s(shadowMapSize, shadowMapSize));
            ssm->setSoftnessWidth(0.005f);
            _shadowedScene->setShadowTechnique(ssm.get());
        }
        else {
            osg::ref_ptr<osgShadow::ShadowMap> sm = new osgShadow::ShadowMap;
            sm->setTextureSize(osg::Vec2s(shadowMapSize, shadowMapSize));
            _shadowedScene->setShadowTechnique(sm.get());
        }

        render();
    }
}

bool OsgVerseViewer::isSoftShadowEnabled() const
{
    return _softShadowEnabled;
}

int OsgVerseViewer::getShadowMapSize(ShadowQuality quality) const
{
    switch (quality) {
        case ShadowQuality::Low:    return 512;
        case ShadowQuality::Medium: return 1024;
        case ShadowQuality::High:   return 2048;
        case ShadowQuality::Ultra:  return 4096;
        default:                    return 1024;
    }
}
