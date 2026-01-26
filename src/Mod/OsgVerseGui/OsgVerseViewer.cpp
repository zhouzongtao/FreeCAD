// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseViewer.h"
#include "OsgVerseWidget.h"
#include "GeometryConverter.h"
#include <Base/Console.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <App/DocumentObject.h>
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
{
    try {
        // Create Qt OpenGL widget
        _widget = new OsgVerseWidget(parent);
        
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
    
    // TODO: Implement ray picking using osgUtil::LineSegmentIntersector
    
    return result;
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
    _selectionMode = Gui::View3D::SelectionMode::None;
}

void OsgVerseViewer::abortSelection()
{
    _selectionMode = Gui::View3D::SelectionMode::None;
}

bool OsgVerseViewer::isSelecting() const
{
    return _selectionMode != Gui::View3D::SelectionMode::None;
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

    Base::Console().message("OsgVerseViewer::updateViewProvider: Updating VP\n");

    auto it = _vpNodes.find(vp);
    if (it == _vpNodes.end()) {
        // ViewProvider not in scene, try to add it now
        Base::Console().message("OsgVerseViewer::updateViewProvider: VP not in scene, adding now\n");
        addViewProvider(vp);
        return;
    }

    // Remove old node
    osg::ref_ptr<osg::Node> oldNode = it->second;
    _sceneRoot->removeChild(oldNode.get());

    // Create new node with updated geometry
    osg::ref_ptr<osg::Node> newNode = createNodeForViewProvider(vp);

    if (!newNode) {
        Base::Console().message("OsgVerseViewer::updateViewProvider: Still no geometry, keeping placeholder\n");
        newNode = createPlaceholderSphere();
    } else {
        Base::Console().message("OsgVerseViewer::updateViewProvider: Geometry updated successfully\n");
    }

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
    
    // TODO: Apply render mode to scene
    
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
    
    // TODO: Implement backlight
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
    // TODO: Actually change the navigation style
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
    
    // TODO: Collect actual stats from OSG
    stats.fps = 60.0;
    stats.frameTime = 16.67;
    stats.triangleCount = 0;
    stats.vertexCount = 0;
    stats.drawCalls = 0;
    stats.frameCount = 0;
    
    return stats;
}

void OsgVerseViewer::resetStats()
{
    // TODO: Reset stats
}

void OsgVerseViewer::setFPSEnabled(bool enabled)
{
    _fpsEnabled = enabled;
    
    // TODO: Show/hide FPS display
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
        
        // Apply material
        applyMaterial(geode.get(), Base::Color(0.8f, 0.8f, 0.8f));
        
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
    if (!node) {
        return;
    }
    
    osg::ref_ptr<osg::StateSet> stateSet = node->getOrCreateStateSet();
    osg::ref_ptr<osg::Material> material = new osg::Material();
    
    osg::Vec4 diffuse(color.r, color.g, color.b, 1.0f);
    material->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
    material->setAmbient(osg::Material::FRONT_AND_BACK, diffuse * 0.3f);
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
    material->setShininess(osg::Material::FRONT_AND_BACK, 32.0f);
    
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
}
