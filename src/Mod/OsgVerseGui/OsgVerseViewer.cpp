// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseViewer.h"
#include "OsgVerseWidget.h"
#include "GeometryConverter.h"
#include "OsgVersePostProcess.h"
#include "OsgVerseNaviCube.h"
#include "OsgVerseBackground.h"
#include <Base/Console.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <App/DocumentObject.h>
#include <App/Property.h>
#include <App/PropertyStandard.h>
#include <App/Material.h>
#include <Gui/Render/Core/RenderTypes.h>

// STL includes for pickRegion
#include <set>
#include <algorithm>

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
#include <osg/LightModel>
#include <osg/MatrixTransform>
#include <osg/Transform>
#include <osg/ComputeBoundsVisitor>
#include <osgGA/CameraManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/OrbitManipulator>

// OSG picking includes
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/PolytopeIntersector>
#include <osgUtil/IntersectionVisitor>

// OSG rendering state includes
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/LineWidth>
#include <osg/BlendFunc>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/ColorMask>
#include <osg/Depth>
#include <osg/CullFace>

// OSG stats includes
#include <osg/Stats>
#include <osg/NodeVisitor>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Group>
#include <osg/PrimitiveSet>
#include <osg/Texture>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osg/StateSet>
#include <osg/Array>

// OSG shadow includes
#include <osgShadow/ShadowedScene>
#include <osgShadow/ShadowMap>
#include <osgShadow/SoftShadowMap>

// OSG text includes for FPS display
#include <osgText/Text>
#include <osgText/Font>
#include <osgDB/ReadFile>

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

        // Create background gradient renderer
        _background = std::make_unique<OsgVerseBackground>();
        _background->setSolidColor(_backgroundColor);

        // Add background camera to scene root (renders before main scene due to PRE_RENDER order)
        // The background camera uses ABSOLUTE_RF so it's not affected by scene transforms
        if (_background->getCamera()) {
            _sceneRoot->addChild(_background->getCamera());
        }

        // Set default linear gradient (matches Coin3D default)
        Gui::View3D::BackgroundGradient defaultGradient;
        defaultGradient.type = Gui::View3D::BackgroundGradientType::Linear;
        defaultGradient.topColor = Base::Color(0.4f, 0.4f, 0.6f);     // Light blue-gray
        defaultGradient.bottomColor = Base::Color(0.1f, 0.1f, 0.2f);  // Dark blue-gray
        setBackgroundGradient(defaultGradient);
        
        // Setup multi-light system for better illumination
        osg::StateSet* stateSet = _sceneRoot->getOrCreateStateSet();
        
        // Light 0: Main headlight (follows camera)
        osg::ref_ptr<osg::Light> light0 = new osg::Light();
        light0->setLightNum(0);
        // Directional light from camera direction
        light0->setPosition(osg::Vec4(0.0f, 0.0f, 1.0f, 0.0f));
        light0->setAmbient(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));  // Reduced ambient
        light0->setDiffuse(osg::Vec4(0.6f, 0.6f, 0.6f, 1.0f));  // Reduced main light intensity
        light0->setSpecular(osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f)); // Reduced specular

        osg::ref_ptr<osg::LightSource> lightSource0 = new osg::LightSource();
        lightSource0->setLight(light0.get());
        lightSource0->setLocalStateSetModes(osg::StateAttribute::ON);
        lightSource0->setReferenceFrame(osg::LightSource::RELATIVE_RF);
        _sceneRoot->addChild(lightSource0.get());

        // Light 1: Fill light from upper right (fixed position)
        osg::ref_ptr<osg::Light> light1 = new osg::Light();
        light1->setLightNum(1);
        // Directional light from upper right
        light1->setPosition(osg::Vec4(1.0f, 1.0f, 1.0f, 0.0f));
        light1->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        light1->setDiffuse(osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));  // Increased fill light
        light1->setSpecular(osg::Vec4(0.1f, 0.1f, 0.1f, 1.0f)); // Reduced specular

        osg::ref_ptr<osg::LightSource> lightSource1 = new osg::LightSource();
        lightSource1->setLight(light1.get());
        lightSource1->setLocalStateSetModes(osg::StateAttribute::ON);
        lightSource1->setReferenceFrame(osg::LightSource::ABSOLUTE_RF);
        _sceneRoot->addChild(lightSource1.get());

        // Light 2: Back light from lower left (fixed position)
        osg::ref_ptr<osg::Light> light2 = new osg::Light();
        light2->setLightNum(2);
        // Directional light from lower left back
        light2->setPosition(osg::Vec4(-1.0f, -1.0f, -0.5f, 0.0f));
        light2->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        light2->setDiffuse(osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));  // Increased back light
        light2->setSpecular(osg::Vec4(0.05f, 0.05f, 0.05f, 1.0f)); // Very low specular

        osg::ref_ptr<osg::LightSource> lightSource2 = new osg::LightSource();
        lightSource2->setLight(light2.get());
        lightSource2->setLocalStateSetModes(osg::StateAttribute::ON);
        lightSource2->setReferenceFrame(osg::LightSource::ABSOLUTE_RF);
        _sceneRoot->addChild(lightSource2.get());

        // Enable lighting and all three lights
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        stateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);
        stateSet->setMode(GL_LIGHT1, osg::StateAttribute::ON);
        stateSet->setMode(GL_LIGHT2, osg::StateAttribute::ON);
        
        // Normalize normals for proper lighting
        stateSet->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

        // Enable two-sided lighting for better visibility
        osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel();
        lightModel->setTwoSided(true);
        lightModel->setAmbientIntensity(osg::Vec4(0.25f, 0.25f, 0.25f, 1.0f));  // Balanced global ambient
        lightModel->setLocalViewer(true);
        stateSet->setAttributeAndModes(lightModel.get(), osg::StateAttribute::ON);
        
        // NOTE: Default camera position is set in OsgVerseWidget constructor
        // Don't override it here - it's already configured for TOP view
        // The home position is set to (0, 0, 100) looking down with Y-up
        
        // NaviCube will be created lazily on first render
        // Don't create it here as the viewer may not be fully initialized
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
        // Update FPS display if enabled
        if (_fpsEnabled) {
            updateFPSDisplay();
        }

        // Create NaviCube if enabled (lazy initialization)
        // Note: draw() is called from paintGL() where OpenGL context is active
        if (_naviCubeEnabled && !_naviCube && _sceneRoot.valid()) {
            try {
                _naviCube = std::make_unique<OsgVerseNaviCube>(this);
                _naviCube->resize(_widget->width(), _widget->height());
            } catch (const std::exception& e) {
                Base::Console().error("OsgVerseViewer: Failed to create NaviCube: %s\n", e.what());
                _naviCubeEnabled = false;
            }
        }

        _widget->update(); // Triggers paintGL()
    }
}

void OsgVerseViewer::resize(int width, int height)
{
    if (_widget) {
        _widget->resize(width, height);
    }

    // Get actual pixel size (considering high DPI)
    qreal dpr = _widget ? _widget->devicePixelRatio() : 1.0;
    int pixelWidth = static_cast<int>(width * dpr);
    int pixelHeight = static_cast<int>(height * dpr);

    // Update FPS camera projection for new size (use pixel size)
    if (_fpsCamera.valid() && pixelWidth > 0 && pixelHeight > 0) {
        _fpsCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, pixelWidth, 0, pixelHeight));

        // Update text positions
        if (_fpsText.valid()) {
            _fpsText->setPosition(osg::Vec3(10.0f, static_cast<float>(pixelHeight) - 25.0f, 0.0f));
        }
        if (_statsText.valid()) {
            _statsText->setPosition(osg::Vec3(10.0f, static_cast<float>(pixelHeight) - 50.0f, 0.0f));
        }
    }

    // Update NaviCube for new size (use pixel size)
    if (_naviCube && pixelWidth > 0 && pixelHeight > 0) {
        _naviCube->resize(pixelWidth, pixelHeight);
    }

    // Update background for new size (use pixel size)
    if (_background && pixelWidth > 0 && pixelHeight > 0) {
        _background->resize(pixelWidth, pixelHeight);
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

    // Update camera manipulator (important: must update manipulator, not just camera)
    osgGA::CameraManipulator* manipulator = viewer->getCameraManipulator();
    if (manipulator) {
        osg::Matrixd viewMatrix;
        viewMatrix.makeLookAt(eye, center, up);
        manipulator->setByMatrix(osg::Matrixd::inverse(viewMatrix));
    }

    // Also set camera directly for immediate effect
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

void OsgVerseViewer::fitSelection()
{
    if (!_widget) {
        return;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        return;
    }

    // Get the FreeCAD selection
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    if (selection.empty()) {
        // No selection, fall back to viewAll
        viewAll();
        return;
    }

    // Compute the combined bounding box of all selected objects
    osg::BoundingBox combinedBBox;

    for (const auto& selObj : selection) {
        const App::DocumentObject* obj = selObj.getObject();
        if (!obj) {
            continue;
        }

        // Find the ViewProvider for this object
        Gui::ViewProvider* vp = nullptr;
        for (const auto& vpPair : _vpNodes) {
            Gui::ViewProviderDocumentObject* vpDoc =
                dynamic_cast<Gui::ViewProviderDocumentObject*>(vpPair.first);
            if (vpDoc && vpDoc->getObject() == obj) {
                vp = vpPair.first;
                break;
            }
        }

        if (!vp) {
            continue;
        }

        // Get the OSG node for this ViewProvider
        auto it = _vpNodes.find(vp);
        if (it == _vpNodes.end() || !it->second.valid()) {
            continue;
        }

        // Compute bounds for this node
        osg::ComputeBoundsVisitor cbv;
        it->second->accept(cbv);
        osg::BoundingBox nodeBBox = cbv.getBoundingBox();

        if (nodeBBox.valid()) {
            combinedBBox.expandBy(nodeBBox);
        }
    }

    if (!combinedBBox.valid()) {
        // No valid bounds found, fall back to viewAll
        viewAll();
        return;
    }

    // Compute bounding sphere from bounding box
    osg::BoundingSphere bs;
    bs.expandBy(combinedBBox);

    if (bs.radius() <= 0.0) {
        viewAll();
        return;
    }

    // Get the camera manipulator and adjust view
    osgGA::CameraManipulator* manipulator = viewer->getCameraManipulator();
    if (!manipulator) {
        viewAll();
        return;
    }

    // Try to cast to different manipulator types to set center and distance
    osgGA::TrackballManipulator* trackball =
        dynamic_cast<osgGA::TrackballManipulator*>(manipulator);
    if (trackball) {
        // Set isometric view to match Coin3D (shows top, front, right)
        double dist = bs.radius() * 2.5;
        osg::Vec3d eye = bs.center() + osg::Vec3d(dist, -dist, dist);
        osg::Vec3d center = bs.center();
        osg::Vec3d up(0, 1, 0);  // Y-up
        
        // Create view matrix and set it
        osg::Matrixd viewMatrix;
        viewMatrix.makeLookAt(eye, center, up);
        trackball->setByMatrix(osg::Matrixd::inverse(viewMatrix));
        
        render();
        return;
    }

    osgGA::OrbitManipulator* orbit =
        dynamic_cast<osgGA::OrbitManipulator*>(manipulator);
    if (orbit) {
        orbit->setCenter(bs.center());
        orbit->setDistance(bs.radius() * 2.5);
        render();
        return;
    }

    // Generic approach: set home position and go home
    // Use isometric view to match Coin3D default (shows top, front, right)
    // Isometric view: eye at (1, -1, 1) direction relative to center
    double dist = bs.radius() * 2.5;
    osg::Vec3d eye = bs.center() + osg::Vec3d(dist, -dist, dist);
    osg::Vec3d center = bs.center();
    osg::Vec3d up(0, 1, 0);  // Y-up to match Coin3D
    manipulator->setHomePosition(eye, center, up);
    viewer->home();
    render();
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
    // NaviCube handles events first
    if (_naviCubeEnabled && _naviCube && _naviCube->handleMouseEvent(event)) {
        return true;
    }

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
        Base::Console().log("OsgVerseViewer::pick: No widget\n");
        return result;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        Base::Console().log("OsgVerseViewer::pick: No viewer\n");
        return result;
    }

    osg::Camera* camera = viewer->getCamera();
    if (!camera) {
        Base::Console().log("OsgVerseViewer::pick: No camera\n");
        return result;
    }

    // Get device pixel ratio for HiDPI support
    float dpr = _widget->devicePixelRatioF();

    // Coordinate conversion: Qt (top-left origin, Y down) -> OSG (bottom-left origin, Y up)
    // Also apply device pixel ratio for HiDPI displays
    int windowHeight = _widget->height();
    float x = static_cast<float>(pos.x()) * dpr;
    float y = static_cast<float>(windowHeight - pos.y()) * dpr;


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

    // Determine pick type (Face/Edge/Vertex) and set sub-element name
    determinePickType(intersection, result);

    return result;
}

//===========================================================================
// 多目标拾取 (Multi-target Picking)
//===========================================================================

std::vector<Gui::View3D::PickResult> OsgVerseViewer::pickAll(const QPoint& pos,
                                                              int maxHits,
                                                              bool pickVisibleOnly,
                                                              bool pickSelectableOnly)
{
    std::vector<Gui::View3D::PickResult> results;

    // Get viewer
    if (!_widget) {
        Base::Console().log("OsgVerseViewer::pickAll: No widget\n");
        return results;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        Base::Console().log("OsgVerseViewer::pickAll: No viewer\n");
        return results;
    }

    osg::Camera* camera = viewer->getCamera();
    if (!camera) {
        Base::Console().log("OsgVerseViewer::pickAll: No camera\n");
        return results;
    }

    // Get device pixel ratio for HiDPI support
    float dpr = _widget->devicePixelRatioF();

    // Coordinate conversion: Qt (top-left origin, Y down) -> OSG (bottom-left origin, Y up)
    int windowHeight = _widget->height();
    float x = static_cast<float>(pos.x()) * dpr;
    float y = static_cast<float>(windowHeight - pos.y()) * dpr;

    // Create line segment intersector
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(
            osgUtil::Intersector::WINDOW,
            x, y
        );

    // Configure to get ALL intersections, not just the nearest
    intersector->setIntersectionLimit(osgUtil::Intersector::NO_LIMIT);

    // Create intersection visitor
    osgUtil::IntersectionVisitor iv(intersector.get());
    iv.setTraversalMask(~0);  // Traverse all nodes

    // Execute intersection test from camera
    camera->accept(iv);

    // Check results
    if (!intersector->containsIntersections()) {
        return results;
    }

    // Get camera eye position for distance calculation
    osg::Vec3d eye, center, up;
    camera->getViewMatrixAsLookAt(eye, center, up);

    // Get all intersections (already sorted by distance in OSG)
    const osgUtil::LineSegmentIntersector::Intersections& intersections =
        intersector->getIntersections();

    // Track unique ViewProviders to avoid duplicates
    std::set<Gui::ViewProvider*> seenViewProviders;

    int hitCount = 0;
    for (const auto& intersection : intersections) {
        // Check max hits limit
        if (maxHits > 0 && hitCount >= maxHits) {
            break;
        }

        Gui::View3D::PickResult pickResult;
        pickResult.valid = true;

        // Extract intersection point in world coordinates
        osg::Vec3d worldPoint = intersection.getWorldIntersectPoint();
        pickResult.point = Base::Vector3d(worldPoint.x(), worldPoint.y(), worldPoint.z());

        // Extract surface normal
        osg::Vec3d worldNormal = intersection.getWorldIntersectNormal();
        pickResult.normal = Base::Vector3d(worldNormal.x(), worldNormal.y(), worldNormal.z());

        // Calculate distance from camera eye to intersection point
        pickResult.distance = (worldPoint - eye).length();

        // Find ViewProvider from node path
        pickResult.viewProvider = findViewProviderFromNodePath(intersection.nodePath);

        // Skip if no ViewProvider found
        if (!pickResult.viewProvider) {
            continue;
        }

        // Determine pick type (Face/Edge/Vertex) and set sub-element name
        determinePickType(intersection, pickResult);

        // Apply visibility filter
        if (pickVisibleOnly && !pickResult.viewProvider->isVisible()) {
            continue;
        }

        // Apply selectability filter
        if (pickSelectableOnly && !pickResult.viewProvider->isSelectable()) {
            continue;
        }

        // Check for duplicate ViewProvider (same object hit multiple times)
        // We keep all hits for the same VP as they may represent different faces/elements
        // But we track it for logging purposes
        bool isNewVP = (seenViewProviders.find(pickResult.viewProvider) == seenViewProviders.end());
        if (isNewVP) {
            seenViewProviders.insert(pickResult.viewProvider);
        }

        results.push_back(pickResult);
        hitCount++;
    }

    // Results are already sorted by distance (OSG sorts them)
    // No need to mark isClosest as it's not part of View3D::PickResult

    Base::Console().log("OsgVerseViewer::pickAll: Found %d hits (%d unique ViewProviders) at (%d, %d)\n",
                        static_cast<int>(results.size()),
                        static_cast<int>(seenViewProviders.size()),
                        pos.x(), pos.y());

    return results;
}

std::vector<Gui::ViewProvider*> OsgVerseViewer::pickAllViewProviders(const QPoint& pos, int maxHits)
{
    std::vector<Gui::ViewProvider*> results;

    // Get viewer
    if (!_widget) {
        return results;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        return results;
    }

    osg::Camera* camera = viewer->getCamera();
    if (!camera) {
        return results;
    }

    // Get device pixel ratio for HiDPI support
    float dpr = _widget->devicePixelRatioF();

    // Coordinate conversion
    int windowHeight = _widget->height();
    float x = static_cast<float>(pos.x()) * dpr;
    float y = static_cast<float>(windowHeight - pos.y()) * dpr;

    // Create line segment intersector
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(
            osgUtil::Intersector::WINDOW,
            x, y
        );

    // Get all intersections
    intersector->setIntersectionLimit(osgUtil::Intersector::NO_LIMIT);

    // Create intersection visitor
    osgUtil::IntersectionVisitor iv(intersector.get());
    iv.setTraversalMask(~0);

    // Execute intersection test
    camera->accept(iv);

    if (!intersector->containsIntersections()) {
        return results;
    }

    // Get camera eye for distance sorting
    osg::Vec3d eye, center, up;
    camera->getViewMatrixAsLookAt(eye, center, up);

    // Collect unique ViewProviders with their nearest distance
    struct VPDistance {
        Gui::ViewProvider* vp;
        double distance;
        bool operator<(const VPDistance& other) const {
            return distance < other.distance;
        }
    };
    std::vector<VPDistance> vpDistances;
    std::set<Gui::ViewProvider*> seenVPs;

    const osgUtil::LineSegmentIntersector::Intersections& intersections =
        intersector->getIntersections();

    for (const auto& intersection : intersections) {
        Gui::ViewProvider* vp = findViewProviderFromNodePath(intersection.nodePath);

        if (vp && seenVPs.find(vp) == seenVPs.end()) {
            // Skip invisible or non-selectable
            if (!vp->isVisible() || !vp->isSelectable()) {
                continue;
            }

            seenVPs.insert(vp);

            // Calculate distance
            osg::Vec3d worldPoint = intersection.getWorldIntersectPoint();
            double distance = (worldPoint - eye).length();

            vpDistances.push_back({vp, distance});
        }
    }

    // Sort by distance (nearest first)
    std::sort(vpDistances.begin(), vpDistances.end());

    // Apply max hits limit and extract ViewProviders
    int count = 0;
    for (const auto& vpd : vpDistances) {
        if (maxHits > 0 && count >= maxHits) {
            break;
        }
        results.push_back(vpd.vp);
        count++;
    }

    return results;
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

//===========================================================================
// 拾取类型检测 (Pick Type Detection)
//===========================================================================

void OsgVerseViewer::determinePickType(const osgUtil::LineSegmentIntersector::Intersection& intersection,
                                        Gui::View3D::PickResult& result)
{
    // Default to Object type (hit something but can't determine Face/Edge/Vertex)
    result.pickType = Gui::View3D::PickType::Object;
    result.primitiveIndex = -1;
    result.faceIndex = -1;
    result.edgeIndex = -1;
    result.vertexIndex = -1;

    // Get the drawable that was hit
    osg::Drawable* drawable = intersection.drawable.get();
    if (!drawable) {
        return;
    }

    // Try to get the geometry
    osg::Geometry* geometry = drawable->asGeometry();
    if (!geometry) {
        return;
    }

    // Get the primitive index from the intersection
    unsigned int primitiveIndex = intersection.primitiveIndex;
    result.primitiveIndex = static_cast<int>(primitiveIndex);

    // Get the primitive set that contains this primitive
    // We need to find which primitive set and what type it is
    const osg::Geometry::PrimitiveSetList& primitiveSets = geometry->getPrimitiveSetList();

    if (primitiveSets.empty()) {
        return;
    }

    // Calculate which primitive set and local index
    unsigned int runningCount = 0;
    for (unsigned int i = 0; i < primitiveSets.size(); ++i) {
        const osg::PrimitiveSet* primSet = primitiveSets[i];
        if (!primSet) continue;

        unsigned int numPrimitives = primSet->getNumPrimitives();

        if (primitiveIndex < runningCount + numPrimitives) {
            // Found the primitive set containing this primitive
            unsigned int localIndex = primitiveIndex - runningCount;

            // Determine type based on primitive set mode
            GLenum mode = primSet->getMode();

            switch (mode) {
                case GL_TRIANGLES:
                case GL_TRIANGLE_STRIP:
                case GL_TRIANGLE_FAN:
                case GL_QUADS:
                case GL_QUAD_STRIP:
                case GL_POLYGON:
                    // Face primitive
                    result.pickType = Gui::View3D::PickType::Face;
                    result.faceIndex = static_cast<int>(localIndex);
                    result.subElementName = generateSubElementName(Gui::View3D::PickType::Face, result.faceIndex + 1);
                    break;

                case GL_LINES:
                case GL_LINE_STRIP:
                case GL_LINE_LOOP:
                    // Edge primitive
                    result.pickType = Gui::View3D::PickType::Edge;
                    result.edgeIndex = static_cast<int>(localIndex);
                    result.subElementName = generateSubElementName(Gui::View3D::PickType::Edge, result.edgeIndex + 1);
                    break;

                case GL_POINTS:
                    // Vertex/Point primitive
                    result.pickType = Gui::View3D::PickType::Vertex;
                    result.vertexIndex = static_cast<int>(localIndex);
                    result.subElementName = generateSubElementName(Gui::View3D::PickType::Vertex, result.vertexIndex + 1);
                    break;

                default:
                    // Unknown primitive type, keep as Object
                    result.pickType = Gui::View3D::PickType::Object;
                    break;
            }

            return;
        }

        runningCount += numPrimitives;
    }

    // If we reach here, the primitive index wasn't found
    // This can happen with complex geometries; default to Face for triangulated meshes
    if (!primitiveSets.empty()) {
        GLenum mode = primitiveSets[0]->getMode();
        if (mode == GL_TRIANGLES || mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN) {
            result.pickType = Gui::View3D::PickType::Face;
            result.faceIndex = static_cast<int>(primitiveIndex);
            result.subElementName = generateSubElementName(Gui::View3D::PickType::Face, result.faceIndex + 1);
        }
    }
}

std::string OsgVerseViewer::generateSubElementName(Gui::View3D::PickType pickType, int index)
{
    std::string name;

    switch (pickType) {
        case Gui::View3D::PickType::Face:
            name = "Face" + std::to_string(index);
            break;
        case Gui::View3D::PickType::Edge:
            name = "Edge" + std::to_string(index);
            break;
        case Gui::View3D::PickType::Vertex:
            name = "Vertex" + std::to_string(index);
            break;
        default:
            name = "";
            break;
    }

    return name;
}

//===========================================================================
// 区域拾取 (Region Picking)
//===========================================================================

std::vector<Gui::ViewProvider*> OsgVerseViewer::pickRegion(int x1, int y1, int x2, int y2,
                                                           bool pickVisibleOnly,
                                                           bool pickSelectableOnly)
{
    std::vector<Gui::ViewProvider*> results;

    // Get viewer
    if (!_widget) {
        Base::Console().log("OsgVerseViewer::pickRegion: No widget\n");
        return results;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        Base::Console().log("OsgVerseViewer::pickRegion: No viewer\n");
        return results;
    }

    osg::Camera* camera = viewer->getCamera();
    if (!camera) {
        Base::Console().log("OsgVerseViewer::pickRegion: No camera\n");
        return results;
    }

    // Get device pixel ratio for HiDPI support
    float dpr = _widget->devicePixelRatioF();

    // Normalize coordinates: ensure x1 < x2 and y1 < y2
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    // Coordinate conversion: Qt (top-left origin, Y down) -> OSG (bottom-left origin, Y up)
    int windowHeight = _widget->height();

    // Apply device pixel ratio for HiDPI displays
    float osgX1 = static_cast<float>(x1) * dpr;
    float osgX2 = static_cast<float>(x2) * dpr;
    // Note: Y is inverted for OSG coordinate system
    float osgY1 = static_cast<float>(windowHeight - y2) * dpr;  // bottom in OSG
    float osgY2 = static_cast<float>(windowHeight - y1) * dpr;  // top in OSG

    // Create PolytopeIntersector for the rectangular region
    // Using WINDOW coordinate frame - the intersector will create a frustum
    // from the camera that encompasses the specified window rectangle
    osg::ref_ptr<osgUtil::PolytopeIntersector> intersector =
        new osgUtil::PolytopeIntersector(
            osgUtil::Intersector::WINDOW,
            osgX1, osgY1, osgX2, osgY2
        );

    // Configure intersector - use LIMIT_ONE_PER_DRAWABLE for compatible behavior
    intersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_ONE_PER_DRAWABLE);

    // Create intersection visitor
    osgUtil::IntersectionVisitor iv(intersector.get());
    iv.setTraversalMask(~0);  // Traverse all nodes

    // Execute intersection test from camera
    camera->accept(iv);

    // Check results
    if (!intersector->containsIntersections()) {
        return results;
    }

    // Collect unique ViewProviders from intersections
    std::set<Gui::ViewProvider*> uniqueVPs;

    const osgUtil::PolytopeIntersector::Intersections& intersections =
        intersector->getIntersections();

    for (const auto& intersection : intersections) {
        // Find ViewProvider from node path
        Gui::ViewProvider* vp = findViewProviderFromNodePath(intersection.nodePath);

        if (vp && uniqueVPs.find(vp) == uniqueVPs.end()) {
            // Apply visibility filter
            if (pickVisibleOnly && !vp->isVisible()) {
                continue;
            }

            // Apply selectability filter
            if (pickSelectableOnly && !vp->isSelectable()) {
                continue;
            }

            uniqueVPs.insert(vp);
            results.push_back(vp);
        }
    }

    Base::Console().log("OsgVerseViewer::pickRegion: Found %d ViewProviders in region (%d,%d)-(%d,%d)\n",
                        static_cast<int>(results.size()), x1, y1, x2, y2);

    return results;
}

std::vector<Gui::View3D::PickResult> OsgVerseViewer::pickRegionDetailed(int x1, int y1, int x2, int y2)
{
    std::vector<Gui::View3D::PickResult> results;

    // Get viewer
    if (!_widget) {
        Base::Console().log("OsgVerseViewer::pickRegionDetailed: No widget\n");
        return results;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        Base::Console().log("OsgVerseViewer::pickRegionDetailed: No viewer\n");
        return results;
    }

    osg::Camera* camera = viewer->getCamera();
    if (!camera) {
        Base::Console().log("OsgVerseViewer::pickRegionDetailed: No camera\n");
        return results;
    }

    // Get device pixel ratio for HiDPI support
    float dpr = _widget->devicePixelRatioF();

    // Normalize coordinates: ensure x1 < x2 and y1 < y2
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    // Coordinate conversion: Qt (top-left origin, Y down) -> OSG (bottom-left origin, Y up)
    int windowHeight = _widget->height();

    // Apply device pixel ratio for HiDPI displays
    float osgX1 = static_cast<float>(x1) * dpr;
    float osgX2 = static_cast<float>(x2) * dpr;
    float osgY1 = static_cast<float>(windowHeight - y2) * dpr;
    float osgY2 = static_cast<float>(windowHeight - y1) * dpr;

    // Create PolytopeIntersector with LIMIT_ONE_PER_DRAWABLE to get all unique intersections
    osg::ref_ptr<osgUtil::PolytopeIntersector> intersector =
        new osgUtil::PolytopeIntersector(
            osgUtil::Intersector::WINDOW,
            osgX1, osgY1, osgX2, osgY2
        );

    // Get all intersections, not just nearest per drawable
    intersector->setIntersectionLimit(osgUtil::Intersector::NO_LIMIT);

    // Create intersection visitor
    osgUtil::IntersectionVisitor iv(intersector.get());
    iv.setTraversalMask(~0);

    // Execute intersection test
    camera->accept(iv);

    // Check results
    if (!intersector->containsIntersections()) {
        return results;
    }

    // Get camera eye position for distance calculation
    osg::Vec3d eye, center, up;
    camera->getViewMatrixAsLookAt(eye, center, up);

    // Process all intersections
    const osgUtil::PolytopeIntersector::Intersections& intersections =
        intersector->getIntersections();

    // Track ViewProviders we've already added (for deduplication if needed)
    std::set<Gui::ViewProvider*> processedVPs;

    for (const auto& intersection : intersections) {
        Gui::View3D::PickResult pickResult;
        pickResult.valid = true;

        // Note: PolytopeIntersector doesn't provide exact intersection point
        // like LineSegmentIntersector. We'll use an approximate center point.
        // For detailed point information, use pick() with specific points.

        // Get bounding box center as approximate intersection point
        if (!intersection.nodePath.empty()) {
            osg::Node* node = intersection.nodePath.back();
            if (node) {
                osg::BoundingSphere bs = node->getBound();
                osg::Vec3d worldCenter = bs.center();

                // Transform to world coordinates if needed
                osg::NodePath::const_iterator pathIter;
                for (pathIter = intersection.nodePath.begin();
                     pathIter != intersection.nodePath.end() - 1; ++pathIter) {
                    osg::Transform* transform = (*pathIter)->asTransform();
                    if (transform) {
                        osg::MatrixTransform* mt = transform->asMatrixTransform();
                        if (mt) {
                            worldCenter = worldCenter * mt->getMatrix();
                        }
                    }
                }

                pickResult.point = Base::Vector3d(worldCenter.x(), worldCenter.y(), worldCenter.z());

                // Calculate distance from camera
                pickResult.distance = (worldCenter - eye).length();
            }
        }

        // Default normal (not available from PolytopeIntersector)
        pickResult.normal = Base::Vector3d(0.0, 0.0, 1.0);

        // Find ViewProvider
        pickResult.viewProvider = findViewProviderFromNodePath(intersection.nodePath);

        // Only add valid results with ViewProviders
        if (pickResult.viewProvider) {
            // Optional: deduplicate by ViewProvider
            if (processedVPs.find(pickResult.viewProvider) == processedVPs.end()) {
                processedVPs.insert(pickResult.viewProvider);
                results.push_back(pickResult);
            }
        }
    }

    // Sort by distance (nearest first)
    std::sort(results.begin(), results.end(),
              [](const Gui::View3D::PickResult& a, const Gui::View3D::PickResult& b) {
                  return a.distance < b.distance;
              });

    Base::Console().log("OsgVerseViewer::pickRegionDetailed: Found %d detailed results\n",
                        static_cast<int>(results.size()));

    return results;
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

std::vector<Gui::ViewProvider*> OsgVerseViewer::finishSelection()
{
    std::vector<Gui::ViewProvider*> selectedVPs;

    // Perform region picking based on selection mode
    if (_selectionMode == Gui::View3D::SelectionMode::Rectangle ||
        _selectionMode == Gui::View3D::SelectionMode::Rubberband) {
        // Use rectangle selection
        selectedVPs = pickRegion(
            _selectionStart.x(), _selectionStart.y(),
            _selectionCurrent.x(), _selectionCurrent.y(),
            true,   // pickVisibleOnly
            true    // pickSelectableOnly
        );
    }
    else if (_selectionMode == Gui::View3D::SelectionMode::Lasso) {
        // For lasso selection, use bounding box of lasso points
        // A more sophisticated implementation would do point-in-polygon testing
        if (!_lassoPoints.empty()) {
            int minX = _lassoPoints[0].x();
            int maxX = _lassoPoints[0].x();
            int minY = _lassoPoints[0].y();
            int maxY = _lassoPoints[0].y();

            for (const auto& pt : _lassoPoints) {
                minX = std::min(minX, pt.x());
                maxX = std::max(maxX, pt.x());
                minY = std::min(minY, pt.y());
                maxY = std::max(maxY, pt.y());
            }

            selectedVPs = pickRegion(minX, minY, maxX, maxY, true, true);

            // TODO: Filter by point-in-polygon for more accurate lasso selection
        }
    }

    clearSelectionVisualization();
    _selectionMode = Gui::View3D::SelectionMode::None;

    Base::Console().log("OsgVerseViewer::finishSelection: Selected %d ViewProviders\n",
                        static_cast<int>(selectedVPs.size()));

    return selectedVPs;
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
        // Shape may not be computed yet - store nullptr and skip adding to scene
        // The node will be added when updateViewProvider is called with valid geometry
        _vpNodes[vp] = nullptr;
        Base::Console().Log("OsgVerseViewer: ViewProvider %s has no geometry yet, deferring scene addition\n",
                          vp->getTypeId().getName());
        return;
    }

    // Set UserData for picking - allows finding ViewProvider from picked node
    node->setUserData(new ViewProviderUserData(vp));

    // Store and add to scene
    _vpNodes[vp] = node;
    _sceneRoot->addChild(node.get());

    // Auto-fit view when first object is added
    viewAll();
    render();
}

void OsgVerseViewer::removeViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }

    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        // Remove node from scene if it exists (may be nullptr if geometry was never available)
        if (it->second.valid()) {
            _sceneRoot->removeChild(it->second.get());
        }
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

    // Remove old node if it exists
    osg::ref_ptr<osg::Node> oldNode = it->second;
    if (oldNode.valid()) {
        _sceneRoot->removeChild(oldNode.get());
    }

    // Create new node with updated geometry
    osg::ref_ptr<osg::Node> newNode = createNodeForViewProvider(vp);

    if (!newNode) {
        // Geometry still not available - store nullptr and skip scene addition
        _vpNodes[vp] = nullptr;
        Base::Console().Log("OsgVerseViewer: ViewProvider %s still has no geometry after update\n",
                          vp->getTypeId().getName());
        render();
        return;
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

    // Clear any previous hidden line mode setup
    clearHiddenLineMode();

    osg::StateSet* stateSet = _sceneRoot->getOrCreateStateSet();

    // Remove existing polygon mode and related attributes
    stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    stateSet->removeAttribute(osg::StateAttribute::POLYGONOFFSET);
    stateSet->removeAttribute(osg::StateAttribute::LINEWIDTH);
    stateSet->removeAttribute(osg::StateAttribute::MATERIAL);

    // Reset render bin to default
    stateSet->setRenderBinDetails(0, "RenderBin");

    // Reset lighting to default (will be overridden per mode)
    stateSet->removeMode(GL_LIGHTING);

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
            // Hidden line - two-pass rendering for proper hidden line removal
            // This uses a technique where we render filled polygons with background color
            // first to establish depth, then render wireframe on top
            setupHiddenLineMode(stateSet);
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

void OsgVerseViewer::setupHiddenLineMode(osg::StateSet* stateSet)
{
    if (!_sceneRoot || !stateSet) {
        return;
    }

    // Clear any previous hidden line setup
    clearHiddenLineMode();

    _hiddenLineModeActive = true;

    // Hidden Line Rendering Strategy:
    // We use a two-pass approach within a single rendering cycle:
    //
    // Pass 1 (Render Order 0): Render filled polygons with background color
    //         This fills the depth buffer and creates the "solid" appearance
    //         that will occlude lines behind it
    //
    // Pass 2 (Render Order 1): Render wireframe with slight polygon offset
    //         Lines are drawn on top, but depth test removes hidden lines

    // Create Pass 1: Depth fill with background color
    _hiddenLinePass1 = new osg::Group();
    _hiddenLinePass1->setName("HiddenLinePass1_DepthFill");

    osg::StateSet* pass1State = _hiddenLinePass1->getOrCreateStateSet();

    // Render as filled polygons
    osg::ref_ptr<osg::PolygonMode> pm1 = new osg::PolygonMode();
    pm1->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
    pass1State->setAttributeAndModes(pm1.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // Use background color for the fill (creates the "paper" effect)
    osg::ref_ptr<osg::Material> bgMaterial = new osg::Material();
    osg::Vec4 bgColor(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, 1.0f);
    bgMaterial->setAmbient(osg::Material::FRONT_AND_BACK, bgColor);
    bgMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, bgColor);
    bgMaterial->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
    bgMaterial->setEmission(osg::Material::FRONT_AND_BACK, bgColor);
    pass1State->setAttributeAndModes(bgMaterial.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // Disable lighting for solid color
    pass1State->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

    // Enable depth writing
    osg::ref_ptr<osg::Depth> depth1 = new osg::Depth();
    depth1->setWriteMask(true);
    pass1State->setAttributeAndModes(depth1.get(), osg::StateAttribute::ON);

    // Render in bin 0 (first)
    pass1State->setRenderBinDetails(0, "RenderBin");

    // Clone the scene geometry for Pass 1
    // We add references to existing VP nodes to the Pass 1 group
    for (const auto& vpPair : _vpNodes) {
        if (vpPair.second.valid()) {
            _hiddenLinePass1->addChild(vpPair.second.get());
        }
    }

    // Add Pass 1 to scene (will render before the original nodes)
    _sceneRoot->addChild(_hiddenLinePass1.get());

    // Configure Pass 2 (main scene): Wireframe rendering
    // This applies to the existing scene nodes

    osg::ref_ptr<osg::PolygonMode> pm2 = new osg::PolygonMode();
    pm2->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    stateSet->setAttributeAndModes(pm2.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // Set line color (black for typical CAD hidden line)
    osg::ref_ptr<osg::Material> lineMaterial = new osg::Material();
    osg::Vec4 lineColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black lines
    lineMaterial->setAmbient(osg::Material::FRONT_AND_BACK, lineColor);
    lineMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, lineColor);
    lineMaterial->setEmission(osg::Material::FRONT_AND_BACK, lineColor);
    stateSet->setAttributeAndModes(lineMaterial.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // No lighting for lines
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

    // Polygon offset to prevent z-fighting (push lines toward camera)
    osg::ref_ptr<osg::PolygonOffset> po = new osg::PolygonOffset();
    po->setFactor(-1.0f);
    po->setUnits(-1.0f);
    stateSet->setAttributeAndModes(po.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // Set line width
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth();
    lw->setWidth(1.5f);
    stateSet->setAttributeAndModes(lw.get(), osg::StateAttribute::ON);

    // Render in bin 1 (after Pass 1)
    stateSet->setRenderBinDetails(1, "RenderBin");

    Base::Console().log("OsgVerseViewer: Hidden Line mode enabled\n");
}

void OsgVerseViewer::clearHiddenLineMode()
{
    if (!_hiddenLineModeActive) {
        return;
    }

    // Remove Pass 1 group from scene
    if (_hiddenLinePass1.valid() && _sceneRoot.valid()) {
        _sceneRoot->removeChild(_hiddenLinePass1.get());
        _hiddenLinePass1 = nullptr;
    }

    _hiddenLineModeActive = false;

    // Reset render bin details on main state set
    if (_sceneRoot.valid()) {
        osg::StateSet* stateSet = _sceneRoot->getStateSet();
        if (stateSet) {
            stateSet->setRenderBinDetails(0, "RenderBin");
        }
    }
}

void OsgVerseViewer::setBackgroundColor(const Base::Color& color)
{
    _backgroundColor = color;

    // Update background gradient renderer
    if (_background) {
        _background->setSolidColor(color);
    }

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

void OsgVerseViewer::setBackgroundGradient(const Gui::View3D::BackgroundGradient& gradient)
{
    _backgroundGradient = gradient;

    if (_background) {
        _background->setGradient(gradient);

        // When using gradient, disable camera clear color
        // The background shader will handle the background rendering
        if (_widget && gradient.type != Gui::View3D::BackgroundGradientType::None) {
            osgViewer::Viewer* viewer = _widget->getViewer();
            if (viewer) {
                // Set clear mask to only clear depth buffer, not color
                viewer->getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);
            }
            _background->setEnabled(true);
        }
        else if (_widget) {
            // Restore normal clear behavior for solid color
            osgViewer::Viewer* viewer = _widget->getViewer();
            if (viewer) {
                viewer->getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                osg::Vec4 bgColor(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, 1.0f);
                viewer->getCamera()->setClearColor(bgColor);
            }
            _background->setEnabled(false);
        }

        render();
    }
}

Gui::View3D::BackgroundGradient OsgVerseViewer::getBackgroundGradient() const
{
    return _backgroundGradient;
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

void OsgVerseViewer::setAmbientIntensity(float intensity)
{
    // Clamp intensity to valid range
    _ambientIntensity = std::clamp(intensity, 0.0f, 1.0f);

    if (!_sceneRoot) {
        return;
    }

    // Update the global light model's ambient intensity
    osg::StateSet* stateSet = _sceneRoot->getOrCreateStateSet();

    // Create or update LightModel
    osg::ref_ptr<osg::LightModel> lightModel = dynamic_cast<osg::LightModel*>(
        stateSet->getAttribute(osg::StateAttribute::LIGHTMODEL)
    );

    if (!lightModel) {
        lightModel = new osg::LightModel();
        lightModel->setTwoSided(true);
        lightModel->setLocalViewer(true);
    }

    // Set ambient intensity (gray value for neutral ambient light)
    lightModel->setAmbientIntensity(osg::Vec4(
        _ambientIntensity,
        _ambientIntensity,
        _ambientIntensity,
        1.0f
    ));

    stateSet->setAttributeAndModes(lightModel.get(), osg::StateAttribute::ON);

    // Also update the main light's ambient component for consistent lighting
    // Find the main light source in the scene
    for (unsigned int i = 0; i < _sceneRoot->getNumChildren(); ++i) {
        osg::LightSource* ls = dynamic_cast<osg::LightSource*>(_sceneRoot->getChild(i));
        if (ls && ls->getLight() && ls->getLight()->getLightNum() == 0) {
            // Scale the main light's ambient contribution
            // Use a slightly higher value for the light's ambient
            float lightAmbient = _ambientIntensity * 1.5f;  // Slightly brighter
            lightAmbient = std::min(lightAmbient, 1.0f);
            ls->getLight()->setAmbient(osg::Vec4(lightAmbient, lightAmbient, lightAmbient, 1.0f));
            break;
        }
    }

    Base::Console().log("OsgVerseViewer: Ambient intensity set to %.2f\n", _ambientIntensity);

    render();
}

float OsgVerseViewer::getAmbientIntensity() const
{
    return _ambientIntensity;
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

namespace {

/**
 * @brief Scene graph visitor for collecting render statistics
 *
 * Traverses the entire scene graph and counts:
 * - Total vertices across all geometry
 * - Total triangles (computed from primitive sets)
 * - Draw calls (number of Drawable objects)
 * - Geometry nodes count
 */
class StatsVisitor : public osg::NodeVisitor {
public:
    StatsVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
        , _vertexCount(0)
        , _triangleCount(0)
        , _drawCalls(0)
        , _geometryCount(0)
        , _geodeCount(0)
        , _nodeCount(0)
    {}

    void apply(osg::Geode& geode) override
    {
        _nodeCount++;
        _geodeCount++;

        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i) {
            osg::Drawable* drawable = geode.getDrawable(i);
            if (drawable) {
                _drawCalls++;
                processDrawable(drawable);
            }
        }

        traverse(geode);
    }

    void apply(osg::Geometry& geometry) override
    {
        _nodeCount++;
        _geometryCount++;
        _drawCalls++;
        processGeometry(&geometry);
        traverse(geometry);
    }

    void apply(osg::Group& group) override
    {
        _nodeCount++;
        traverse(group);
    }

    void apply(osg::Node& node) override
    {
        _nodeCount++;
        traverse(node);
    }

    // Getters for statistics
    uint32_t getVertexCount() const { return _vertexCount; }
    uint32_t getTriangleCount() const { return _triangleCount; }
    uint32_t getDrawCalls() const { return _drawCalls; }
    uint32_t getGeometryCount() const { return _geometryCount; }
    uint32_t getGeodeCount() const { return _geodeCount; }
    uint32_t getNodeCount() const { return _nodeCount; }

private:
    void processDrawable(osg::Drawable* drawable)
    {
        osg::Geometry* geometry = drawable->asGeometry();
        if (geometry) {
            processGeometry(geometry);
        }
    }

    void processGeometry(osg::Geometry* geometry)
    {
        // Count vertices
        osg::Array* vertices = geometry->getVertexArray();
        if (vertices) {
            _vertexCount += vertices->getNumElements();
        }

        // Count triangles from primitive sets
        for (unsigned int i = 0; i < geometry->getNumPrimitiveSets(); ++i) {
            osg::PrimitiveSet* primSet = geometry->getPrimitiveSet(i);
            if (primSet) {
                _triangleCount += countTriangles(primSet);
            }
        }
    }

    /**
     * @brief Count triangles in a primitive set
     *
     * Different GL modes have different triangle counts:
     * - GL_TRIANGLES: numVertices / 3
     * - GL_TRIANGLE_STRIP: numVertices - 2
     * - GL_TRIANGLE_FAN: numVertices - 2
     * - GL_QUADS: numVertices / 4 * 2
     * - GL_QUAD_STRIP: (numVertices - 2) / 2 * 2
     * - GL_POLYGON: numVertices - 2
     */
    uint32_t countTriangles(osg::PrimitiveSet* primSet) const
    {
        unsigned int numPrimitives = primSet->getNumPrimitives();
        unsigned int numIndices = primSet->getNumIndices();
        GLenum mode = primSet->getMode();

        switch (mode) {
            case GL_TRIANGLES:
                return numIndices / 3;

            case GL_TRIANGLE_STRIP:
            case GL_TRIANGLE_FAN:
                return (numIndices >= 3) ? numIndices - 2 : 0;

            case GL_QUADS:
                return (numIndices / 4) * 2;

            case GL_QUAD_STRIP:
                return (numIndices >= 4) ? ((numIndices - 2) / 2) * 2 : 0;

            case GL_POLYGON:
                return (numIndices >= 3) ? numIndices - 2 : 0;

            case GL_LINES:
            case GL_LINE_STRIP:
            case GL_LINE_LOOP:
            case GL_POINTS:
                // Lines and points don't contribute triangles
                return 0;

            default:
                // Unknown mode, try to estimate
                return numIndices / 3;
        }
    }

    uint32_t _vertexCount;
    uint32_t _triangleCount;
    uint32_t _drawCalls;
    uint32_t _geometryCount;
    uint32_t _geodeCount;
    uint32_t _nodeCount;
};

/**
 * @brief Scene graph visitor for collecting memory statistics
 *
 * Traverses the scene graph and calculates:
 * - Texture memory (from all osg::Texture objects)
 * - VBO/Geometry memory (from vertex arrays, normals, colors, etc.)
 */
class MemoryVisitor : public osg::NodeVisitor {
public:
    MemoryVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
        , _textureMemory(0)
        , _vboMemory(0)
        , _textureCount(0)
    {}

    void apply(osg::Node& node) override
    {
        // Check for textures in state set
        osg::StateSet* stateSet = node.getStateSet();
        if (stateSet) {
            processStateSet(stateSet);
        }
        traverse(node);
    }

    void apply(osg::Geode& geode) override
    {
        // Process state set
        osg::StateSet* stateSet = geode.getStateSet();
        if (stateSet) {
            processStateSet(stateSet);
        }

        // Process drawables for VBO memory
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i) {
            osg::Drawable* drawable = geode.getDrawable(i);
            if (drawable) {
                // Check drawable's state set
                osg::StateSet* drawableStateSet = drawable->getStateSet();
                if (drawableStateSet) {
                    processStateSet(drawableStateSet);
                }

                // Calculate geometry memory
                osg::Geometry* geometry = drawable->asGeometry();
                if (geometry) {
                    processGeometry(geometry);
                }
            }
        }

        traverse(geode);
    }

    void apply(osg::Geometry& geometry) override
    {
        osg::StateSet* stateSet = geometry.getStateSet();
        if (stateSet) {
            processStateSet(stateSet);
        }
        processGeometry(&geometry);
        traverse(geometry);
    }

    // Getters
    uint64_t getTextureMemory() const { return _textureMemory; }
    uint64_t getVBOMemory() const { return _vboMemory; }
    uint32_t getTextureCount() const { return _textureCount; }
    uint64_t getTotalMemory() const { return _textureMemory + _vboMemory; }

private:
    void processStateSet(osg::StateSet* stateSet)
    {
        // Iterate through texture attributes
        for (unsigned int unit = 0; unit < 16; ++unit) {  // Check up to 16 texture units
            osg::StateAttribute* attr = stateSet->getTextureAttribute(unit, osg::StateAttribute::TEXTURE);
            if (attr) {
                osg::Texture* texture = dynamic_cast<osg::Texture*>(attr);
                if (texture && _processedTextures.find(texture) == _processedTextures.end()) {
                    _processedTextures.insert(texture);
                    _textureMemory += calculateTextureMemory(texture);
                    _textureCount++;
                }
            }
        }
    }

    uint64_t calculateTextureMemory(osg::Texture* texture)
    {
        uint64_t memory = 0;

        // Try to get image(s) from texture
        osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(texture);
        if (tex2D) {
            osg::Image* image = tex2D->getImage();
            if (image) {
                memory = calculateImageMemory(image);
            } else {
                // Texture might be render target, estimate from dimensions
                int width = tex2D->getTextureWidth();
                int height = tex2D->getTextureHeight();
                if (width > 0 && height > 0) {
                    memory = static_cast<uint64_t>(width) * height * 4;  // Assume RGBA
                }
            }
            return memory;
        }

        osg::Texture3D* tex3D = dynamic_cast<osg::Texture3D*>(texture);
        if (tex3D) {
            osg::Image* image = tex3D->getImage();
            if (image) {
                memory = calculateImageMemory(image);
            } else {
                int width = tex3D->getTextureWidth();
                int height = tex3D->getTextureHeight();
                int depth = tex3D->getTextureDepth();
                if (width > 0 && height > 0 && depth > 0) {
                    memory = static_cast<uint64_t>(width) * height * depth * 4;
                }
            }
            return memory;
        }

        osg::TextureCubeMap* texCube = dynamic_cast<osg::TextureCubeMap*>(texture);
        if (texCube) {
            for (int face = 0; face < 6; ++face) {
                osg::Image* image = texCube->getImage(static_cast<osg::TextureCubeMap::Face>(face));
                if (image) {
                    memory += calculateImageMemory(image);
                }
            }
            return memory;
        }

        return memory;
    }

    uint64_t calculateImageMemory(osg::Image* image)
    {
        if (!image) return 0;

        uint64_t size = image->getTotalSizeInBytes();
        if (size > 0) {
            return size;
        }

        // Fallback: calculate from dimensions and pixel format
        int width = image->s();
        int height = image->t();
        int depth = image->r();
        unsigned int pixelSize = osg::Image::computePixelSizeInBits(
            image->getPixelFormat(), image->getDataType()) / 8;

        return static_cast<uint64_t>(width) * height * depth * pixelSize;
    }

    void processGeometry(osg::Geometry* geometry)
    {
        // Vertex array
        _vboMemory += calculateArrayMemory(geometry->getVertexArray());

        // Normal array
        _vboMemory += calculateArrayMemory(geometry->getNormalArray());

        // Color arrays
        _vboMemory += calculateArrayMemory(geometry->getColorArray());

        // Texture coordinate arrays
        for (unsigned int unit = 0; unit < geometry->getNumTexCoordArrays(); ++unit) {
            _vboMemory += calculateArrayMemory(geometry->getTexCoordArray(unit));
        }

        // Secondary color array
        _vboMemory += calculateArrayMemory(geometry->getSecondaryColorArray());

        // Fog coord array
        _vboMemory += calculateArrayMemory(geometry->getFogCoordArray());

        // Index arrays from primitive sets
        for (unsigned int i = 0; i < geometry->getNumPrimitiveSets(); ++i) {
            osg::PrimitiveSet* primSet = geometry->getPrimitiveSet(i);
            if (primSet) {
                _vboMemory += calculatePrimitiveSetMemory(primSet);
            }
        }
    }

    uint64_t calculateArrayMemory(osg::Array* array)
    {
        if (!array) return 0;
        return static_cast<uint64_t>(array->getTotalDataSize());
    }

    uint64_t calculatePrimitiveSetMemory(osg::PrimitiveSet* primSet)
    {
        if (!primSet) return 0;

        // DrawElements types have index arrays
        osg::DrawElementsUByte* deUByte = dynamic_cast<osg::DrawElementsUByte*>(primSet);
        if (deUByte) {
            return deUByte->size() * sizeof(GLubyte);
        }

        osg::DrawElementsUShort* deUShort = dynamic_cast<osg::DrawElementsUShort*>(primSet);
        if (deUShort) {
            return deUShort->size() * sizeof(GLushort);
        }

        osg::DrawElementsUInt* deUInt = dynamic_cast<osg::DrawElementsUInt*>(primSet);
        if (deUInt) {
            return deUInt->size() * sizeof(GLuint);
        }

        return 0;
    }

    uint64_t _textureMemory;
    uint64_t _vboMemory;
    uint32_t _textureCount;
    std::set<osg::Texture*> _processedTextures;  // Track processed textures to avoid duplicates
};

} // anonymous namespace

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
        stats.frameCount = static_cast<uint32_t>(frameStamp->getFrameNumber());
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

    // Traverse scene graph to collect detailed geometry statistics
    if (_sceneRoot.valid()) {
        StatsVisitor statsVisitor;
        // Need const_cast because accept() is not const in OSG
        const_cast<osg::Group*>(_sceneRoot.get())->accept(statsVisitor);

        stats.vertexCount = statsVisitor.getVertexCount();
        stats.triangleCount = statsVisitor.getTriangleCount();
        stats.drawCalls = statsVisitor.getDrawCalls();
        stats.geometryCount = statsVisitor.getGeometryCount();
        stats.geodeCount = statsVisitor.getGeodeCount();
        stats.nodeCount = statsVisitor.getNodeCount();
    }

    // ViewProvider count
    stats.viewProviderCount = static_cast<uint32_t>(_vpNodes.size());

    // Collect memory statistics (Task 2.3)
    if (_sceneRoot.valid()) {
        MemoryVisitor memVisitor;
        const_cast<osg::Group*>(_sceneRoot.get())->accept(memVisitor);

        stats.textureMemory = memVisitor.getTextureMemory();
        stats.vboMemory = memVisitor.getVBOMemory();
        stats.gpuMemoryUsed = memVisitor.getTotalMemory();

        // Note: gpuMemoryTotal would require platform-specific GPU queries
        // (e.g., GL_NVX_gpu_memory_info on NVIDIA, GL_ATI_meminfo on AMD)
        // For now, we only report the memory we're using
        stats.gpuMemoryTotal = 0;
    }

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

    // Create FPS display if it doesn't exist
    if (enabled && !_fpsCamera.valid()) {
        createFPSDisplay();
    }

    // Show or hide the FPS display
    showFPSDisplay(enabled);
}

bool OsgVerseViewer::isFPSEnabled() const
{
    return _fpsEnabled;
}

void OsgVerseViewer::createFPSDisplay()
{
    if (!_widget) {
        return;
    }

    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) {
        return;
    }

    int width = _widget->width();
    int height = _widget->height();

    // Create HUD camera for FPS display
    _fpsCamera = new osg::Camera();
    _fpsCamera->setName("FPS_HUD_Camera");
    _fpsCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, width, 0, height));
    _fpsCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _fpsCamera->setViewMatrix(osg::Matrix::identity());
    _fpsCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    _fpsCamera->setRenderOrder(osg::Camera::POST_RENDER, 100);
    _fpsCamera->setAllowEventFocus(false);

    // Create geode for text
    _fpsGeode = new osg::Geode();
    _fpsGeode->setName("FPS_Geode");

    // Disable lighting for HUD
    osg::StateSet* stateSet = _fpsGeode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    // Create FPS text
    _fpsText = new osgText::Text();
    _fpsText->setName("FPS_Text");
    _fpsText->setCharacterSize(16.0f);
    _fpsText->setFont("fonts/arial.ttf");  // Will use default if not found
    _fpsText->setColor(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));  // Yellow
    _fpsText->setPosition(osg::Vec3(10.0f, height - 25.0f, 0.0f));
    _fpsText->setAlignment(osgText::Text::LEFT_TOP);
    _fpsText->setText("FPS: --");
    _fpsText->setBackdropType(osgText::Text::DROP_SHADOW_BOTTOM_RIGHT);
    _fpsText->setBackdropColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.7f));

    // Create detailed stats text
    _statsText = new osgText::Text();
    _statsText->setName("Stats_Text");
    _statsText->setCharacterSize(14.0f);
    _statsText->setFont("fonts/arial.ttf");
    _statsText->setColor(osg::Vec4(0.9f, 0.9f, 0.9f, 1.0f));  // Light gray
    _statsText->setPosition(osg::Vec3(10.0f, height - 50.0f, 0.0f));
    _statsText->setAlignment(osgText::Text::LEFT_TOP);
    _statsText->setText("Vertices: --\nTriangles: --\nDraw Calls: --");
    _statsText->setBackdropType(osgText::Text::DROP_SHADOW_BOTTOM_RIGHT);
    _statsText->setBackdropColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.5f));

    // Add text to geode
    _fpsGeode->addDrawable(_fpsText.get());
    _fpsGeode->addDrawable(_statsText.get());

    // Add geode to camera
    _fpsCamera->addChild(_fpsGeode.get());

    // Add camera to scene
    if (_sceneRoot.valid()) {
        _sceneRoot->addChild(_fpsCamera.get());
    }

    Base::Console().log("OsgVerseViewer: FPS display created\n");
}

void OsgVerseViewer::updateFPSDisplay()
{
    if (!_fpsEnabled || !_fpsText.valid() || !_statsText.valid()) {
        return;
    }

    // Get current stats
    Gui::Render::RenderStats stats = getStats();

    // Update FPS text
    char fpsBuffer[64];
    if (stats.fps > 0) {
        snprintf(fpsBuffer, sizeof(fpsBuffer), "FPS: %.1f (%.2f ms)", stats.fps, stats.frameTime);
    } else {
        snprintf(fpsBuffer, sizeof(fpsBuffer), "FPS: --");
    }
    _fpsText->setText(fpsBuffer);

    // Update detailed stats text
    char statsBuffer[512];

    // Format memory sizes for readability
    auto formatMemory = [](uint64_t bytes) -> std::string {
        char buf[32];
        if (bytes >= 1024 * 1024 * 1024) {
            snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
        } else if (bytes >= 1024 * 1024) {
            snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
        } else if (bytes >= 1024) {
            snprintf(buf, sizeof(buf), "%.2f KB", bytes / 1024.0);
        } else {
            snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(bytes));
        }
        return std::string(buf);
    };

    std::string texMemStr = formatMemory(stats.textureMemory);
    std::string vboMemStr = formatMemory(stats.vboMemory);
    std::string totalMemStr = formatMemory(stats.gpuMemoryUsed);

    snprintf(statsBuffer, sizeof(statsBuffer),
             "Vertices: %u\n"
             "Triangles: %u\n"
             "Draw Calls: %u\n"
             "Nodes: %u\n"
             "ViewProviders: %u\n"
             "---\n"
             "Texture Mem: %s\n"
             "VBO Mem: %s\n"
             "Total GPU: %s",
             stats.vertexCount,
             stats.triangleCount,
             stats.drawCalls,
             stats.nodeCount,
             stats.viewProviderCount,
             texMemStr.c_str(),
             vboMemStr.c_str(),
             totalMemStr.c_str());
    _statsText->setText(statsBuffer);

    // Update camera projection if window size changed
    if (_widget && _fpsCamera.valid()) {
        int width = _widget->width();
        int height = _widget->height();
        _fpsCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, width, 0, height));

        // Update text positions
        _fpsText->setPosition(osg::Vec3(10.0f, height - 25.0f, 0.0f));
        _statsText->setPosition(osg::Vec3(10.0f, height - 50.0f, 0.0f));
    }
}

void OsgVerseViewer::showFPSDisplay(bool visible)
{
    if (!_fpsCamera.valid()) {
        return;
    }

    if (visible) {
        _fpsCamera->setNodeMask(~0u);  // Visible
        updateFPSDisplay();  // Update immediately
    } else {
        _fpsCamera->setNodeMask(0);    // Hidden
    }
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
// NaviCube
//===========================================================================

void OsgVerseViewer::setNaviCubeEnabled(bool enabled)
{
    _naviCubeEnabled = enabled;

    // Create NaviCube on demand
    if (enabled && !_naviCube) {
        _naviCube = std::make_unique<OsgVerseNaviCube>(this);
        if (_widget) {
            _naviCube->resize(_widget->width(), _widget->height());
        }
    }

    if (_naviCube) {
        _naviCube->setEnabled(enabled);
    }
}

bool OsgVerseViewer::isNaviCubeEnabled() const
{
    return _naviCubeEnabled;
}

void OsgVerseViewer::setNaviCubeCorner(int corner)
{
    // Create NaviCube if needed
    if (!_naviCube) {
        _naviCube = std::make_unique<OsgVerseNaviCube>(this);
        if (_widget) {
            _naviCube->resize(_widget->width(), _widget->height());
        }
    }

    // Convert int to Corner enum
    OsgVerseNaviCube::Corner c;
    switch (corner) {
        case 0: c = OsgVerseNaviCube::CornerTopLeft; break;
        case 1: c = OsgVerseNaviCube::CornerTopRight; break;
        case 2: c = OsgVerseNaviCube::CornerBottomLeft; break;
        case 3: c = OsgVerseNaviCube::CornerBottomRight; break;
        default: c = OsgVerseNaviCube::CornerTopRight; break;
    }

    _naviCube->setCorner(c);
}

int OsgVerseViewer::getNaviCubeCorner() const
{
    if (_naviCube) {
        return static_cast<int>(_naviCube->getCorner());
    }
    return 1;  // Default: TopRight
}

OsgVerseNaviCube* OsgVerseViewer::getNaviCube()
{
    // Create NaviCube on demand
    if (!_naviCube) {
        _naviCube = std::make_unique<OsgVerseNaviCube>(this);
        if (_widget) {
            _naviCube->resize(_widget->width(), _widget->height());
        }
    }
    return _naviCube.get();
}

//===========================================================================
// Private methods
//===========================================================================

osg::ref_ptr<osg::Node> OsgVerseViewer::createNodeForViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return nullptr;
    }

    // Check if this is a ViewProviderDocumentObject
    auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);
    if (!vpDoc) {
        return nullptr;
    }

    App::DocumentObject* obj = vpDoc->getObject();
    if (!obj) {
        return nullptr;
    }

    // Check if this is a Part::Feature
    if (!obj->isDerivedFrom(Part::Feature::getClassTypeId())) {
        return nullptr;
    }

    // Extract TopoDS_Shape and convert to OSG geometry
    try {
        Part::TopoShape topoShape = Part::Feature::getTopoShape(
            obj,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );

        const TopoDS_Shape& shape = topoShape.getShape();

        if (shape.IsNull()) {
            return nullptr;
        }

        // Convert using GeometryConverter
        GeometryConverter::ConversionOptions options;
        options.deflection = 0.1;
        options.angle = 0.5;
        options.computeNormals = true;

        GeometryConverter::ConversionStats stats;
        osg::ref_ptr<osg::Geode> geode = GeometryConverter::convertShape(shape, options, &stats);

        if (!geode) {
            Base::Console().warning("OsgVerseViewer: GeometryConverter returned null (shape may have no faces)\n");
            return nullptr;
        }

        // Extract color and transparency from ViewProvider
        Base::Color shapeColor(0.8f, 0.8f, 0.8f);  // Default gray
        float transparency = 1.0f;  // 1.0 = fully opaque

        // Try to get ShapeAppearance property (FreeCAD 1.0+)
        // ShapeAppearance is a PropertyMaterialList (tuple of Material objects)
        App::Property* appearanceProp = vp->getPropertyByName("ShapeAppearance");
        if (appearanceProp) {
            // ShapeAppearance is a PropertyMaterialList - get first material
            if (appearanceProp->isDerivedFrom(App::PropertyMaterialList::getClassTypeId())) {
                App::PropertyMaterialList* propMatList = static_cast<App::PropertyMaterialList*>(appearanceProp);
                const std::vector<App::Material>& materials = propMatList->getValues();
                if (!materials.empty()) {
                    const App::Material& mat = materials[0];
                    shapeColor = mat.diffuseColor;
                    transparency = 1.0f - mat.transparency;
                }
            }
            // Also try PropertyMaterial (single material)
            else if (appearanceProp->isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
                App::PropertyMaterial* propMat = static_cast<App::PropertyMaterial*>(appearanceProp);
                const App::Material& mat = propMat->getValue();
                shapeColor = mat.diffuseColor;
                transparency = 1.0f - mat.transparency;
            }
        }

        // Fallback: Try legacy ShapeColor property
        if (!appearanceProp) {
            App::Property* colorProp = vp->getPropertyByName("ShapeColor");
            if (colorProp && colorProp->isDerivedFrom(App::PropertyColor::getClassTypeId())) {
                App::PropertyColor* propColor = static_cast<App::PropertyColor*>(colorProp);
                shapeColor = propColor->getValue();
            }

            // Try to get Transparency property
            App::Property* transProp = vp->getPropertyByName("Transparency");
            if (transProp && transProp->isDerivedFrom(App::PropertyPercent::getClassTypeId())) {
                App::PropertyPercent* propTrans = static_cast<App::PropertyPercent*>(transProp);
                int transPercent = propTrans->getValue();
                transparency = 1.0f - (static_cast<float>(transPercent) / 100.0f);
            }
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

    // Enable lighting on this node
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);

    // Create material for lighting
    osg::ref_ptr<osg::Material> material = new osg::Material();
    osg::Vec4 diffuse(color.r, color.g, color.b, alpha);
    osg::Vec4 ambient(color.r * 0.4f, color.g * 0.4f, color.b * 0.4f, alpha);
    material->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
    material->setAmbient(osg::Material::FRONT_AND_BACK, ambient);
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));
    material->setShininess(osg::Material::FRONT_AND_BACK, 25.0f);
    material->setColorMode(osg::Material::OFF);
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // Also set vertex colors as fallback for non-lit rendering modes
    osg::Geode* geode = dynamic_cast<osg::Geode*>(node);
    if (geode) {
        osg::Vec4 vertexColor(color.r, color.g, color.b, alpha);
        for (unsigned int i = 0; i < geode->getNumDrawables(); ++i) {
            osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode->getDrawable(i));
            if (geom) {
                osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
                colors->push_back(vertexColor);
                geom->setColorArray(colors.get(), osg::Array::BIND_OVERALL);
            }
            osg::ShapeDrawable* shapeDrawable = dynamic_cast<osg::ShapeDrawable*>(geode->getDrawable(i));
            if (shapeDrawable) {
                shapeDrawable->setColor(vertexColor);
            }
        }
    }

    // Enable alpha blending for transparent materials
    if (alpha < 1.0f) {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc(
            osg::BlendFunc::SRC_ALPHA,
            osg::BlendFunc::ONE_MINUS_SRC_ALPHA
        );
        stateSet->setAttributeAndModes(blendFunc.get(), osg::StateAttribute::ON);
        stateSet->setRenderBinDetails(10, "DepthSortedBin");
    }
    else {
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

//===========================================================================
// 后处理效果 (Post-Processing Effects)
//===========================================================================

void OsgVerseViewer::setSSAOEnabled(bool enabled)
{
    if (!_postProcessManager) {
        // 延迟初始化后处理管理器
        _postProcessManager = std::make_unique<PostProcessManager>();
        if (_widget) {
            int w = _widget->width();
            int h = _widget->height();
            // Note: Full initialization requires scene camera integration
            // This is a simplified setup for now
            Base::Console().log("OsgVerseViewer: PostProcessManager created (%dx%d)\n", w, h);
        }
    }

    _postProcessManager->setSSAOEnabled(enabled);

    if (enabled) {
        Base::Console().log("OsgVerseViewer: SSAO enabled\n");
    }
    else {
        Base::Console().log("OsgVerseViewer: SSAO disabled\n");
    }

    render();
}

bool OsgVerseViewer::isSSAOEnabled() const
{
    return _postProcessManager && _postProcessManager->isSSAOEnabled();
}

void OsgVerseViewer::setSSAORadius(float radius)
{
    if (_postProcessManager && _postProcessManager->getSSAO()) {
        _postProcessManager->getSSAO()->setRadius(radius);
        render();
    }
}

float OsgVerseViewer::getSSAORadius() const
{
    if (_postProcessManager && _postProcessManager->getSSAO()) {
        return _postProcessManager->getSSAO()->getRadius();
    }
    return 0.5f;  // default
}

void OsgVerseViewer::setSSAOIntensity(float intensity)
{
    if (_postProcessManager && _postProcessManager->getSSAO()) {
        _postProcessManager->getSSAO()->setIntensity(intensity);
        render();
    }
}

float OsgVerseViewer::getSSAOIntensity() const
{
    if (_postProcessManager && _postProcessManager->getSSAO()) {
        return _postProcessManager->getSSAO()->getIntensity();
    }
    return 1.0f;  // default
}

void OsgVerseViewer::setBloomEnabled(bool enabled)
{
    if (!_postProcessManager) {
        _postProcessManager = std::make_unique<PostProcessManager>();
        if (_widget) {
            int w = _widget->width();
            int h = _widget->height();
            Base::Console().log("OsgVerseViewer: PostProcessManager created (%dx%d)\n", w, h);
        }
    }

    _postProcessManager->setBloomEnabled(enabled);

    if (enabled) {
        Base::Console().log("OsgVerseViewer: Bloom enabled\n");
    }
    else {
        Base::Console().log("OsgVerseViewer: Bloom disabled\n");
    }

    render();
}

bool OsgVerseViewer::isBloomEnabled() const
{
    return _postProcessManager && _postProcessManager->isBloomEnabled();
}

void OsgVerseViewer::setBloomThreshold(float threshold)
{
    if (_postProcessManager && _postProcessManager->getBloom()) {
        _postProcessManager->getBloom()->setThreshold(threshold);
        render();
    }
}

float OsgVerseViewer::getBloomThreshold() const
{
    if (_postProcessManager && _postProcessManager->getBloom()) {
        return _postProcessManager->getBloom()->getThreshold();
    }
    return 1.0f;  // default
}

void OsgVerseViewer::setBloomIntensity(float intensity)
{
    if (_postProcessManager && _postProcessManager->getBloom()) {
        _postProcessManager->getBloom()->setIntensity(intensity);
        render();
    }
}

float OsgVerseViewer::getBloomIntensity() const
{
    if (_postProcessManager && _postProcessManager->getBloom()) {
        return _postProcessManager->getBloom()->getIntensity();
    }
    return 1.0f;  // default
}
