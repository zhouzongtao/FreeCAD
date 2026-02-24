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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QImage>
#include <QPainter>
#include <QMenu>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <fstream>
#endif

#include "OsgVerseViewer.h"
#include "OsgVerseLight.h"
#include "OsgVerseNaviCube.h"
#include "OsgVerseShaderManager.h"
#include "OsgVersePickingService.h"
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osgGA/TrackballManipulator>
#include <osgGA/GUIEventAdapter>
#include <osg/Camera>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/ComputeBoundsVisitor>
#include <osgDB/WriteFile>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/Uniform>
#include <Base/Console.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <App/DocumentObject.h>
#include <App/Document.h>
#include <App/Property.h>
#include <App/PropertyStandard.h>
#include <App/Application.h>

// Shape geometry display via App-level ComplexGeoData interface
// (No direct Part module dependency — uses virtual dispatch at runtime)
#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>
#include <App/ComplexGeoData.h>
// OSG arrays for geometry creation
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Geometry>
#include <osg/Program>
#include <osg/Shader>
#include <osg/LineWidth>
#include <osg/ClipPlane>
#include <osg/ClipNode>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <Base/ViewProj.h>
#include <Base/BoundBox.h>

using namespace Gui::Render;

//===========================================================================
// OsgVerseViewer Implementation
//===========================================================================

OsgVerseViewer::OsgVerseViewer()
{
    // 延迟初始化模式：不在构造函数中做任何初始化
    // Lazy initialization: do not initialize anything in constructor
    // 所有初始化都延迟到第一次使用时（ensureInitialized）
    // All initialization is deferred until first use (ensureInitialized)
    
    Base::Console().log("OsgVerseViewer: Constructor called (lazy initialization mode)\n");
}

OsgVerseViewer::~OsgVerseViewer()
{
    // Shutdown engine FIRST (while viewer is still alive) to safely release scene root
    if (_engine) {
        _engine->shutdown();
    }

    // Clear all ViewProvider mappings
    _viewProviders.clear();
    _nodeToVPMap.clear();
    _vpToNodeMap.clear();

    // Stop viewer
    if (_viewer) {
        _viewer->setDone(true);
    }

    // Delete widget BEFORE the osg viewer — ViewerWidget's destructor accesses _viewer
    if (_widget) {
        _widget->setParent(nullptr);
        delete _widget;
        _widget = nullptr;
    }

    // Delete the osg viewer
    if (_viewer) {
        delete _viewer;
        _viewer = nullptr;
    }

    // Reset engine (already shut down, destructor is safe now)
    _engine.reset();

    // Release graphics window
    _graphicsWindow = nullptr;
}

//-----------------------------------------------------------------------
// Scene Graph Management
//-----------------------------------------------------------------------

void OsgVerseViewer::setSceneRoot(RenderNode::Ptr root)
{
    ensureInitialized();
    _sceneRoot = root;
    if (_engine) {
        _engine->setSceneRoot(root);
    }
}

RenderNode::Ptr OsgVerseViewer::getSceneRoot() const
{
    return _sceneRoot;
}

void OsgVerseViewer::updateScene()
{
    _engine->updateScene();
    if (_widget) {
        _widget->update();
    }
}

//-----------------------------------------------------------------------
// Rendering Control
//-----------------------------------------------------------------------

void OsgVerseViewer::render()
{
    ensureInitialized();
    if (_viewer) {
        // 更新相机动画（如果正在动画中）
        // Update camera animation (if animating)
        if (_animationEnabled && !_animationComplete && _viewer->getFrameStamp()) {
            updateCameraAnimation(_viewer->getFrameStamp()->getReferenceTime());
        }
        // Update axis cross rotation to match main camera
        if (_axisCrossEnabled) {
            updateAxisCross();
        }
        // Update spin animation
        if (_isSpinning) {
            updateSpinAnimation();
        }
        // NaviCube is now drawn in ViewerWidget::paintGL() where OpenGL context is active
        // NaviCube现在在ViewerWidget::paintGL()中绘制，那里OpenGL上下文是激活的
        if (_widget) {
            _widget->update();  // Triggers paintGL()
        }
    }
}

void OsgVerseViewer::setRenderMode(RenderMode mode)
{
    _renderMode = mode;
    _engine->setRenderMode(mode);
}

RenderMode OsgVerseViewer::getRenderMode() const
{
    return _renderMode;
}

void OsgVerseViewer::setBackgroundColor(const Color& color)
{
    _backgroundColor = color;
    _engine->setBackgroundColor(color);

    if (_viewer) {
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            camera->setClearColor(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
}

Color OsgVerseViewer::getBackgroundColor() const
{
    return _backgroundColor;
}

//-----------------------------------------------------------------------
// Camera Control
//-----------------------------------------------------------------------

void OsgVerseViewer::setCamera(const CameraParams& params)
{
    _cameraParams = params;

    if (!_viewer) return;

    // 设置投影矩阵 / Set projection matrix
    osg::Camera* camera = _viewer->getCamera();
    if (camera) {
        camera->setProjectionMatrixAsPerspective(
            params.fieldOfView, params.aspectRatio, params.nearPlane, params.farPlane);
    }

    // 核心策略：通过Manipulator设置Camera，不要直接设置Camera的ViewMatrix
    // Manipulator是Camera的唯一控制者，这样就不会有"打架"问题
    // Core strategy: Set camera THROUGH the Manipulator, never set Camera ViewMatrix directly.
    // Manipulator is the sole controller of Camera, so there is no "fighting" problem.
    osgGA::TrackballManipulator* manipulator =
        dynamic_cast<osgGA::TrackballManipulator*>(_viewer->getCameraManipulator());

    if (manipulator) {
        osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
        osg::Vec3d center(params.target.x, params.target.y, params.target.z);
        osg::Vec3d up(params.upVector.x, params.upVector.y, params.upVector.z);

        // 直接设置Manipulator的内部状态
        // Directly set Manipulator's internal state
        manipulator->setCenter(center);

        osg::Vec3d direction = eye - center;
        double distance = direction.length();
        if (distance < 1e-6) distance = 1.0;
        manipulator->setDistance(distance);

        // 从eye/center/up计算旋转四元数
        // Calculate rotation quaternion from eye/center/up
        osg::Matrixd viewMatrix;
        viewMatrix.makeLookAt(eye, center, up);
        osg::Matrixd rotMatrix = viewMatrix;
        rotMatrix.setTrans(0, 0, 0);
        manipulator->setRotation(rotMatrix.getRotate().inverse());
    }
}

CameraParams OsgVerseViewer::getCamera() const
{
    // 始终从Manipulator读取最新状态（Manipulator是Camera的唯一控制者）
    // Always read the latest state from Manipulator (Manipulator is the sole controller of Camera)
    if (_viewer) {
        osgGA::TrackballManipulator* manipulator =
            dynamic_cast<osgGA::TrackballManipulator*>(_viewer->getCameraManipulator());

        if (manipulator) {
            osg::Vec3d center = manipulator->getCenter();
            double distance = manipulator->getDistance();
            osg::Quat rotation = manipulator->getRotation();

            // 从Manipulator内部状态计算eye和up
            // Calculate eye and up from Manipulator internal state
            // TrackballManipulator的默认视线方向是(0, 0, -distance)，然后应用rotation
            osg::Vec3d defaultDir(0.0, 0.0, distance);
            osg::Vec3d offset = rotation * defaultDir;
            osg::Vec3d eye = center + offset;

            osg::Vec3d defaultUp(0.0, 1.0, 0.0);
            osg::Vec3d up = rotation * defaultUp;

            CameraParams& params = const_cast<CameraParams&>(_cameraParams);
            params.position.x = static_cast<float>(eye.x());
            params.position.y = static_cast<float>(eye.y());
            params.position.z = static_cast<float>(eye.z());
            params.target.x = static_cast<float>(center.x());
            params.target.y = static_cast<float>(center.y());
            params.target.z = static_cast<float>(center.z());
            params.upVector.x = static_cast<float>(up.x());
            params.upVector.y = static_cast<float>(up.y());
            params.upVector.z = static_cast<float>(up.z());
        }

        // 获取投影参数 / Get projection parameters
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            CameraParams& projParams = const_cast<CameraParams&>(_cameraParams);
            double fovy, aspectRatio, zNear, zFar;
            camera->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
            projParams.fieldOfView = static_cast<float>(fovy);
            projParams.aspectRatio = static_cast<float>(aspectRatio);
            projParams.nearPlane = static_cast<float>(zNear);
            projParams.farPlane = static_cast<float>(zFar);
        }
    }

    return _cameraParams;
}

void OsgVerseViewer::resetCamera()
{
    if (_viewer) {
        osgGA::CameraManipulator* manipulator = _viewer->getCameraManipulator();
        if (manipulator) {
            manipulator->home(0.0);
        }
    }
}

void OsgVerseViewer::fitAll()
{
    if (!_viewer || !_engine) {
        return;
    }

    osg::Group* sceneRoot = _engine->getOsgSceneRoot();
    if (!sceneRoot || sceneRoot->getNumChildren() == 0) {
        return;
    }

    // Compute actual scene bounding box
    osg::ComputeBoundsVisitor cbv;
    sceneRoot->accept(cbv);
    osg::BoundingBox bb = cbv.getBoundingBox();

    if (!bb.valid()) {
        // Fallback to home()
        _viewer->home();
        return;
    }

    osg::BoundingSphere bs;
    bs.expandBy(bb);
    float radius = bs.radius();

    if (radius < 1e-6f) {
        _viewer->home();
        return;
    }

    // Position camera via TrackballManipulator
    osgGA::TrackballManipulator* manip =
        dynamic_cast<osgGA::TrackballManipulator*>(_viewer->getCameraManipulator());
    if (manip) {
        manip->setCenter(bs.center());
        manip->setDistance(static_cast<double>(radius) * 2.5);
        Base::Console().message("OsgVerse: fitAll -> center=(%.1f,%.1f,%.1f) dist=%.1f\n",
            bs.center().x(), bs.center().y(), bs.center().z(), radius * 2.5);
    } else {
        _viewer->home();
    }
}

void OsgVerseViewer::fitSelection()
{
    Base::Console().log("OsgVerseViewer::fitSelection: Fitting to selection...\n");

    if (!_engine || !_viewer) {
        Base::Console().warning("OsgVerseViewer::fitSelection: Engine or viewer not initialized\n");
        return;
    }

    // 获取场景根节点
    // Get scene root node
    osg::Group* sceneRoot = _engine->getOsgSceneRoot();
    if (!sceneRoot) {
        Base::Console().warning("OsgVerseViewer::fitSelection: Scene root is null\n");
        return;
    }

    // 查找选中的节点（带有 Selection 名称或特殊标记的节点）
    // Find selected nodes (nodes with Selection name or special markers)

    // 遍历场景查找选中物体
    // Traverse scene to find selected objects
    class FindSelectedNodes : public osg::NodeVisitor {
    public:
        FindSelectedNodes() : osg::NodeVisitor(), _hasSelection(false) {
            setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
        }

        void apply(osg::Node& node) override {
            // 检查是否是 OsgVerseSelectionRoot 或带有 "Selected" 标记的节点
            // Check if it's OsgVerseSelectionRoot or node with "Selected" marker
            if (node.asGroup()) {
                std::string name = node.getName();
                if (name.find("Selection") != std::string::npos ||
                    name.find("Selected") != std::string::npos) {
                    osg::BoundingBox* bbox = dynamic_cast<osg::BoundingBox*>(&node);
                    if (bbox) {
                        // 获取边界框
                        // Get bounding box
                        selectedBounds.expandBy(bbox->_min);
                        selectedBounds.expandBy(bbox->_max);
                        _hasSelection = true;
                    } else {
                        // 对于普通节点，计算其包围盒
                        // For normal nodes, calculate their bounding box
                        osg::ComputeBoundsVisitor cb;
                        node.accept(cb);
                        if (cb.getBoundingBox().valid()) {
                            selectedBounds.expandBy(cb.getBoundingBox()._min);
                            selectedBounds.expandBy(cb.getBoundingBox()._max);
                            _hasSelection = true;
                        }
                    }
                }
            }
            traverse(node);
        }

        bool hasSelection() const { return _hasSelection; }
        osg::BoundingBox getBounds() const { return selectedBounds; }

    private:
        osg::BoundingBox selectedBounds;
        bool _hasSelection;
    };

    FindSelectedNodes finder;
    sceneRoot->accept(finder);

    if (finder.hasSelection()) {
        osg::BoundingBox bounds = finder.getBounds();

        if (bounds.valid()) {
            // 计算包围盒的中心和大小
            // Calculate bounding box center and size
            osg::Vec3 center = bounds.center();
            osg::Vec3 size = bounds._max - bounds._min;

            // 计算相机应该放置的位置
            // Calculate where camera should be positioned
            // 相机位置 = 中心 + 尺寸向量 * 缩放因子 + 偏移向量
            // Camera position = center + size vector * scale factor + offset vector
            float scaleFactor = 2.0f;  // 缩放因子，决定相机距离
            osg::Vec3 offset(0.0f, -1.0f, 0.5f);  // 从斜上方观察

            osg::Vec3 eye = center + osg::Vec3(size.x() * offset.x(),
                                                   size.y() * offset.y(),
                                                   size.z() * offset.z()) * scaleFactor;
            osg::Vec3 target = center;
            osg::Vec3 up(0.0f, 0.0f, 1.0f);

            // 设置相机
            // Set camera
            osg::Camera* camera = _viewer->getCamera();
            if (camera) {
                camera->setViewMatrixAsLookAt(eye, target, up);

                // 调整视场角以适应选中物体的大小
                // Adjust FOV to fit selected object size
                float maxDimension = std::max({size.x(), size.y(), size.z()});
                float fov = std::min(60.0f, std::max(30.0f, maxDimension * 5.0f));
                camera->setProjectionMatrixAsPerspective(fov,
                                                                 _cameraParams.aspectRatio,
                                                                 _cameraParams.nearPlane,
                                                                 _cameraParams.farPlane);

                Base::Console().log("OsgVerseViewer::fitSelection: Fitted to selection\n");
                Base::Console().log("OsgVerseViewer::fitSelection: Center (%.2f, %.2f, %.2f)\n",
                                   center.x(), center.y(), center.z());
            }
        } else {
            Base::Console().warning("OsgVerseViewer::fitSelection: Invalid bounding box\n");
            // 降级到 fitAll
            // Fallback to fitAll
            fitAll();
        }
    } else {
        Base::Console().log("OsgVerseViewer::fitSelection: No selection found, fitting to all\n");
        // 没有选中物体，降级到 fitAll
        // No selection found, fallback to fitAll
        fitAll();
    }
}

//-----------------------------------------------------------------------
// Light Control
//-----------------------------------------------------------------------

void OsgVerseViewer::setAmbientIntensity(float intensity)
{
    _ambientIntensity = intensity;

    // Update scene lighting
    if (_engine && _engine->getOsgSceneRoot()) {
        osg::Group* sceneRoot = _engine->getOsgSceneRoot();

        // Find and update SunLight (first LightSource)
        for (unsigned int i = 0; i < sceneRoot->getNumChildren(); ++i) {
            osg::Node* node = sceneRoot->getChild(i);
            osg::LightSource* ls = dynamic_cast<osg::LightSource*>(node);
            if (ls && ls->getLight() && ls->getName() == "SunLight") {
                // Get current ambient color from light manager
                auto& lightManager = OsgVerseLightManager::instance();
                Color ambientColor = lightManager.getAmbientColor();

                // Update ambient light with new intensity
                ls->getLight()->setAmbient(osg::Vec4(ambientColor.r * _ambientIntensity,
                                                       ambientColor.g * _ambientIntensity,
                                                       ambientColor.b * _ambientIntensity, 1.0f));
                break;
            }
        }
    }
}

float OsgVerseViewer::getAmbientIntensity() const
{
    return _ambientIntensity;
}

void OsgVerseViewer::setBacklightEnabled(bool enabled)
{
    _backlightEnabled = enabled;

    // Update backlight
    if (_engine && _engine->getOsgSceneRoot()) {
        osg::Group* sceneRoot = _engine->getOsgSceneRoot();

        if (_backlightEnabled) {
            // Check if BackLight already exists
            bool hasBackLight = false;
            for (unsigned int i = 0; i < sceneRoot->getNumChildren(); ++i) {
                osg::Node* node = sceneRoot->getChild(i);
                if (node && dynamic_cast<osg::LightSource*>(node) && node->getName() == "BackLight") {
                    hasBackLight = true;
                    // Enable GL_LIGHT1
                    osg::StateSet* stateSet = sceneRoot->getOrCreateStateSet();
                    stateSet->setMode(GL_LIGHT1, osg::StateAttribute::ON);
                    break;
                }
            }

            // Create BackLight if it doesn't exist
            if (!hasBackLight) {
                osg::Light* backLight = new osg::Light();
                backLight->setLightNum(1);  // GL_LIGHT1

                osg::Vec3 backDir(0.0f, 1.0f, 0.0f);
                backLight->setPosition(osg::Vec4(backDir.x(), backDir.y(), backDir.z(), 0.0f));

                float backIntensity = 0.3f;
                backLight->setDiffuse(osg::Vec4(backIntensity, backIntensity, backIntensity, 1.0f));
                backLight->setSpecular(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
                backLight->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

                osg::LightSource* backLightSource = new osg::LightSource();
                backLightSource->setLight(backLight);
                backLightSource->setLocalStateSetModes(osg::StateAttribute::ON);
                backLightSource->setName("BackLight");

                sceneRoot->addChild(backLightSource);

                // Enable GL_LIGHT1 in StateSet
                osg::StateSet* stateSet = sceneRoot->getOrCreateStateSet();
                stateSet->setMode(GL_LIGHT1, osg::StateAttribute::ON);
            }
        } else {
            // Remove BackLight if it exists
            for (int i = sceneRoot->getNumChildren() - 1; i >= 0; --i) {
                osg::Node* node = sceneRoot->getChild(i);
                if (node && dynamic_cast<osg::LightSource*>(node) && node->getName() == "BackLight") {
                    sceneRoot->removeChild(i);
                    // Disable GL_LIGHT1 in StateSet
                    osg::StateSet* stateSet = sceneRoot->getOrCreateStateSet();
                    stateSet->setMode(GL_LIGHT1, osg::StateAttribute::OFF);
                    break;
                }
            }
        }
    }
}

bool OsgVerseViewer::isBacklightEnabled() const
{
    return _backlightEnabled;
}

//-----------------------------------------------------------------------
// Window Integration
//-----------------------------------------------------------------------

QWidget* OsgVerseViewer::getWidget() const
{
    // 确保已初始化
    // Ensure initialized
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();
    
    // Lazy initialization of widget
    if (!_widget && _initialized) {
        // Cast away const for lazy initialization
        const_cast<OsgVerseViewer*>(this)->initializeWidget();
    }
    return _widget;
}

void OsgVerseViewer::resize(int width, int height)
{
    if (_widget) {
        _widget->resize(width, height);
    }
}

void OsgVerseViewer::onResize(int width, int height)
{
    // width/height here are logical pixels; convert to physical for OSG
    float dpr = 1.0f;
    if (_widget) {
        dpr = _widget->devicePixelRatio();
    }
    const int physW = static_cast<int>(width * dpr);
    const int physH = static_cast<int>(height * dpr);

    if (_graphicsWindow.valid()) {
        _graphicsWindow->resized(0, 0, physW, physH);
    }

    if (_viewer) {
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            camera->setViewport(0, 0, physW, physH);
            double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
            camera->setProjectionMatrixAsPerspective(
                _cameraParams.fieldOfView,
                aspectRatio,
                _cameraParams.nearPlane,
                _cameraParams.farPlane
            );
        }
    }

    // Update axis cross viewport (100x100 physical pixels, bottom-left)
    if (_axisCrossCamera) {
        int axisSize = static_cast<int>(100 * dpr);
        int margin = static_cast<int>(10 * dpr);
        _axisCrossCamera->setViewport(margin, margin, axisSize, axisSize);
    }
}

//-----------------------------------------------------------------------
// Screenshot Functionality
//-----------------------------------------------------------------------

QImage OsgVerseViewer::grabImage(int width, int height)
{
    if (!_viewer) {
        return QImage();
    }

    // If no custom size requested, just grab the widget framebuffer
    if (width <= 0 || height <= 0) {
        if (_widget) {
            return _widget->grabFramebuffer();
        }
        return QImage();
    }

    // For custom resolution, use RTT (render-to-texture) offscreen rendering
    osg::Camera* mainCamera = _viewer->getCamera();
    if (!mainCamera || !_viewer->getSceneData()) {
        return QImage();
    }

    // Create RTT camera that copies the main camera's view/projection
    osg::ref_ptr<osg::Camera> rttCamera = new osg::Camera();
    rttCamera->setClearColor(mainCamera->getClearColor());
    rttCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    rttCamera->setViewport(0, 0, width, height);
    rttCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    rttCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    // Copy view and projection matrices from main camera
    rttCamera->setViewMatrix(mainCamera->getViewMatrix());
    rttCamera->setProjectionMatrix(mainCamera->getProjectionMatrix());

    // Fix aspect ratio for the requested resolution
    double fovy, aspectRatio, zNear, zFar;
    if (mainCamera->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar)) {
        double newAspect = static_cast<double>(width) / static_cast<double>(height);
        rttCamera->setProjectionMatrixAsPerspective(fovy, newAspect, zNear, zFar);
    }

    // Create color texture for attachment
    osg::ref_ptr<osg::Texture2D> colorTex = new osg::Texture2D();
    colorTex->setTextureSize(width, height);
    colorTex->setInternalFormat(GL_RGBA);
    colorTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    colorTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    rttCamera->attach(osg::Camera::COLOR_BUFFER, colorTex.get());

    // Create depth renderbuffer
    rttCamera->attach(osg::Camera::DEPTH_BUFFER, GL_DEPTH_COMPONENT24);

    // Create image to read back pixels
    osg::ref_ptr<osg::Image> osgImage = new osg::Image();
    osgImage->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    rttCamera->attach(osg::Camera::COLOR_BUFFER, osgImage.get());

    // Add scene as child of RTT camera
    rttCamera->addChild(_viewer->getSceneData());

    // Temporarily add RTT camera to the scene, render one frame, then remove
    auto* sceneRoot = dynamic_cast<osg::Group*>(_viewer->getSceneData());
    if (!sceneRoot) {
        return QImage();
    }

    sceneRoot->addChild(rttCamera.get());
    _viewer->frame();
    sceneRoot->removeChild(rttCamera.get());

    // Convert osg::Image to QImage (OSG stores bottom-up, Qt stores top-down)
    QImage result(width, height, QImage::Format_RGBA8888);
    const unsigned char* data = osgImage->data();
    for (int row = 0; row < height; ++row) {
        int srcRow = height - 1 - row;  // Flip vertically
        memcpy(result.scanLine(row), data + srcRow * width * 4, width * 4);
    }

    return result;
}

bool OsgVerseViewer::saveScreenshot(const QString& filename, int width, int height)
{
    QImage image = grabImage(width, height);
    if (image.isNull()) {
        return false;
    }

    return image.save(filename);
}

//-----------------------------------------------------------------------
// Statistics
//-----------------------------------------------------------------------

RenderStats OsgVerseViewer::getStats() const
{
    return _engine->getStats();
}

void OsgVerseViewer::resetStats() const
{
    _engine->resetStats();
}

//-----------------------------------------------------------------------
// Compatibility Interface
//-----------------------------------------------------------------------

void* OsgVerseViewer::getNativePointer() const
{
    return _viewer;
}

void OsgVerseViewer::setEventCallback(EventCallback callback)
{
    _eventCallback = std::move(callback);
}

//-----------------------------------------------------------------------
// Viewer Modes
//-----------------------------------------------------------------------

RenderViewer::ViewerMode OsgVerseViewer::getViewerMode() const
{
    return _viewerMode;
}

void OsgVerseViewer::setViewerMode(ViewerMode mode)
{
    _viewerMode = mode;
}

bool OsgVerseViewer::isAnimating() const
{
    return _isAnimating;
}

void OsgVerseViewer::stopAnimation()
{
    _isAnimating = false;
}

//-----------------------------------------------------------------------
// OsgVerse-specific Interface
//-----------------------------------------------------------------------

void OsgVerseViewer::setCameraManipulator(void* manipulator)
{
    if (_viewer && manipulator) {
        _viewer->setCameraManipulator(static_cast<osgGA::CameraManipulator*>(manipulator));
    }
}

void OsgVerseViewer::setStatsEnabled(bool enabled)
{
    _statsEnabled = enabled;
    if (_viewer) {
        _viewer->getViewerStats()->collectStats("frame_rate", enabled);
    }
}

//-----------------------------------------------------------------------
// Private Methods
//-----------------------------------------------------------------------

void OsgVerseViewer::initializeViewer()
{
    Base::Console().log("OsgVerseViewer::initializeViewer: Creating viewer...\n");
    
    // Create graphics window embedded (like OsgVerse qt_viewer example)
    _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(0, 0, 800, 600);
    
    // Create viewer
    _viewer = new osgViewer::Viewer();
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // Set camera with graphics context
    osg::Camera* camera = _viewer->getCamera();
    camera->setGraphicsContext(_graphicsWindow.get());
    camera->setViewport(0, 0, 800, 600);
    camera->setClearColor(osg::Vec4(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, _backgroundColor.a));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set camera manipulator
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator());
    _viewer->setKeyEventSetsDone(0);  // Don't exit on Escape

    // Connect engine to viewer
    _engine->setViewer(_viewer);
    
    Base::Console().log("OsgVerseViewer::initializeViewer: Viewer created successfully\n");
}

void OsgVerseViewer::initializeWidget()
{
    // Only initialize once
    if (_widget) {
        return;
    }

    Base::Console().log("OsgVerseViewer::initializeWidget: Creating widget...\n");
    _widget = new ViewerWidget(this, _viewer, _graphicsWindow.get());
    Base::Console().log("OsgVerseViewer::initializeWidget: Widget created successfully\n");
}

void OsgVerseViewer::setupDefaultCamera()
{
    if (!_viewer) {
        return;
    }

    osg::Camera* camera = _viewer->getCamera();
    if (!camera) {
        return;
    }

    _cameraParams.position = Vec3f(0.0f, -8.0f, 12.0f);
    _cameraParams.target = Vec3f(0.0f, 0.0f, 0.0f);
    _cameraParams.upVector = Vec3f(0.0f, 1.0f, 0.0f);
    _cameraParams.fieldOfView = 60.0f;
    _cameraParams.aspectRatio = 1.333f;
    _cameraParams.nearPlane = 0.1f;
    _cameraParams.farPlane = 100000.0f;

    camera->setProjectionMatrixAsPerspective(
        _cameraParams.fieldOfView,
        _cameraParams.aspectRatio,
        _cameraParams.nearPlane,
        _cameraParams.farPlane
    );

    // Let OSG auto-compute near/far planes based on scene bounding volumes each frame.
    // This is critical for large scenes (e.g. ArchDetail with radius ~124000 units).
    camera->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
    camera->setNearFarRatio(0.00005);  // Allow very large depth range

    camera->setClearColor(osg::Vec4(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, _backgroundColor.a));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OsgVerseViewer::setupDefaultLighting()
{
    Base::Console().log("OsgVerseViewer::setupDefaultLighting: Setting up default lighting...\n");

    // 确保引擎和查看器已初始化
    // Ensure engine and viewer are initialized
    if (!_engine || !_viewer) {
        Base::Console().warning("OsgVerseViewer::setupDefaultLighting: Engine or viewer not initialized\n");
        return;
    }

    // 获取 OSG 场景根节点
    // Get OSG scene root node
    osg::Group* sceneRoot = _engine->getOsgSceneRoot();
    if (!sceneRoot) {
        Base::Console().warning("OsgVerseViewer::setupDefaultLighting: Scene root is null\n");
        return;
    }

    // 创建光源管理器默认光源
    // Create default lights in light manager
    auto& lightManager = OsgVerseLightManager::instance();
    lightManager.createDefaultLights();

    // 从光源管理器获取默认光源并添加到场景
    // Get default lights from light manager and add to scene
    std::shared_ptr<OsgVerseLight> sunLight = lightManager.getLight("Sun");
    if (sunLight) {
        // 创建 OSG LightSource 节点
        // Create OSG LightSource node
        osg::Light* osgLight = new osg::Light();
        osgLight->setLightNum(0);  // GL_LIGHT0

        // 设置光源颜色和��度
        // Set light color and intensity
        Color lightColor = sunLight->getColor();
        float intensity = sunLight->getIntensity();
        osgLight->setDiffuse(osg::Vec4(lightColor.r * intensity,
                                       lightColor.g * intensity,
                                       lightColor.b * intensity, 1.0f));
        osgLight->setSpecular(osg::Vec4(lightColor.r * intensity,
                                        lightColor.g * intensity,
                                        lightColor.b * intensity, 1.0f));

        // 设置方向光方向
        // Set directional light direction
        osg::Vec3 dir = sunLight->getDirection();
        osgLight->setPosition(osg::Vec4(dir.x(), dir.y(), dir.z(), 0.0f));  // w=0 表示方向光

        // 创建 LightSource 并添加到场景
        // Create LightSource and add to scene
        osg::LightSource* lightSource = new osg::LightSource();
        lightSource->setLight(osgLight);
        lightSource->setLocalStateSetModes(osg::StateAttribute::ON);  // 启用光源
        lightSource->setName("SunLight");

        // 添加到场景根节点
        // Add to scene root
        sceneRoot->addChild(lightSource);

        Base::Console().log("OsgVerseViewer::setupDefaultLighting: Sun light added to scene\n");
    }

    // 设置环境光
    // Set ambient light
    Color ambientColor = lightManager.getAmbientColor();
    if (sceneRoot->getNumChildren() > 0) {
        osg::Node* firstChild = sceneRoot->getChild(0);
        if (firstChild && dynamic_cast<osg::LightSource*>(firstChild)) {
            osg::LightSource* ls = dynamic_cast<osg::LightSource*>(firstChild);
            if (ls && ls->getLight()) {
                ls->getLight()->setAmbient(osg::Vec4(ambientColor.r * _ambientIntensity,
                                                      ambientColor.g * _ambientIntensity,
                                                      ambientColor.b * _ambientIntensity, 1.0f));
                Base::Console().log("OsgVerseViewer::setupDefaultLighting: Ambient light set (%.2f, %.2f, %.2f)\n",
                                  ambientColor.r * _ambientIntensity,
                                  ambientColor.g * _ambientIntensity,
                                  ambientColor.b * _ambientIntensity);
            }
        }
    }

    // 如果启用了背光，添加背光
    // Add backlight if enabled
    if (_backlightEnabled) {
        osg::Light* backLight = new osg::Light();
        backLight->setLightNum(1);  // GL_LIGHT1

        // 背光从相机后方照射
        // Backlight comes from behind the camera
        osg::Vec3 backDir(0.0f, 1.0f, 0.0f);  // 从后方照射
        backLight->setPosition(osg::Vec4(backDir.x(), backDir.y(), backDir.z(), 0.0f));

        // 较弱的背光强度
        // Weaker backlight intensity
        float backIntensity = 0.3f;
        backLight->setDiffuse(osg::Vec4(backIntensity, backIntensity, backIntensity, 1.0f));
        backLight->setSpecular(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));  // 无镜面反射
        backLight->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));    // 无环境光

        osg::LightSource* backLightSource = new osg::LightSource();
        backLightSource->setLight(backLight);
        backLightSource->setLocalStateSetModes(osg::StateAttribute::ON);
        backLightSource->setName("BackLight");

        sceneRoot->addChild(backLightSource);

        Base::Console().log("OsgVerseViewer::setupDefaultLighting: Back light added to scene\n");
    }

    // 确保场景根节点的 StateSet 启用光照
    // Ensure scene root StateSet has lighting enabled
    osg::StateSet* stateSet = sceneRoot->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);
    if (_backlightEnabled) {
        stateSet->setMode(GL_LIGHT1, osg::StateAttribute::ON);
    }

    // Apply GLSL 1.20 Phong shader globally on the scene root.
    // Child geometry inherits this shader and overrides baseColor per-object.
    OsgVerseShaderManager::instance().applyShader(stateSet, ShaderType::Standard);

    // Default base color uniform (child geometry overrides this per-object)
    stateSet->addUniform(new osg::Uniform("baseColor", osg::Vec4(0.8f, 0.8f, 0.9f, 1.0f)));

    Base::Console().log("OsgVerseViewer::setupDefaultLighting: Default lighting setup complete\n");
}

//===========================================================================
// ViewerWidget Implementation
//===========================================================================

OsgVerseViewer::ViewerWidget::ViewerWidget(OsgVerseViewer* osgVerseViewer,
                                            osgViewer::Viewer* viewer,
                                            osgViewer::GraphicsWindowEmbedded* graphicsWindow,
                                            QWidget* parent)
    : QOpenGLWidget(parent)
    , _osgVerseViewer(osgVerseViewer)
    , _viewer(viewer)
    , _graphicsWindow(graphicsWindow)
    , _firstFrame(true)
{
    // Set OpenGL format (like OsgVerse qt_viewer example)
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setSamples(4);
    setFormat(format);

    // Enable mouse tracking and keyboard focus
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setMinimumSize(320, 240);

    Base::Console().log("OsgVerseViewer::ViewerWidget: Constructor completed\n");
}

OsgVerseViewer::ViewerWidget::~ViewerWidget()
{
    Base::Console().log("OsgVerseViewer::ViewerWidget: Destructor called\n");

    // Release OpenGL resources while context is still valid
    // QOpenGLWidget::makeCurrent() is available here
    makeCurrent();

    // Stop the OSG viewer first to prevent further rendering
    if (_viewer) {
        _viewer->setDone(false);  // Don't delete viewer here, just stop it
    }

    // Release GraphicsWindow
    if (_graphicsWindow.valid()) {
        _graphicsWindow->close();
        _graphicsWindow = nullptr;
    }

    doneCurrent();
    Base::Console().log("OsgVerseViewer::ViewerWidget: Destructor complete\n");
}

void OsgVerseViewer::ViewerWidget::initializeGL()
{
    Base::Console().log("OsgVerseViewer::ViewerWidget::initializeGL: OpenGL context initialized\n");
}

void OsgVerseViewer::ViewerWidget::paintGL()
{
    if (_viewer) {
        // Set default FBO on first frame (like OsgVerse qt_viewer example)
        if (_firstFrame) {
            GLuint defaultFboId = this->defaultFramebufferObject();
            _graphicsWindow->setDefaultFboId(defaultFboId);
            _firstFrame = false;
            Base::Console().log("OsgVerseViewer::ViewerWidget::paintGL: Set default FBO ID: %u\n", defaultFboId);
        }
        _viewer->frame();

        // Deferred fitAll: wait until scene actually has geometry (more than just lights)
        if (_osgVerseViewer && _osgVerseViewer->_pendingFitAll) {
            osg::Group* root = _osgVerseViewer->_engine
                ? _osgVerseViewer->_engine->getOsgSceneRoot() : nullptr;
            if (root && root->getNumChildren() > 2) {
                _osgVerseViewer->fitAll();
                _osgVerseViewer->_pendingFitAll = false;
            }
        }

        // Draw NaviCube AFTER main scene (so it appears on top)
        // OpenGL context is active here
        if (_osgVerseViewer && _osgVerseViewer->_naviCube && _osgVerseViewer->_naviCubeEnabled) {
            _osgVerseViewer->_naviCube->draw();
        }
    }
}

void OsgVerseViewer::ViewerWidget::resizeGL(int width, int height)
{
    // On HiDPI/Retina displays, the framebuffer is larger than logical pixels.
    // resizeGL receives logical size, but OSG viewport needs physical pixels.
    const float dpr = devicePixelRatio();
    const int physW = static_cast<int>(width * dpr);
    const int physH = static_cast<int>(height * dpr);

    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->windowResize(this->x(), this->y(), physW, physH);
        _graphicsWindow->resized(this->x(), this->y(), physW, physH);
    }

    if (_viewer) {
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            camera->setViewport(0, 0, physW, physH);
        }
    }

    // Paint once to avoid flicker (like OsgVerse qt_viewer example)
    paintGL();
}

void OsgVerseViewer::ViewerWidget::mousePressEvent(QMouseEvent* event)
{
    // Dispatch to registered event callbacks first
    if (_osgVerseViewer->dispatchEventCallbacks(OsgVerseViewer::EventType::MouseButtonPress, event)) {
        event->accept();
        return;
    }

    // Check NaviCube first - it has priority for mouse events
    if (_osgVerseViewer && _osgVerseViewer->_naviCube &&
        _osgVerseViewer->_naviCubeEnabled &&
        _osgVerseViewer->_naviCube->handleMouseEvent(event)) {
        event->accept();
        update();
        return;
    }

    // Stop spin animation on any mouse press
    if (_osgVerseViewer) {
        _osgVerseViewer->_isSpinning = false;
    }

    // Right click → context menu
    if (event->button() == Qt::RightButton && _osgVerseViewer) {
        _osgVerseViewer->showContextMenu(event->globalPosition().toPoint());
        event->accept();
        return;
    }

    // Left click → selection
    if (event->button() == Qt::LeftButton && _osgVerseViewer) {
        const float dpr = devicePixelRatio();
        int sx = static_cast<int>(event->position().x() * dpr);
        int sy = static_cast<int>(event->position().y() * dpr);
        bool ctrl = event->modifiers() & Qt::ControlModifier;
        _osgVerseViewer->handleSelection(sx, sy, ctrl);
    }

    if (_graphicsWindow.valid()) {
        const float dpr = devicePixelRatio();
        float x = static_cast<float>(event->position().x()) * dpr;
        float y = static_cast<float>(event->position().y()) * dpr;
        unsigned int button = 0;

        switch (event->button()) {
            case Qt::LeftButton: button = 1; break;
            case Qt::MiddleButton: button = 2; break;
            case Qt::RightButton: button = 3; break;
            default: break;
        }

        _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, button);
        event->accept();
    }
    update();
}

void OsgVerseViewer::ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // Dispatch to registered event callbacks first
    if (_osgVerseViewer->dispatchEventCallbacks(OsgVerseViewer::EventType::MouseButtonRelease, event)) {
        event->accept();
        return;
    }

    // Check NaviCube first
    if (_osgVerseViewer && _osgVerseViewer->_naviCube &&
        _osgVerseViewer->_naviCubeEnabled &&
        _osgVerseViewer->_naviCube->handleMouseEvent(event)) {
        event->accept();
        update();
        return;
    }

    // Complete rubber band selection if active
    if (_rubberBandActive && event->button() == Qt::LeftButton) {
        _rubberBandActive = false;
        _rubberBandEnd = event->pos();

        const float dpr = devicePixelRatio();
        int x1 = static_cast<int>(_rubberBandStart.x() * dpr);
        int y1 = static_cast<int>(_rubberBandStart.y() * dpr);
        int x2 = static_cast<int>(_rubberBandEnd.x() * dpr);
        int y2 = static_cast<int>(_rubberBandEnd.y() * dpr);

        // Only do box selection if the drag was significant (> 5 pixels)
        if (std::abs(x2 - x1) > 5 || std::abs(y2 - y1) > 5) {
            bool ctrl = event->modifiers() & Qt::ControlModifier;
            if (_osgVerseViewer) {
                _osgVerseViewer->handleBoxSelection(x1, y1, x2, y2, ctrl);
            }
        }

        update();
        event->accept();
        return;
    }

    if (_graphicsWindow.valid()) {
        const float dpr = devicePixelRatio();
        float x = static_cast<float>(event->position().x()) * dpr;
        float y = static_cast<float>(event->position().y()) * dpr;
        unsigned int button = 0;

        switch (event->button()) {
            case Qt::LeftButton: button = 1; break;
            case Qt::MiddleButton: button = 2; break;
            case Qt::RightButton: button = 3; break;
            default: break;
        }

        _graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, button);
        event->accept();

        // Start spin animation if middle button released with velocity
        if (_osgVerseViewer && _osgVerseViewer->_spinEnabled &&
            event->button() == Qt::MiddleButton) {
            // Use the last mouse move delta as velocity estimate
            // The OSG event queue tracks positions; we compute velocity from
            // the release position vs the last known position
            double vx = static_cast<double>(event->position().x()) - _lastMouseX;
            double vy = static_cast<double>(event->position().y()) - _lastMouseY;
            double speed = std::abs(vx) + std::abs(vy);
            if (speed > 2.0) {
                _osgVerseViewer->_isSpinning = true;
                _osgVerseViewer->_spinVelocityX = vx;
                _osgVerseViewer->_spinVelocityY = vy;
            }
        }
    }
    update();
}

void OsgVerseViewer::ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
    // Dispatch to registered event callbacks first
    if (_osgVerseViewer->dispatchEventCallbacks(OsgVerseViewer::EventType::MouseMove, event)) {
        event->accept();
        return;
    }

    // Check NaviCube first
    if (_osgVerseViewer && _osgVerseViewer->_naviCube &&
        _osgVerseViewer->_naviCubeEnabled &&
        _osgVerseViewer->_naviCube->handleMouseEvent(event)) {
        event->accept();
        update();
        return;
    }

    // Preselection: only when no mouse buttons are pressed (pure hover)
    if (event->buttons() == Qt::NoButton && _osgVerseViewer) {
        const float dpr = devicePixelRatio();
        int sx = static_cast<int>(event->position().x() * dpr);
        int sy = static_cast<int>(event->position().y() * dpr);
        _osgVerseViewer->handlePreselection(sx, sy);
    }

    // Update rubber band if active
    if (_rubberBandActive) {
        _rubberBandEnd = event->pos();
        update();  // Trigger repaint for rubber band overlay
    }

    // Determine navigation action from current style
    OsgVerseViewer::NavAction action = OsgVerseViewer::NavAction::None;
    if (_osgVerseViewer) {
        action = _osgVerseViewer->getNavAction(event);
    }

    // Forward to OSG — the TrackballManipulator handles the actual
    // camera manipulation. For Inventor style, we remap buttons so
    // that the manipulator receives the expected button codes:
    //   OSG TrackballManipulator: left=rotate, middle=pan, right=zoom
    if (_graphicsWindow.valid()) {
        const float dpr = devicePixelRatio();
        float osgX = static_cast<float>(event->position().x()) * dpr;
        float osgY = static_cast<float>(event->position().y()) * dpr;
        _graphicsWindow->getEventQueue()->mouseMotion(osgX, osgY);
        event->accept();
    }

    // Track last mouse position for spin velocity calculation
    _lastMouseX = event->position().x();
    _lastMouseY = event->position().y();

    update();
}

void OsgVerseViewer::ViewerWidget::wheelEvent(QWheelEvent* event)
{
    // Dispatch to registered event callbacks first
    if (_osgVerseViewer->dispatchEventCallbacks(OsgVerseViewer::EventType::Wheel, event)) {
        event->accept();
        return;
    }

    if (_graphicsWindow.valid()) {
        int delta = event->angleDelta().y();

        osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ?
            osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;

        _graphicsWindow->getEventQueue()->mouseScroll(motion);

        event->accept();
    }
    update();
}

void OsgVerseViewer::ViewerWidget::keyPressEvent(QKeyEvent* event)
{
    // Dispatch to registered event callbacks first
    if (_osgVerseViewer->dispatchEventCallbacks(OsgVerseViewer::EventType::KeyPress, event)) {
        event->accept();
        return;
    }

    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->keyPress(event->key());
    }
    update();
}

void OsgVerseViewer::ViewerWidget::keyReleaseEvent(QKeyEvent* event)
{
    // Dispatch to registered event callbacks first
    if (_osgVerseViewer->dispatchEventCallbacks(OsgVerseViewer::EventType::KeyRelease, event)) {
        event->accept();
        return;
    }

    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->keyRelease(event->key());
    }
    update();
}

void OsgVerseViewer::ViewerWidget::paintEvent(QPaintEvent* event)
{
    // Let QOpenGLWidget do its normal GL rendering
    QOpenGLWidget::paintEvent(event);

    // Draw rubber band overlay if active
    if (_rubberBandActive) {
        QPainter painter(this);
        painter.setPen(QPen(QColor(100, 100, 255), 1, Qt::DashLine));
        painter.setBrush(QColor(100, 100, 255, 40));
        QRect rect = QRect(_rubberBandStart, _rubberBandEnd).normalized();
        painter.drawRect(rect);
        painter.end();
    }
}


//-----------------------------------------------------------------------
// 延迟初始化 / Lazy Initialization
//-----------------------------------------------------------------------

void OsgVerseViewer::ensureInitialized()
{
    // 如果已经初始化或初始化失败，直接返回
    // If already initialized or initialization failed, return immediately
    if (_initialized || _initializationFailed) {
        return;
    }

    Base::Console().log("OsgVerseViewer: Starting lazy initialization...\n");

    try {
        // 检查前置条件
        // Check prerequisites
        if (!checkPrerequisites()) {
            Base::Console().error("OsgVerseViewer: Prerequisites check failed\n");
            _initializationFailed = true;
            return;
        }

        // 创建引擎
        // Create engine
        Base::Console().log("OsgVerseViewer: Creating engine...\n");
        _engine = std::make_unique<OsgVerseEngine>();
        _engine->initialize();
        Base::Console().log("OsgVerseViewer: Engine created and initialized\n");

        // 初始化查看器
        // Initialize viewer
        Base::Console().log("OsgVerseViewer: Initializing viewer...\n");
        initializeViewer();
        Base::Console().log("OsgVerseViewer: Viewer initialized\n");

        // 设置默认相机
        // Setup default camera
        Base::Console().log("OsgVerseViewer: Setting up default camera...\n");
        setupDefaultCamera();
        Base::Console().log("OsgVerseViewer: Default camera set up\n");

        // 设置默认光照
        // Setup default lighting
        Base::Console().log("OsgVerseViewer: Setting up default lighting...\n");
        setupDefaultLighting();
        Base::Console().log("OsgVerseViewer: Default lighting set up\n");

        // 创建 NaviCube
        // Create NaviCube
        Base::Console().log("OsgVerseViewer: Creating NaviCube...\n");
        try {
            _naviCube = std::make_unique<OsgVerseNaviCube>(this);
            Base::Console().log("OsgVerseViewer: NaviCube created successfully\n");
        } catch (const std::exception& e) {
            Base::Console().error("OsgVerseViewer: Failed to create NaviCube: %s\n", e.what());
            // NaviCube 创建失败不应阻止整个初始化
            // NaviCube creation failure should not block overall initialization
        }

        // Create picking service
        _pickingService = std::make_unique<OsgVersePickingService>();
        _pickingService->setOsgVerseViewer(this);

        // Setup gradient background and axis cross
        setupGradientBackground();
        setupAxisCross();

        _initialized = true;
        Base::Console().log("OsgVerseViewer: Lazy initialization completed successfully\n");
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseViewer: Initialization failed with exception: %s\n", e.what());
        _initializationFailed = true;
        _initialized = false;
    }
    catch (...) {
        Base::Console().error("OsgVerseViewer: Initialization failed with unknown exception\n");
        _initializationFailed = true;
        _initialized = false;
    }
}

bool OsgVerseViewer::checkPrerequisites()
{
    Base::Console().log("OsgVerseViewer: Checking prerequisites...\n");

    // 检查 Qt 应用程序
    // Check Qt application
    if (!QApplication::instance()) {
        Base::Console().warning("OsgVerseViewer: Qt application not initialized yet\n");
        // 注意：这可能是正常的，在某些启动阶段
        // Note: This might be normal during certain startup phases
    }

    // 检查 OpenGL 上下文（可选，因为可能还未创建）
    // Check OpenGL context (optional, as it might not be created yet)
    // 我们不强制要求，因为上下文会在 Widget 创建时建立
    // We don't enforce this as context will be established when Widget is created

    Base::Console().log("OsgVerseViewer: Prerequisites check passed\n");
    return true;
}

//===========================================================================
// 相机动画和预设视角实现 / Camera Animation and Preset Views Implementation
//===========================================================================

void OsgVerseViewer::setPresetView(PresetView view)
{
    ensureInitialized();

    if (!_viewer || !_viewer->getCamera()) {
        Base::Console().warning("OsgVerseViewer::setPresetView: Viewer or camera not initialized\n");
        return;
    }

    osg::Camera* camera = _viewer->getCamera();
    if (!camera) {
        Base::Console().warning("OsgVerseViewer::setPresetView: Failed to get camera\n");
        return;
    }

    // 获取场景包围盒用于计算相机位置
    // Get scene bounding box for camera position calculation
    osg::BoundingBox sceneBounds;
    if (_engine && _engine->getOsgSceneRoot()) {
        osg::ComputeBoundsVisitor boundsVisitor;
        _engine->getOsgSceneRoot()->accept(boundsVisitor);
        sceneBounds = boundsVisitor.getBoundingBox();
    }

    // 如果场景为空，使用默认包围盒
    // If scene is empty, use default bounding box
    if (!sceneBounds.valid()) {
        sceneBounds.set(-10, -10, -10, 10, 10, 10);
    }

    osg::Vec3d center = sceneBounds.center();
    double radius = sceneBounds.radius();

    // 根据预设视角设置相机位置
    // Set camera position based on preset view
    osg::Vec3d eye;
    osg::Vec3d target = center;
    osg::Vec3d up(0, 0, 1);

    switch (view) {
        case PresetView::Top:
            // 从上方俯视 / View from top
            eye = osg::Vec3d(center.x(), center.y(), center.z() + radius * 2.5);
            up = osg::Vec3d(0, 1, 0);  // Y 轴向上
            break;

        case PresetView::Bottom:
            // 从下方仰视 / View from bottom
            eye = osg::Vec3d(center.x(), center.y(), center.z() - radius * 2.5);
            up = osg::Vec3d(0, 1, 0);
            break;

        case PresetView::Front:
            // 从前方正视 / View from front (Y- direction)
            eye = osg::Vec3d(center.x(), center.y() - radius * 2.5, center.z());
            break;

        case PresetView::Right:
            // 从右方右视 / View from right (X+ direction)
            eye = osg::Vec3d(center.x() + radius * 2.5, center.y(), center.z());
            break;

        case PresetView::Left:
            // 从左方左视 / View from left (X- direction)
            eye = osg::Vec3d(center.x() - radius * 2.5, center.y(), center.z());
            break;

        case PresetView::Rear:
            // 从后方后视 / View from rear (Y+ direction)
            eye = osg::Vec3d(center.x(), center.y() + radius * 2.5, center.z());
            break;

        case PresetView::Iso: {
            // 等轴测视角 / Isometric view
            // 从斜上方观察，45度角
            double isoDistance = radius * 3.0;
            double isoAngle = M_PI / 4.0;  // 45 degrees
            eye = osg::Vec3d(
                center.x() + isoDistance * std::cos(isoAngle),
                center.y() - isoDistance * std::sin(isoAngle),
                center.z() + isoDistance * 0.5
            );
            break;
        }

        case PresetView::Default:
        default:
            // ��认视角 / Default view
            eye = osg::Vec3d(center.x(), center.y() - radius * 2.0, center.z() + radius * 0.5);
            break;
    }

    // 更新预设视角状态
    // Update preset view state
    _presetView = view;

    // 如果启用了动画，使用平滑过渡
    // If animation is enabled, use smooth transition
    if (_animationEnabled) {
        // Convert osg::Vec3d to Vec3d
        Vec3d eyeVec(eye.x(), eye.y(), eye.z());
        Vec3d targetVec(target.x(), target.y(), target.z());
        startCameraAnimation(eyeVec, targetVec);
    } else {
        // 直接设置相机位置
        // Set camera position directly
        camera->setViewMatrixAsLookAt(eye, target, up);
        _cameraParams.position = Vec3f(static_cast<float>(eye.x()), static_cast<float>(eye.y()), static_cast<float>(eye.z()));
        _cameraParams.target = Vec3f(static_cast<float>(target.x()), static_cast<float>(target.y()), static_cast<float>(target.z()));
        _cameraParams.up = Vec3f(static_cast<float>(up.x()), static_cast<float>(up.y()), static_cast<float>(up.z()));
    }

    Base::Console().log("OsgVerseViewer: Switched to preset view %d\n", static_cast<int>(view));
}

void OsgVerseViewer::saveCurrentViewAsPreset(const std::string& name)
{
    if (name.empty()) {
        Base::Console().warning("OsgVerseViewer::saveCurrentViewAsPreset: Empty name provided\n");
        return;
    }

    // 保存当前相机参数
    // Save current camera parameters
    _savedViewParams[name] = getCamera();

    // 添加到名称列表（如果不存在）
    // Add to name list (if not exists)
    auto it = std::find(_savedViewNames.begin(), _savedViewNames.end(), name);
    if (it == _savedViewNames.end()) {
        _savedViewNames.push_back(name);
    }

    Base::Console().log("OsgVerseViewer: Saved view preset '%s'\n", name.c_str());
}

void OsgVerseViewer::switchToPresetView(const std::string& name)
{
    auto it = _savedViewParams.find(name);
    if (it == _savedViewParams.end()) {
        Base::Console().warning("OsgVerseViewer::switchToPresetView: View preset '%s' not found\n", name.c_str());
        return;
    }

    // 加载保存的相机参数
    // Load saved camera parameters
    const CameraParams& params = it->second;

    if (_animationEnabled) {
        // 使用动画过渡到保存的视角
        // Use animation to transition to saved view
        Vec3d eye(params.position.x, params.position.y, params.position.z);
        Vec3d target(params.target.x, params.target.y, params.target.z);
        startCameraAnimation(eye, target);
    } else {
        // 直接设置相机参数
        // Set camera parameters directly
        setCamera(params);
    }

    _presetView = PresetView::User;
    Base::Console().log("OsgVerseViewer: Switched to saved view '%s'\n", name.c_str());
}

std::string OsgVerseViewer::getPresetViewName() const
{
    switch (_presetView) {
        case PresetView::Top: return "Top";
        case PresetView::Bottom: return "Bottom";
        case PresetView::Front: return "Front";
        case PresetView::Right: return "Right";
        case PresetView::Left: return "Left";
        case PresetView::Rear: return "Rear";
        case PresetView::Iso: return "Iso";
        case PresetView::User: return "User";
        case PresetView::Default:
        default: return "Default";
    }
}

//===========================================================================
// 相机动画内部实现 / Camera Animation Internal Implementation
//===========================================================================

void OsgVerseViewer::startCameraAnimation(const Vec3d& targetEye, const Vec3d& targetCenter)
{
    if (!_viewer || !_viewer->getCamera()) {
        return;
    }

    // 保存当前相机位置作为动画起点
    // Save current camera position as animation start point
    osg::Vec3d eye, center, up;
    _viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    // 设置动画目标
    // Set animation target
    _animationTarget.eye = targetEye;
    _animationTarget.center = targetCenter;
    _animationTarget.distance = (targetEye - targetCenter).Length();

    // 记录开始时间
    // Record start time
    _animationStartTime = _viewer->getFrameStamp()->getReferenceTime();
    _animationComplete = false;

    Base::Console().log("OsgVerseViewer: Starting camera animation (duration: %.2f seconds)\n", _animationDuration);
}

void OsgVerseViewer::updateCameraAnimation(double currentTime)
{
    if (_animationComplete || !_animationEnabled) {
        return;
    }

    if (!_viewer || !_viewer->getCamera()) {
        return;
    }

    // 计算动画进度 (0.0 到 1.0)
    // Calculate animation progress (0.0 to 1.0)
    double elapsed = currentTime - _animationStartTime;
    double progress = elapsed / static_cast<double>(_animationDuration);

    if (progress >= 1.0) {
        // 动画完成
        // Animation complete
        progress = 1.0;
        _animationComplete = true;
        _isAnimating = false;
    } else {
        _isAnimating = true;
    }

    // 使用平滑缓动函数 (ease-in-out)
    // Use smooth easing function (ease-in-out)
    double t = progress;
    double easedT = t * t * (3.0 - 2.0 * t);  // Smoothstep

    // 获取当前相机位置
    // Get current camera position
    osg::Vec3d startEye, startCenter, up;
    _viewer->getCamera()->getViewMatrixAsLookAt(startEye, startCenter, up);

    // 转换目标位置为 osg::Vec3d
    // Convert target positions to osg::Vec3d
    osg::Vec3d targetEye(_animationTarget.eye.x, _animationTarget.eye.y, _animationTarget.eye.z);
    osg::Vec3d targetCenter(_animationTarget.center.x, _animationTarget.center.y, _animationTarget.center.z);

    // 插值计算新的相机位置
    // Interpolate new camera position
    osg::Vec3d currentEye = startEye + (targetEye - startEye) * easedT;
    osg::Vec3d currentCenter = startCenter + (targetCenter - startCenter) * easedT;

    // 应用新的相机位置
    // Apply new camera position
    _viewer->getCamera()->setViewMatrixAsLookAt(currentEye, currentCenter, up);

    // 更新相机参数
    // Update camera parameters
    _cameraParams.position = Vec3f(static_cast<float>(currentEye.x()), static_cast<float>(currentEye.y()), static_cast<float>(currentEye.z()));
    _cameraParams.target = Vec3f(static_cast<float>(currentCenter.x()), static_cast<float>(currentCenter.y()), static_cast<float>(currentCenter.z()));

    // 如果动画完成，触发事件回调
    // Trigger event callback if animation complete
    if (_animationComplete && _eventCallback) {
        // _eventCallback(RenderEvent::CameraAnimationComplete);  // TODO: Define RenderEvent enum
    }
}

void OsgVerseViewer::stopCameraAnimation()
{
    _animationComplete = true;
    _isAnimating = false;
    Base::Console().log("OsgVerseViewer: Camera animation stopped\n");
}

//===========================================================================
// ViewProvider 管理 / ViewProvider Management
//===========================================================================

void OsgVerseViewer::addViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        Base::Console().warning("OsgVerseViewer::addViewProvider: Null ViewProvider\n");
        return;
    }

    // 检查是否已存在
    // Check if already exists
    if (_viewProviders.find(vp) != _viewProviders.end()) {
        Base::Console().log("OsgVerseViewer::addViewProvider: ViewProvider already exists\n");
        return;
    }

    // 尝试获取对象名称 (getNameInDocument() can return nullptr during document restore)
    // Try to get object name
    std::string objName = "ViewProvider";
    if (auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp)) {
        if (vpDoc->getObject()) {
            const char* name = vpDoc->getObject()->getNameInDocument();
            if (name) {
                objName = name;
            }
        }
    }

    Base::Console().log("OsgVerseViewer::addViewProvider: Adding ViewProvider for %s\n", objName.c_str());

    // 添加到集合
    // Add to set
    _viewProviders.insert(vp);

  try {  // Top-level try-catch to prevent crashes during document restore

    // 创建OSG节点
    // Create OSG node
    osg::ref_ptr<osg::Group> vpGroup = new osg::Group();
    vpGroup->setName(objName);

    // 尝试获取几何体
    // Try to get geometry
    bool hasGeometry = false;

    // Get geometry via App-level ComplexGeoData interface (no Part module link dependency)
    // Uses virtual dispatch: Part::TopoShape::getFaces() does BRepMesh tessellation at runtime
    if (auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp)) {
        App::DocumentObject* obj = vpDoc->getObject();
        if (obj) {
            // Try multiple paths to find geometry:
            // 1. Direct "Shape" property (Part::Feature and derived)
            // 2. GeoFeature::getPropertyOfGeometry() (generic fallback for Arch, Mesh, etc.)
            const App::PropertyComplexGeoData* geoDataProp = nullptr;

            App::Property* shapeProp = obj->getPropertyByName("Shape");
            if (shapeProp) {
                geoDataProp = dynamic_cast<const App::PropertyComplexGeoData*>(shapeProp);
            }

            // Fallback: use GeoFeature generic interface
            if (!geoDataProp) {
                if (auto* geoFeature = dynamic_cast<App::GeoFeature*>(obj)) {
                    geoDataProp = geoFeature->getPropertyOfGeometry();
                }
            }

            if (geoDataProp) {
                const Data::ComplexGeoData* geoData = geoDataProp->getComplexData();
                if (geoData) {
                    Base::Console().log("OsgVerseViewer::addViewProvider: Getting mesh for %s via ComplexGeoData\n", objName.c_str());

                    try {
                        // Get material list for color info
                        auto* matListProp = dynamic_cast<App::PropertyMaterialList*>(
                            vp->getPropertyByName("ShapeAppearance"));
                        int matCount = (matListProp && matListProp->getSize() > 0) ? matListProp->getSize() : 0;
                        unsigned long numFaces = geoData->countSubElements("Face");

                        bool usePerFaceColor = (matCount > 1 && numFaces > 0);

                        if (usePerFaceColor) {
                            // === Per-face color path ===
                            // Group faces by color, create separate geometry per color group
                            // (gl_Color with BIND_PER_VERTEX is unreliable on macOS GL 2.1,
                            //  so we use baseColor uniform per geometry instead)
                            struct ColorGroup {
                                osg::Vec4 color;
                                std::vector<osg::Vec3> vertices;
                                std::vector<osg::Vec3> normals;
                                std::vector<unsigned int> indices;
                            };
                            std::vector<ColorGroup> colorGroups;
                            // Map color (as 4 ints 0-255) to group index
                            std::map<uint32_t, size_t> colorToGroup;

                            auto colorKey = [](const osg::Vec4& c) -> uint32_t {
                                return (uint32_t(c.r() * 255) << 24) | (uint32_t(c.g() * 255) << 16)
                                     | (uint32_t(c.b() * 255) << 8)  | uint32_t(c.a() * 255);
                            };

                            for (unsigned long fi = 0; fi < numFaces; fi++) {
                                std::vector<Base::Vector3d> facePoints, faceNormals;
                                std::vector<Data::ComplexGeoData::Facet> faceFacets;

                                try {
                                    std::unique_ptr<Data::Segment> seg(geoData->getSubElement("Face", fi + 1));
                                    if (!seg) continue;
                                    geoData->getFacesFromSubElement(seg.get(), facePoints, faceNormals, faceFacets);
                                } catch (...) {
                                    continue;
                                }
                                if (facePoints.empty() || faceFacets.empty()) continue;

                                // Get color for this face
                                osg::Vec4 faceColor(0.8f, 0.8f, 0.9f, 1.0f);
                                int colorIdx = (static_cast<int>(fi) < matCount) ? static_cast<int>(fi) : 0;
                                if (matCount > 0) {
                                    const Base::Color& c = matListProp->getDiffuseColor(colorIdx);
                                    float trans = matListProp->getTransparency(colorIdx);
                                    faceColor = osg::Vec4(c.r, c.g, c.b, 1.0f - trans);
                                }

                                // Find or create color group
                                uint32_t key = colorKey(faceColor);
                                size_t gi;
                                auto it = colorToGroup.find(key);
                                if (it != colorToGroup.end()) {
                                    gi = it->second;
                                } else {
                                    gi = colorGroups.size();
                                    colorToGroup[key] = gi;
                                    colorGroups.push_back({faceColor, {}, {}, {}});
                                }
                                ColorGroup& grp = colorGroups[gi];

                                unsigned int baseIdx = static_cast<unsigned int>(grp.vertices.size());
                                bool hasNormals = (faceNormals.size() == facePoints.size());
                                for (size_t i = 0; i < facePoints.size(); i++) {
                                    grp.vertices.push_back(osg::Vec3(
                                        static_cast<float>(facePoints[i].x),
                                        static_cast<float>(facePoints[i].y),
                                        static_cast<float>(facePoints[i].z)));
                                    if (hasNormals) {
                                        grp.normals.push_back(osg::Vec3(
                                            static_cast<float>(faceNormals[i].x),
                                            static_cast<float>(faceNormals[i].y),
                                            static_cast<float>(faceNormals[i].z)));
                                    }
                                }
                                for (size_t i = 0; i < faceFacets.size(); i++) {
                                    grp.indices.push_back(baseIdx + faceFacets[i].I1);
                                    grp.indices.push_back(baseIdx + faceFacets[i].I2);
                                    grp.indices.push_back(baseIdx + faceFacets[i].I3);
                                }
                            }

                            // Create one osg::Geometry per color group
                            int colorGroupIdx = 0;
                            for (auto& grp : colorGroups) {
                                if (grp.vertices.empty() || grp.indices.empty()) continue;

                                osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(grp.vertices.begin(), grp.vertices.end());
                                osg::ref_ptr<osg::DrawElementsUInt> ia =
                                    new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, grp.indices.begin(), grp.indices.end());

                                // Normals
                                osg::ref_ptr<osg::Vec3Array> na;
                                if (grp.normals.size() == grp.vertices.size()) {
                                    na = new osg::Vec3Array(grp.normals.begin(), grp.normals.end());
                                } else {
                                    na = new osg::Vec3Array(grp.vertices.size());
                                    for (size_t i = 0; i < grp.vertices.size(); i++)
                                        (*na)[i] = osg::Vec3(0, 0, 0);
                                    for (size_t i = 0; i + 2 < grp.indices.size(); i += 3) {
                                        osg::Vec3 fn = ((*va)[grp.indices[i+1]] - (*va)[grp.indices[i]])
                                                     ^ ((*va)[grp.indices[i+2]] - (*va)[grp.indices[i]]);
                                        (*na)[grp.indices[i]]   += fn;
                                        (*na)[grp.indices[i+1]] += fn;
                                        (*na)[grp.indices[i+2]] += fn;
                                    }
                                    for (size_t i = 0; i < grp.vertices.size(); i++) {
                                        float len = (*na)[i].length();
                                        (*na)[i] = (len > 1e-7f) ? (*na)[i] / len : osg::Vec3(0, 0, 1);
                                    }
                                }

                                osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
                                geometry->setVertexArray(va.get());
                                geometry->setNormalArray(na.get());
                                geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
                                geometry->addPrimitiveSet(ia.get());

                                osg::StateSet* ss = geometry->getOrCreateStateSet();
                                ss->addUniform(new osg::Uniform("baseColor", grp.color));
                                ss->addUniform(new osg::Uniform("u_colorMode", 0));

                                osg::ref_ptr<osg::Material> mat = new osg::Material();
                                mat->setDiffuse(osg::Material::FRONT_AND_BACK, grp.color);
                                mat->setAmbient(osg::Material::FRONT_AND_BACK,
                                    osg::Vec4(grp.color.r()*0.3f, grp.color.g()*0.3f, grp.color.b()*0.3f, 1.0f));
                                mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));
                                mat->setShininess(osg::Material::FRONT_AND_BACK, 40.0f);
                                mat->setColorMode(osg::Material::OFF);
                                if (grp.color.a() < 1.0f) {
                                    mat->setAlpha(osg::Material::FRONT_AND_BACK, grp.color.a());
                                    ss->setMode(GL_BLEND, osg::StateAttribute::ON);
                                    ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                                }
                                ss->setAttributeAndModes(mat.get(),
                                    osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                                ss->setMode(GL_LIGHTING, osg::StateAttribute::ON);

                                osg::ref_ptr<osg::Geode> geode = new osg::Geode();
                                geode->addDrawable(geometry.get());
                                geode->setName("ColorGroup" + std::to_string(colorGroupIdx));
                                vpGroup->addChild(geode);
                                hasGeometry = true;
                                colorGroupIdx++;
                            }

                            Base::Console().log("OsgVerseViewer::addViewProvider: %s per-face: %lu faces, %zu color groups\n",
                                objName.c_str(), numFaces, colorGroups.size());
                        } else {
                            // === Single color path (original) ===
                            std::vector<Base::Vector3d> points;
                            std::vector<Data::ComplexGeoData::Facet> facets;
                            double accuracy = geoData->getAccuracy();
                            geoData->getFaces(points, facets, accuracy);

                            if (!points.empty() && !facets.empty()) {
                                osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array(points.size());
                                for (size_t i = 0; i < points.size(); i++) {
                                    (*vertexArray)[i] = osg::Vec3(
                                        static_cast<float>(points[i].x),
                                        static_cast<float>(points[i].y),
                                        static_cast<float>(points[i].z));
                                }

                                osg::ref_ptr<osg::DrawElementsUInt> indexArray =
                                    new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, facets.size() * 3);
                                for (size_t i = 0; i < facets.size(); i++) {
                                    (*indexArray)[i * 3 + 0] = facets[i].I1;
                                    (*indexArray)[i * 3 + 1] = facets[i].I2;
                                    (*indexArray)[i * 3 + 2] = facets[i].I3;
                                }

                                osg::ref_ptr<osg::Vec3Array> normalArray = new osg::Vec3Array(points.size());
                                for (size_t i = 0; i < points.size(); i++)
                                    (*normalArray)[i] = osg::Vec3(0, 0, 0);
                                for (size_t i = 0; i < facets.size(); i++) {
                                    const osg::Vec3& v0 = (*vertexArray)[facets[i].I1];
                                    const osg::Vec3& v1 = (*vertexArray)[facets[i].I2];
                                    const osg::Vec3& v2 = (*vertexArray)[facets[i].I3];
                                    osg::Vec3 faceNormal = (v1 - v0) ^ (v2 - v0);
                                    (*normalArray)[facets[i].I1] += faceNormal;
                                    (*normalArray)[facets[i].I2] += faceNormal;
                                    (*normalArray)[facets[i].I3] += faceNormal;
                                }
                                for (size_t i = 0; i < points.size(); i++) {
                                    osg::Vec3& n = (*normalArray)[i];
                                    float len = n.length();
                                    n = (len > 1e-7f) ? n / len : osg::Vec3(0, 0, 1);
                                }

                                // Get single diffuse color
                                osg::Vec4 diffuseColor(0.8f, 0.8f, 0.9f, 1.0f);
                                float transparency = 0.0f;
                                if (matCount > 0) {
                                    const Base::Color& c = matListProp->getDiffuseColor();
                                    diffuseColor = osg::Vec4(c.r, c.g, c.b, 1.0f);
                                    transparency = matListProp->getTransparency();
                                    if (transparency > 0.0f)
                                        diffuseColor.a() = 1.0f - transparency;
                                }

                                osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
                                geometry->setVertexArray(vertexArray.get());
                                geometry->setNormalArray(normalArray.get());
                                geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
                                geometry->addPrimitiveSet(indexArray.get());

                                osg::StateSet* stateSet = geometry->getOrCreateStateSet();
                                stateSet->addUniform(new osg::Uniform("baseColor", diffuseColor));
                                stateSet->addUniform(new osg::Uniform("u_colorMode", 0));

                                osg::ref_ptr<osg::Material> material = new osg::Material();
                                material->setDiffuse(osg::Material::FRONT_AND_BACK, diffuseColor);
                                material->setAmbient(osg::Material::FRONT_AND_BACK,
                                    osg::Vec4(diffuseColor.r() * 0.3f, diffuseColor.g() * 0.3f,
                                              diffuseColor.b() * 0.3f, 1.0f));
                                material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));
                                material->setShininess(osg::Material::FRONT_AND_BACK, 40.0f);
                                if (transparency > 0.0f)
                                    material->setAlpha(osg::Material::FRONT_AND_BACK, 1.0f - transparency);
                                material->setColorMode(osg::Material::OFF);

                                stateSet->setAttributeAndModes(material.get(),
                                    osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                                stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);

                                if (transparency > 0.0f) {
                                    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
                                    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                                }

                                osg::ref_ptr<osg::Geode> geode = new osg::Geode();
                                geode->addDrawable(geometry.get());
                                vpGroup->addChild(geode);
                                hasGeometry = true;

                                Base::Console().log("OsgVerseViewer::addViewProvider: Converted %s: %zu vertices, %zu triangles\n",
                                    objName.c_str(), points.size(), facets.size());
                            }
                        }
                    } catch (const std::exception& e) {
                        Base::Console().error("OsgVerseViewer::addViewProvider: Exception: %s\n", e.what());
                    } catch (...) {
                        Base::Console().error("OsgVerseViewer::addViewProvider: Unknown exception during geometry conversion\n");
                    }
                }
            }
        }
    }

    // Skip ViewProviders without geometry (FEM constraints, analysis containers, etc.)
    // Keep the VP registered so it can be updated later when geometry becomes available
    // (e.g., during document restore, shapes may not be computed yet)
    if (!hasGeometry) {
        Base::Console().log("OsgVerseViewer::addViewProvider: No geometry for %s (will retry on update)\n", objName.c_str());
        return;
    }

    // Respect VP visibility — hide node if VP is not visible
    // (e.g., construction geometry like Wires, Rectangles in Arch)
    if (!vp->isVisible()) {
        vpGroup->setNodeMask(0x0);
    }

    // 添加到场景图
    // Add to scene graph
    if (_engine) {
        osg::Group* sceneRoot = _engine->getOsgSceneRoot();
        if (sceneRoot) {
            sceneRoot->addChild(vpGroup);
        }
    }

    // 保存映射
    // Save mapping
    _vpToNodeMap[vp] = vpGroup;
    _nodeToVPMap[vpGroup.get()] = vp;

    // Apply current override mode if set
    if (!_overrideMode.empty()) {
        vp->setOverrideMode(_overrideMode);
    }

  } catch (const std::exception& e) {
    Base::Console().error("OsgVerseViewer::addViewProvider: CAUGHT exception for %s: %s\n",
                          objName.c_str(), e.what());
  } catch (...) {
    Base::Console().error("OsgVerseViewer::addViewProvider: CAUGHT unknown exception for %s\n",
                          objName.c_str());
  }
}

void OsgVerseViewer::removeViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }

    Base::Console().log("OsgVerseViewer::removeViewProvider: Removing ViewProvider\n");

    // 从场景图中移除节点
    // Remove node from scene graph
    auto it = _vpToNodeMap.find(vp);
    if (it != _vpToNodeMap.end()) {
        osg::Node* node = it->second.get();

        if (_engine) {
            osg::Group* sceneRoot = _engine->getOsgSceneRoot();
            if (sceneRoot) {
                // Check if node is still a child before removing
                int index = sceneRoot->getChildIndex(node);
                if (index >= 0 && index < static_cast<int>(sceneRoot->getNumChildren())) {
                    sceneRoot->removeChild(node);
                    Base::Console().log("OsgVerseViewer::removeViewProvider: Removed node from scene root\n");
                }
            }
        }

        // 清理映射
        // Clean up mappings
        _nodeToVPMap.erase(node);
        _vpToNodeMap.erase(it);
    }

    // 从集合中移除
    // Remove from set
    _viewProviders.erase(vp);

    Base::Console().log("OsgVerseViewer::removeViewProvider: Removal complete\n");
}

void OsgVerseViewer::updateViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }

    Base::Console().log("OsgVerseViewer::updateViewProvider: Updating ViewProvider\n");

    // 简单实现：先删除再添加
    // Simple implementation: remove and re-add
    removeViewProvider(vp);
    addViewProvider(vp);
}

bool OsgVerseViewer::hasViewProvider(Gui::ViewProvider* vp) const
{
    return _viewProviders.find(vp) != _viewProviders.end();
}

std::vector<Gui::ViewProvider*> OsgVerseViewer::getViewProviders() const
{
    return std::vector<Gui::ViewProvider*>(_viewProviders.begin(), _viewProviders.end());
}

//===========================================================================
// Selection support
//===========================================================================

Gui::ViewProvider* OsgVerseViewer::findViewProviderForNode(osg::Node* node) const
{
    if (!node) return nullptr;

    // Direct lookup first
    auto it = _nodeToVPMap.find(node);
    if (it != _nodeToVPMap.end()) {
        return it->second;
    }

    // Walk up the parent hierarchy to find a mapped node
    // OSG nodes can have multiple parents, but in our scene graph each VP group
    // has a unique path from the scene root
    const osg::Node::ParentList& parents = node->getParents();
    for (osg::Group* parent : parents) {
        auto pit = _nodeToVPMap.find(parent);
        if (pit != _nodeToVPMap.end()) {
            return pit->second;
        }
        // Recurse up
        Gui::ViewProvider* vp = findViewProviderForNode(parent);
        if (vp) return vp;
    }

    return nullptr;
}

osg::Node* OsgVerseViewer::getNodeForViewProvider(Gui::ViewProvider* vp) const
{
    auto it = _vpToNodeMap.find(vp);
    if (it != _vpToNodeMap.end()) {
        return it->second.get();
    }
    return nullptr;
}

//===========================================================================
// Selection integration
//===========================================================================

void OsgVerseViewer::handlePreselection(int screenX, int screenY)
{
    if (!_pickingService) return;

    Gui::Render::PickResults results = _pickingService->pick(screenX, screenY);

    if (results.hasHit()) {
        const Gui::Render::PickResult& hit = results.closest();
        Gui::ViewProvider* vp = hit.viewProvider;

        if (vp != _preselectedVP || hit.elementName != _preselectedElement) {
            // Clear old preselection highlight
            if (_preselectedVP) {
                updatePreselectionHighlight(_preselectedVP, false);
            }

            _preselectedVP = vp;
            _preselectedElement = hit.elementName;

            if (vp) {
                updatePreselectionHighlight(vp, true);

                // Notify FreeCAD SelectionSingleton
                if (auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp)) {
                    if (vpDoc->getObject() && vpDoc->getObject()->getDocument()) {
                        const char* docName = vpDoc->getObject()->getDocument()->getName();
                        const char* objName = vpDoc->getObject()->getNameInDocument();
                        if (docName && objName) {
                            Gui::Selection().setPreselect(
                                docName, objName,
                                hit.elementName.c_str(),
                                hit.point.x, hit.point.y, hit.point.z);
                        }
                    }
                }
            }
        }
    } else {
        // Nothing under cursor — clear preselection
        if (_preselectedVP) {
            updatePreselectionHighlight(_preselectedVP, false);
            _preselectedVP = nullptr;
            _preselectedElement.clear();
            Gui::Selection().rmvPreselect();
        }
    }
}

void OsgVerseViewer::handleSelection(int screenX, int screenY, bool ctrlPressed)
{
    if (!_pickingService) return;

    Gui::Render::PickResults results = _pickingService->pick(screenX, screenY);

    if (results.hasHit()) {
        const Gui::Render::PickResult& hit = results.closest();
        Gui::ViewProvider* vp = hit.viewProvider;

        if (vp) {
            if (auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp)) {
                if (vpDoc->getObject() && vpDoc->getObject()->getDocument()) {
                    const char* docName = vpDoc->getObject()->getDocument()->getName();
                    const char* objName = vpDoc->getObject()->getNameInDocument();
                    if (docName && objName) {
                        if (!ctrlPressed) {
                            // Single select: clear previous, add new
                            Gui::Selection().clearSelection(docName);
                        }
                        Gui::Selection().addSelection(
                            docName, objName,
                            hit.elementName.c_str(),
                            hit.point.x, hit.point.y, hit.point.z);
                    }
                }
            }
        }
    } else {
        // Clicked on empty space — clear selection
        if (!ctrlPressed) {
            Gui::Selection().clearSelection();
            clearSelectionHighlights();
        }
    }
}

void OsgVerseViewer::updateSelectionHighlight(Gui::ViewProvider* vp, bool selected)
{
    osg::Node* node = getNodeForViewProvider(vp);
    if (!node) return;

    osg::StateSet* ss = node->getOrCreateStateSet();
    if (selected) {
        ss->addUniform(new osg::Uniform("u_selectionColor", osg::Vec4(0.1f, 0.8f, 0.1f, 1.0f)));
        ss->addUniform(new osg::Uniform("u_selectionActive", 1));
    } else {
        ss->removeUniform("u_selectionColor");
        ss->removeUniform("u_selectionActive");
    }
}

void OsgVerseViewer::updatePreselectionHighlight(Gui::ViewProvider* vp, bool preselected)
{
    osg::Node* node = getNodeForViewProvider(vp);
    if (!node) return;

    osg::StateSet* ss = node->getOrCreateStateSet();
    if (preselected) {
        ss->addUniform(new osg::Uniform("u_preselectionColor", osg::Vec4(0.8f, 0.8f, 0.1f, 1.0f)));
        ss->addUniform(new osg::Uniform("u_preselectionActive", 1));
    } else {
        ss->removeUniform("u_preselectionColor");
        ss->removeUniform("u_preselectionActive");
    }
}

void OsgVerseViewer::clearSelectionHighlights()
{
    for (auto& pair : _vpToNodeMap) {
        updateSelectionHighlight(pair.first, false);
    }
}

//===========================================================================
// Editing mode
//===========================================================================

void OsgVerseViewer::setEditingViewProvider(Gui::ViewProvider* vp, int mode)
{
    if (_editingVP == vp) return;

    // Exit current editing mode if active
    if (_editingVP) {
        resetEditingViewProvider();
    }

    if (!vp) return;

    _editingVP = vp;
    _editingMode = mode;

    Base::Console().log("OsgVerseViewer: Entering edit mode %d\n", mode);

    // Create editing root node
    if (!_editingRoot) {
        _editingRoot = new osg::Group();
        _editingRoot->setName("EditingRoot");
    }

    // Reduce opacity of non-editing objects
    for (auto& pair : _vpToNodeMap) {
        if (pair.first != vp) {
            osg::Node* node = pair.second.get();
            if (node) {
                osg::StateSet* ss = node->getOrCreateStateSet();
                // Store original transparency and make semi-transparent
                ss->addUniform(new osg::Uniform("u_editingDimmed", 1));
            }
        }
    }
}

void OsgVerseViewer::resetEditingViewProvider()
{
    if (!_editingVP) return;

    Base::Console().log("OsgVerseViewer: Exiting edit mode\n");

    // Restore opacity of all objects
    for (auto& pair : _vpToNodeMap) {
        osg::Node* node = pair.second.get();
        if (node) {
            osg::StateSet* ss = node->getStateSet();
            if (ss) {
                ss->removeUniform("u_editingDimmed");
            }
        }
    }

    _editingVP = nullptr;
    _editingMode = 0;
    _editingRoot = nullptr;
}

//===========================================================================
// Editing root node
//===========================================================================

void OsgVerseViewer::setupEditingRoot(void* node, const Base::Matrix4D* mat)
{
    osg::Group* sceneRoot = _engine ? _engine->getOsgSceneRoot() : nullptr;
    if (!sceneRoot) return;

    if (!_editingRootNode) {
        _editingRootNode = new osg::Group();
        _editingRootNode->setName("EditingRootNode");
    }

    if (!_editingTransform) {
        _editingTransform = new osg::MatrixTransform();
        _editingTransform->setName("EditingTransform");
    }

    if (mat) {
        double glMat[16];
        mat->getGLMatrix(glMat);
        _editingTransform->setMatrix(osg::Matrixd(glMat));
    }

    if (_editingRootNode->getChildIndex(_editingTransform.get()) == _editingRootNode->getNumChildren()) {
        _editingRootNode->addChild(_editingTransform.get());
    }

    if (node) {
        osg::Node* osgNode = static_cast<osg::Node*>(node);
        _editingRootNode->addChild(osgNode);
    } else if (_editingVP) {
        auto it = _vpToNodeMap.find(_editingVP);
        if (it != _vpToNodeMap.end()) {
            osg::Node* vpNode = it->second.get();
            osg::Group* vpGroup = vpNode ? vpNode->asGroup() : nullptr;
            if (vpGroup) {
                while (vpGroup->getNumChildren() > 0) {
                    osg::ref_ptr<osg::Node> child = vpGroup->getChild(0);
                    vpGroup->removeChild(0u, 1u);
                    _editingRootNode->addChild(child.get());
                }
            }
        }
    }

    sceneRoot->addChild(_editingRootNode.get());

    for (auto& pair : _vpToNodeMap) {
        if (pair.first != _editingVP) {
            osg::Node* n = pair.second.get();
            if (n) {
                osg::StateSet* ss = n->getOrCreateStateSet();
                ss->addUniform(new osg::Uniform("u_editingDimmed", 1));
            }
        }
    }

    Base::Console().log("OsgVerseViewer: Editing root set up\n");
}

void OsgVerseViewer::resetEditingRoot(bool updateLinks)
{
    if (!_editingRootNode) return;

    if (_editingVP) {
        auto it = _vpToNodeMap.find(_editingVP);
        if (it != _vpToNodeMap.end()) {
            osg::Group* vpGroup = it->second.get() ? it->second->asGroup() : nullptr;
            if (vpGroup) {
                for (unsigned int i = _editingRootNode->getNumChildren(); i > 0; --i) {
                    osg::ref_ptr<osg::Node> child = _editingRootNode->getChild(i - 1);
                    if (child.get() != _editingTransform.get()) {
                        _editingRootNode->removeChild(i - 1, 1u);
                        vpGroup->addChild(child.get());
                    }
                }
            }
        }
    }

    while (_editingRootNode->getNumParents() > 0) {
        osg::Group* parent = _editingRootNode->getParent(0);
        parent->removeChild(_editingRootNode.get());
    }

    for (auto& pair : _vpToNodeMap) {
        osg::Node* n = pair.second.get();
        if (n) {
            osg::StateSet* ss = n->getStateSet();
            if (ss) {
                ss->removeUniform("u_editingDimmed");
            }
        }
    }

    _editingRootNode = nullptr;
    _editingTransform = nullptr;

    if (updateLinks) {
        Base::Console().log("OsgVerseViewer: Editing root reset (updateLinks=true, link update is future work)\n");
    } else {
        Base::Console().log("OsgVerseViewer: Editing root reset\n");
    }
}

void OsgVerseViewer::setEditingTransform(const Base::Matrix4D& mat)
{
    if (!_editingTransform) return;

    double glMat[16];
    mat.getGLMatrix(glMat);
    _editingTransform->setMatrix(osg::Matrixd(glMat));
}

//===========================================================================
// Override mode
//===========================================================================

void OsgVerseViewer::setOverrideMode(const std::string& mode)
{
    _overrideMode = mode;

    for (auto* vp : _viewProviders) {
        if (vp) {
            vp->setOverrideMode(mode);
        }
    }

    Base::Console().log("OsgVerseViewer: Override mode set to '%s'\n", mode.c_str());
}

//===========================================================================
// Box selection
//===========================================================================

void OsgVerseViewer::handleBoxSelection(int x1, int y1, int x2, int y2, bool ctrlPressed)
{
    if (!_pickingService) return;

    Gui::Render::PickResults results = _pickingService->pickRegion(x1, y1, x2, y2);

    if (!ctrlPressed) {
        Gui::Selection().clearSelection();
        clearSelectionHighlights();
    }

    for (const auto& hit : results.hits) {
        // Resolve ViewProvider for each hit
        // The region picker doesn't automatically resolve VPs, so we need
        // to use the point-based lookup
        if (hit.viewProvider) {
            if (auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(hit.viewProvider)) {
                if (vpDoc->getObject() && vpDoc->getObject()->getDocument()) {
                    const char* docName = vpDoc->getObject()->getDocument()->getName();
                    const char* objName = vpDoc->getObject()->getNameInDocument();
                    if (docName && objName) {
                        Gui::Selection().addSelection(docName, objName);
                    }
                }
            }
        }
    }
}

void OsgVerseViewer::setRubberBandEnabled(bool enabled)
{
    // Rubber band state is managed internally by ViewerWidget
    // This method is called by the adapter to signal selection mode changes
    (void)enabled;
}

//===========================================================================
// Navigation style
//===========================================================================

void OsgVerseViewer::setNavigationStyle(const std::string& style)
{
    _navigationStyle = style;
    Base::Console().log("OsgVerseViewer: Navigation style set to '%s'\n", style.c_str());
}

OsgVerseViewer::NavAction OsgVerseViewer::getNavAction(QMouseEvent* event) const
{
    Qt::MouseButtons buttons = event->buttons();
    Qt::KeyboardModifiers mods = event->modifiers();

    if (_navigationStyle == "CAD" || _navigationStyle.empty()) {
        // CAD Navigation (FreeCAD default)
        if (buttons & Qt::MiddleButton) {
            if (mods & Qt::ShiftModifier) return NavAction::Pan;
            if (mods & Qt::ControlModifier) return NavAction::Zoom;
            return NavAction::Rotate;
        }
        if ((buttons & Qt::RightButton) && (mods & Qt::ShiftModifier)) {
            return NavAction::Pan;
        }
    }
    else if (_navigationStyle == "Blender") {
        if (buttons & Qt::MiddleButton) {
            if (mods & Qt::ShiftModifier) return NavAction::Pan;
            if (mods & Qt::ControlModifier) return NavAction::Zoom;
            return NavAction::Rotate;
        }
    }
    else if (_navigationStyle == "Inventor") {
        if ((buttons & Qt::LeftButton) && (buttons & Qt::MiddleButton)) {
            return NavAction::Zoom;
        }
        if (buttons & Qt::MiddleButton) {
            return NavAction::Pan;
        }
        if (buttons & Qt::LeftButton) {
            return NavAction::Rotate;
        }
    }

    return NavAction::None;
}

void OsgVerseViewer::scale(float factor)
{
    ensureInitialized();
    if (!_viewer) return;

    CameraParams cam = getCamera();
    Base::Vector3d dir(cam.target.x - cam.position.x,
                       cam.target.y - cam.position.y,
                       cam.target.z - cam.position.z);
    double dist = dir.Length();
    dir.Normalize();

    double newDist = dist / factor;
    Base::Vector3d newPos = Base::Vector3d(cam.target.x, cam.target.y, cam.target.z) - dir * newDist;
    cam.position = Vec3f(static_cast<float>(newPos.x), static_cast<float>(newPos.y), static_cast<float>(newPos.z));

    if (cam.orthographic) {
        cam.height /= factor;
    }

    setCamera(cam);
}

void OsgVerseViewer::moveCameraTo(const Base::Vector3d& target)
{
    ensureInitialized();
    if (!_viewer) return;

    CameraParams cam = getCamera();
    Base::Vector3d dir(cam.target.x - cam.position.x,
                       cam.target.y - cam.position.y,
                       cam.target.z - cam.position.z);
    double dist = dir.Length();
    dir.Normalize();

    Base::Vector3d newTarget = target;
    Base::Vector3d newPos = target - dir * dist;
    cam.target = Vec3f(static_cast<float>(newTarget.x), static_cast<float>(newTarget.y), static_cast<float>(newTarget.z));
    cam.position = Vec3f(static_cast<float>(newPos.x), static_cast<float>(newPos.y), static_cast<float>(newPos.z));

    if (_animationEnabled) {
        startCameraAnimation(
            Vec3d(cam.position.x, cam.position.y, cam.position.z),
            Vec3d(cam.target.x, cam.target.y, cam.target.z)
        );
    } else {
        setCamera(cam);
    }
}

std::vector<std::string> OsgVerseViewer::listNavigationTypes()
{
    return {"Gui::CADNavigationStyle", "Gui::BlenderNavigationStyle", "Gui::InventorNavigationStyle"};
}

//===========================================================================
// Context menu
//===========================================================================

void OsgVerseViewer::showContextMenu(const QPoint& globalPos)
{
    QMenu menu;

    // Standard views
    QMenu* viewMenu = menu.addMenu(QObject::tr("Standard views"));
    viewMenu->addAction(QObject::tr("Front"), [this]() { setPresetView(PresetView::Front); });
    viewMenu->addAction(QObject::tr("Rear"), [this]() { setPresetView(PresetView::Rear); });
    viewMenu->addAction(QObject::tr("Top"), [this]() { setPresetView(PresetView::Top); });
    viewMenu->addAction(QObject::tr("Bottom"), [this]() { setPresetView(PresetView::Bottom); });
    viewMenu->addAction(QObject::tr("Left"), [this]() { setPresetView(PresetView::Left); });
    viewMenu->addAction(QObject::tr("Right"), [this]() { setPresetView(PresetView::Right); });
    viewMenu->addAction(QObject::tr("Isometric"), [this]() { setPresetView(PresetView::Iso); });

    menu.addSeparator();

    // Render modes
    QMenu* renderMenu = menu.addMenu(QObject::tr("Draw style"));
    renderMenu->addAction(QObject::tr("As Is"), [this]() { setRenderMode(RenderMode::Default); });
    renderMenu->addAction(QObject::tr("Wireframe"), [this]() { setRenderMode(RenderMode::Wireframe); });
    renderMenu->addAction(QObject::tr("Shaded"), [this]() { setRenderMode(RenderMode::Shaded); });
    renderMenu->addAction(QObject::tr("Flat Lines"), [this]() { setRenderMode(RenderMode::Flat); });
    renderMenu->addAction(QObject::tr("Points"), [this]() { setRenderMode(RenderMode::Points); });

    menu.addSeparator();

    // Camera
    menu.addAction(QObject::tr("Fit All"), [this]() { fitAll(); });

    menu.exec(globalPos);
}

//===========================================================================
// Gradient Background
//===========================================================================

void OsgVerseViewer::setupGradientBackground()
{
    if (!_viewer) return;

    osg::Group* sceneRoot = _engine ? _engine->getOsgSceneRoot() : nullptr;
    if (!sceneRoot) return;

    // Create HUD camera for gradient background
    _gradientCamera = new osg::Camera();
    _gradientCamera->setName("GradientBackground");
    _gradientCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _gradientCamera->setRenderOrder(osg::Camera::PRE_RENDER, -100);
    _gradientCamera->setClearMask(0);  // Don't clear anything
    _gradientCamera->setAllowEventFocus(false);
    _gradientCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
    _gradientCamera->setViewMatrix(osg::Matrix::identity());

    // Fullscreen quad with vertex colors
    _gradientGeom = new osg::Geometry();
    _gradientGeom->setUseDisplayList(false);
    _gradientGeom->setUseVertexBufferObjects(true);

    osg::Vec3Array* verts = new osg::Vec3Array(4);
    (*verts)[0].set(0.0f, 0.0f, 0.0f);  // bottom-left
    (*verts)[1].set(1.0f, 0.0f, 0.0f);  // bottom-right
    (*verts)[2].set(1.0f, 1.0f, 0.0f);  // top-right
    (*verts)[3].set(0.0f, 1.0f, 0.0f);  // top-left
    _gradientGeom->setVertexArray(verts);

    // Default gradient: dark blue bottom, lighter blue top
    osg::Vec4Array* colors = new osg::Vec4Array(4);
    (*colors)[0].set(0.10f, 0.10f, 0.20f, 1.0f);  // bottom
    (*colors)[1].set(0.10f, 0.10f, 0.20f, 1.0f);  // bottom
    (*colors)[2].set(0.30f, 0.35f, 0.50f, 1.0f);  // top
    (*colors)[3].set(0.30f, 0.35f, 0.50f, 1.0f);  // top
    _gradientGeom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);

    _gradientGeom->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

    // GLSL 1.20 shader — pass vertex color through a varying
    static const char* gradVert =
        "#version 120\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "    vColor = gl_Color;\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n";

    static const char* gradFrag =
        "#version 120\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "    gl_FragColor = vColor;\n"
        "}\n";

    osg::Program* prog = new osg::Program();
    prog->addShader(new osg::Shader(osg::Shader::VERTEX, gradVert));
    prog->addShader(new osg::Shader(osg::Shader::FRAGMENT, gradFrag));

    osg::StateSet* ss = _gradientGeom->getOrCreateStateSet();
    ss->setAttributeAndModes(prog, osg::StateAttribute::ON);
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(_gradientGeom.get());
    _gradientCamera->addChild(geode);

    // Insert as first child so it renders behind everything
    sceneRoot->insertChild(0, _gradientCamera.get());

    Base::Console().log("OsgVerseViewer: Gradient background created\n");
}

void OsgVerseViewer::setGradientBackground(float topR, float topG, float topB,
                                            float botR, float botG, float botB)
{
    ensureInitialized();

    // Create gradient camera on first call
    if (!_gradientCamera) {
        setupGradientBackground();
    }

    if (!_gradientGeom) return;

    osg::Vec4Array* colors = dynamic_cast<osg::Vec4Array*>(_gradientGeom->getColorArray());
    if (!colors || colors->size() < 4) return;

    (*colors)[0].set(botR, botG, botB, 1.0f);  // bottom-left
    (*colors)[1].set(botR, botG, botB, 1.0f);  // bottom-right
    (*colors)[2].set(topR, topG, topB, 1.0f);  // top-right
    (*colors)[3].set(topR, topG, topB, 1.0f);  // top-left
    colors->dirty();
    _gradientGeom->dirtyDisplayList();

    // Disable main camera clear color since gradient provides the background
    if (_viewer && _viewer->getCamera()) {
        _viewer->getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);
    }
}

//===========================================================================
// Axis Cross
//===========================================================================

void OsgVerseViewer::setupAxisCross()
{
    if (!_viewer) return;

    osg::Group* sceneRoot = _engine ? _engine->getOsgSceneRoot() : nullptr;
    if (!sceneRoot) return;

    // HUD camera in bottom-left corner (100x100 logical pixels)
    _axisCrossCamera = new osg::Camera();
    _axisCrossCamera->setName("AxisCross");
    _axisCrossCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _axisCrossCamera->setRenderOrder(osg::Camera::POST_RENDER, 100);
    _axisCrossCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    _axisCrossCamera->setAllowEventFocus(false);
    // Orthographic projection centered at origin, axes extend [-1,1]
    _axisCrossCamera->setProjectionMatrix(osg::Matrix::ortho(-1.5, 1.5, -1.5, 1.5, -10, 10));
    _axisCrossCamera->setViewMatrix(osg::Matrix::identity());
    // Viewport will be set in onResize; default to 100x100 at bottom-left
    _axisCrossCamera->setViewport(10, 10, 100, 100);

    // Transform node that rotates with the main camera
    _axisCrossTransform = new osg::MatrixTransform();

    // Create XYZ axis lines
    osg::Geometry* axisGeom = new osg::Geometry();
    axisGeom->setUseDisplayList(false);

    osg::Vec3Array* verts = new osg::Vec3Array(6);
    // X axis
    (*verts)[0].set(0.0f, 0.0f, 0.0f);
    (*verts)[1].set(1.0f, 0.0f, 0.0f);
    // Y axis
    (*verts)[2].set(0.0f, 0.0f, 0.0f);
    (*verts)[3].set(0.0f, 1.0f, 0.0f);
    // Z axis
    (*verts)[4].set(0.0f, 0.0f, 0.0f);
    (*verts)[5].set(0.0f, 0.0f, 1.0f);
    axisGeom->setVertexArray(verts);

    osg::Vec4Array* colors = new osg::Vec4Array(6);
    (*colors)[0].set(1.0f, 0.0f, 0.0f, 1.0f);  // X = red
    (*colors)[1].set(1.0f, 0.0f, 0.0f, 1.0f);
    (*colors)[2].set(0.0f, 1.0f, 0.0f, 1.0f);  // Y = green
    (*colors)[3].set(0.0f, 1.0f, 0.0f, 1.0f);
    (*colors)[4].set(0.0f, 0.3f, 1.0f, 1.0f);  // Z = blue
    (*colors)[5].set(0.0f, 0.3f, 1.0f, 1.0f);
    axisGeom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);

    axisGeom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 6));

    // Simple pass-through shader (same as gradient)
    static const char* axisVert =
        "#version 120\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "    vColor = gl_Color;\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n";

    static const char* axisFrag =
        "#version 120\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "    gl_FragColor = vColor;\n"
        "}\n";

    osg::Program* prog = new osg::Program();
    prog->addShader(new osg::Shader(osg::Shader::VERTEX, axisVert));
    prog->addShader(new osg::Shader(osg::Shader::FRAGMENT, axisFrag));

    osg::StateSet* ss = axisGeom->getOrCreateStateSet();
    ss->setAttributeAndModes(prog, osg::StateAttribute::ON);
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setAttributeAndModes(new osg::LineWidth(2.0f), osg::StateAttribute::ON);

    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(axisGeom);
    _axisCrossTransform->addChild(geode);
    _axisCrossCamera->addChild(_axisCrossTransform.get());

    sceneRoot->addChild(_axisCrossCamera.get());

    Base::Console().log("OsgVerseViewer: Axis cross created\n");
}

void OsgVerseViewer::updateAxisCross()
{
    if (!_axisCrossTransform || !_viewer) return;

    // Extract rotation-only from the main camera's view matrix
    osg::Matrixd viewMatrix = _viewer->getCamera()->getViewMatrix();
    // Zero out translation to keep only rotation
    viewMatrix(3, 0) = 0.0;
    viewMatrix(3, 1) = 0.0;
    viewMatrix(3, 2) = 0.0;
    _axisCrossTransform->setMatrix(viewMatrix);
}

void OsgVerseViewer::setAxisCrossEnabled(bool enabled)
{
    _axisCrossEnabled = enabled;
    if (_axisCrossCamera) {
        _axisCrossCamera->setNodeMask(enabled ? ~0u : 0u);
    }
}

//===========================================================================
// Phase G: Focal Plane Projection
//===========================================================================

Base::Vector3d OsgVerseViewer::getPointOnFocalPlane(int x, int y) const
{
    if (!_viewer || !_pickingService) {
        return Base::Vector3d(0, 0, 0);
    }

    // Focal plane passes through the camera target, with normal = view direction
    Base::Vector3f eye = _cameraParams.position;
    Base::Vector3f center = _cameraParams.target;
    Base::Vector3f dir = center - eye;
    dir.Normalize();

    Vector3 planePoint(center.x, center.y, center.z);
    Vector3 planeNormal(dir.x, dir.y, dir.z);
    Vector3 worldPoint;

    if (_pickingService->unprojectToPlane(x, y, planePoint, planeNormal, worldPoint)) {
        return Base::Vector3d(worldPoint.x, worldPoint.y, worldPoint.z);
    }

    return Base::Vector3d(0, 0, 0);
}

//===========================================================================
// Coordinate Projection System / 坐标投影系统
//===========================================================================

// Helper: screen coords (Qt convention, Y top-down) to world ray
static bool screenToWorldRay(osgViewer::Viewer* viewer, int qtX, int qtY,
                              osg::Vec3d& rayOrigin, osg::Vec3d& rayDir)
{
    osg::Camera* cam = viewer->getCamera();
    if (!cam || !cam->getViewport()) {
        return false;
    }

    int vpWidth = static_cast<int>(cam->getViewport()->width());
    int vpHeight = static_cast<int>(cam->getViewport()->height());
    if (vpWidth <= 0 || vpHeight <= 0) {
        return false;
    }

    // Qt Y is top-down, NDC Y is bottom-up
    double ndcX = (2.0 * qtX / vpWidth) - 1.0;
    double ndcY = 1.0 - (2.0 * qtY / vpHeight);

    osg::Matrixd viewMat = cam->getViewMatrix();
    osg::Matrixd projMat = cam->getProjectionMatrix();
    osg::Matrixd invVP = osg::Matrixd::inverse(viewMat * projMat);

    osg::Vec3d nearPt = osg::Vec3d(ndcX, ndcY, -1.0) * invVP;
    osg::Vec3d farPt = osg::Vec3d(ndcX, ndcY, 1.0) * invVP;

    rayOrigin = nearPt;
    rayDir = farPt - nearPt;
    rayDir.normalize();
    return true;
}

// Helper: world point to screen coords (Qt convention)
static osg::Vec3d worldToScreenQt(osgViewer::Viewer* viewer, const osg::Vec3d& worldPt)
{
    osg::Camera* cam = viewer->getCamera();
    if (!cam || !cam->getViewport()) {
        return osg::Vec3d(0, 0, 0);
    }

    osg::Matrixd viewMat = cam->getViewMatrix();
    osg::Matrixd projMat = cam->getProjectionMatrix();
    osg::Matrixd vpMat = cam->getViewport()->computeWindowMatrix();

    osg::Vec3d screenPt = worldPt * viewMat * projMat * vpMat;
    // Flip Y from OSG (bottom-up) to Qt (top-down)
    int vpHeight = static_cast<int>(cam->getViewport()->height());
    screenPt.y() = vpHeight - screenPt.y();
    return screenPt;
}

// Helper: intersect ray with plane, returns false if parallel
static bool rayPlaneIntersect(const osg::Vec3d& rayOrigin, const osg::Vec3d& rayDir,
                               const osg::Vec3d& planePoint, const osg::Vec3d& planeNormal,
                               osg::Vec3d& hitPoint)
{
    double denom = rayDir * planeNormal;
    if (std::abs(denom) < 1e-10) {
        return false;
    }
    double t = ((planePoint - rayOrigin) * planeNormal) / denom;
    hitPoint = rayOrigin + rayDir * t;
    return true;
}

Base::Vector3d OsgVerseViewer::getViewDirection() const
{
    if (!_viewer) {
        return Base::Vector3d(0, 0, -1);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    CameraParams cam = getCamera();
    Base::Vector3d dir(cam.target.x - cam.position.x,
                       cam.target.y - cam.position.y,
                       cam.target.z - cam.position.z);
    dir.Normalize();
    return dir;
}

Base::Vector3d OsgVerseViewer::getUpDirection() const
{
    if (!_viewer) {
        return Base::Vector3d(0, 1, 0);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    CameraParams cam = getCamera();
    return Base::Vector3d(cam.upVector.x, cam.upVector.y, cam.upVector.z);
}

void OsgVerseViewer::getCameraOrientation(double& x, double& y, double& z, double& w) const
{
    if (!_viewer) {
        x = y = z = 0.0;
        w = 1.0;
        return;
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osgGA::TrackballManipulator* manip =
        dynamic_cast<osgGA::TrackballManipulator*>(_viewer->getCameraManipulator());
    if (manip) {
        osg::Quat q = manip->getRotation();
        x = q.x();
        y = q.y();
        z = q.z();
        w = q.w();
    } else {
        x = y = z = 0.0;
        w = 1.0;
    }
}

QPoint OsgVerseViewer::getPointOnViewport(const Base::Vector3d& pt) const
{
    if (!_viewer) {
        return QPoint(0, 0);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Vec3d worldPt(pt.x, pt.y, pt.z);
    osg::Vec3d screenPt = worldToScreenQt(_viewer, worldPt);
    return QPoint(static_cast<int>(screenPt.x()), static_cast<int>(screenPt.y()));
}

Base::Vector3d OsgVerseViewer::getPointOnLine(const QPoint& screenPos,
                                               const Base::Vector3d& axisCenter,
                                               const Base::Vector3d& axis) const
{
    if (!_viewer) {
        return Base::Vector3d(0, 0, 0);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Vec3d rayOrigin, rayDir;
    if (!screenToWorldRay(_viewer, screenPos.x(), screenPos.y(), rayOrigin, rayDir)) {
        return Base::Vector3d(0, 0, 0);
    }

    // Find closest point on the axis line to the ray
    // Line1: P = rayOrigin + t * rayDir
    // Line2: Q = axisCenter + s * axis
    // Minimize |P - Q|^2
    osg::Vec3d ac(axisCenter.x, axisCenter.y, axisCenter.z);
    osg::Vec3d ax(axis.x, axis.y, axis.z);
    ax.normalize();

    osg::Vec3d w = ac - rayOrigin;
    double a = rayDir * rayDir;
    double b = rayDir * ax;
    double c = ax * ax;
    double d = rayDir * w;
    double e = ax * w;
    double denom = a * c - b * b;

    double s;
    if (std::abs(denom) < 1e-10) {
        s = e / c;
    } else {
        s = (a * e - b * d) / denom;
    }

    osg::Vec3d closestOnAxis = ac + ax * s;
    return Base::Vector3d(closestOnAxis.x(), closestOnAxis.y(), closestOnAxis.z());
}

Base::Vector3d OsgVerseViewer::getPointOnXYPlaneOfPlacement(const QPoint& screenPos,
                                                             const Base::Placement& plc) const
{
    if (!_viewer) {
        return Base::Vector3d(0, 0, 0);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Vec3d rayOrigin, rayDir;
    if (!screenToWorldRay(_viewer, screenPos.x(), screenPos.y(), rayOrigin, rayDir)) {
        return Base::Vector3d(0, 0, 0);
    }

    // The XY plane of the placement: normal is the Z axis of the placement
    Base::Vector3d pos = plc.getPosition();
    Base::Rotation rot = plc.getRotation();
    Base::Vector3d zAxis;
    rot.multVec(Base::Vector3d(0, 0, 1), zAxis);

    osg::Vec3d planePoint(pos.x, pos.y, pos.z);
    osg::Vec3d planeNormal(zAxis.x, zAxis.y, zAxis.z);

    osg::Vec3d hitPoint;
    if (!rayPlaneIntersect(rayOrigin, rayDir, planePoint, planeNormal, hitPoint)) {
        return Base::Vector3d(0, 0, 0);
    }

    return Base::Vector3d(hitPoint.x(), hitPoint.y(), hitPoint.z());
}

Base::Vector2d OsgVerseViewer::getNormalizedPosition(const QPoint& screenPos) const
{
    if (!_viewer) {
        return Base::Vector2d(0.5, 0.5);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Camera* cam = _viewer->getCamera();
    if (!cam || !cam->getViewport()) {
        return Base::Vector2d(0.5, 0.5);
    }

    double vpW = cam->getViewport()->width();
    double vpH = cam->getViewport()->height();
    if (vpW <= 0 || vpH <= 0) {
        return Base::Vector2d(0.5, 0.5);
    }

    return Base::Vector2d(screenPos.x() / vpW, screenPos.y() / vpH);
}

Base::Vector3d OsgVerseViewer::projectOnNearPlane(const Base::Vector2d& pt) const
{
    if (!_viewer) {
        return Base::Vector3d(0, 0, 0);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Camera* cam = _viewer->getCamera();
    if (!cam) {
        return Base::Vector3d(0, 0, 0);
    }

    // pt is normalized [0..1], convert to NDC [-1..1]
    double ndcX = pt.x * 2.0 - 1.0;
    double ndcY = 1.0 - pt.y * 2.0;  // Flip Y

    osg::Matrixd invVP = osg::Matrixd::inverse(
        cam->getViewMatrix() * cam->getProjectionMatrix());

    osg::Vec3d nearPt = osg::Vec3d(ndcX, ndcY, -1.0) * invVP;
    return Base::Vector3d(nearPt.x(), nearPt.y(), nearPt.z());
}

Base::Vector3d OsgVerseViewer::projectOnFarPlane(const Base::Vector2d& pt) const
{
    if (!_viewer) {
        return Base::Vector3d(0, 0, 0);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Camera* cam = _viewer->getCamera();
    if (!cam) {
        return Base::Vector3d(0, 0, 0);
    }

    double ndcX = pt.x * 2.0 - 1.0;
    double ndcY = 1.0 - pt.y * 2.0;

    osg::Matrixd invVP = osg::Matrixd::inverse(
        cam->getViewMatrix() * cam->getProjectionMatrix());

    osg::Vec3d farPt = osg::Vec3d(ndcX, ndcY, 1.0) * invVP;
    return Base::Vector3d(farPt.x(), farPt.y(), farPt.z());
}

void OsgVerseViewer::projectPointToLine(const QPoint& screenPos,
                                         Base::Vector3d& pt1,
                                         Base::Vector3d& pt2) const
{
    if (!_viewer) {
        pt1 = pt2 = Base::Vector3d(0, 0, 0);
        return;
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Vec3d rayOrigin, rayDir;
    if (!screenToWorldRay(_viewer, screenPos.x(), screenPos.y(), rayOrigin, rayDir)) {
        pt1 = pt2 = Base::Vector3d(0, 0, 0);
        return;
    }

    // pt1 = near point (ray origin), pt2 = far point
    osg::Vec3d farPt = rayOrigin + rayDir * 10000.0;
    pt1 = Base::Vector3d(rayOrigin.x(), rayOrigin.y(), rayOrigin.z());
    pt2 = Base::Vector3d(farPt.x(), farPt.y(), farPt.z());
}

Base::Vector3d OsgVerseViewer::getCenterPointOnFocalPlane() const
{
    if (!_viewer) {
        return Base::Vector3d(0, 0, 0);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Camera* cam = _viewer->getCamera();
    if (!cam || !cam->getViewport()) {
        return Base::Vector3d(0, 0, 0);
    }

    int cx = static_cast<int>(cam->getViewport()->width() / 2);
    int cy = static_cast<int>(cam->getViewport()->height() / 2);
    return getPointOnFocalPlane(cx, cy);
}

void OsgVerseViewer::getNearPlane(Base::Vector3d& pt, Base::Vector3d& normal) const
{
    if (!_viewer) {
        pt = Base::Vector3d(0, 0, 0);
        normal = Base::Vector3d(0, 0, 1);
        return;
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    // Near plane center = project screen center onto near plane
    Base::Vector2d center(0.5, 0.5);
    pt = projectOnNearPlane(center);

    // Normal = negative view direction (pointing toward camera)
    Base::Vector3d viewDir = getViewDirection();
    normal = Base::Vector3d(-viewDir.x, -viewDir.y, -viewDir.z);
}

void OsgVerseViewer::getFarPlane(Base::Vector3d& pt, Base::Vector3d& normal) const
{
    if (!_viewer) {
        pt = Base::Vector3d(0, 0, 0);
        normal = Base::Vector3d(0, 0, -1);
        return;
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    Base::Vector2d center(0.5, 0.5);
    pt = projectOnFarPlane(center);

    // Normal = view direction (pointing away from camera)
    normal = getViewDirection();
}

void OsgVerseViewer::getDimensions(float& height, float& width) const
{
    height = width = 1.0f;
    if (!_viewer) {
        return;
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Camera* cam = _viewer->getCamera();
    if (!cam || !cam->getViewport()) {
        return;
    }

    double fovy, aspectRatio, zNear, zFar;
    if (!cam->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar)) {
        return;
    }

    CameraParams params = getCamera();
    double dx = params.target.x - params.position.x;
    double dy = params.target.y - params.position.y;
    double dz = params.target.z - params.position.z;
    double focalDist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (focalDist < 1e-6) {
        focalDist = 1.0;
    }

    double fovyRad = fovy * M_PI / 180.0;
    height = static_cast<float>(2.0 * focalDist * std::tan(fovyRad / 2.0));
    width = height * static_cast<float>(aspectRatio);
}

float OsgVerseViewer::getMaxDimension() const
{
    float h, w;
    getDimensions(h, w);
    return std::max(h, w);
}

void OsgVerseViewer::getBoundingBox(Base::Vector3d& min, Base::Vector3d& max) const
{
    min = Base::Vector3d(0, 0, 0);
    max = Base::Vector3d(0, 0, 0);

    if (!_viewer || !_viewer->getSceneData()) {
        return;
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::ComputeBoundsVisitor cbv;
    _viewer->getSceneData()->accept(cbv);
    const osg::BoundingBox& bb = cbv.getBoundingBox();

    if (bb.valid()) {
        min = Base::Vector3d(bb.xMin(), bb.yMin(), bb.zMin());
        max = Base::Vector3d(bb.xMax(), bb.yMax(), bb.zMax());
    }
}

//===========================================================================
// Phase G: Seek
//===========================================================================

bool OsgVerseViewer::seekToPoint(int screenX, int screenY)
{
    if (!_pickingService) {
        return false;
    }

    PickResults results = _pickingService->pick(screenX, screenY);
    if (!results.hasHit()) {
        return false;
    }

    const PickResult& hit = results.closest();
    seekToPoint(Base::Vector3d(hit.point.x, hit.point.y, hit.point.z));
    return true;
}

void OsgVerseViewer::seekToPoint(const Base::Vector3d& worldPos)
{
    if (!_viewer) {
        return;
    }

    // Current camera state
    Base::Vector3f eye = _cameraParams.position;
    Base::Vector3f center = _cameraParams.target;

    // View direction and focal distance
    Base::Vector3f dir = center - eye;
    double focalDist = dir.Length();
    if (focalDist < 1e-6) {
        focalDist = 1.0;
    }
    dir.Normalize();

    // Target: move camera so it looks at worldPos from the same distance/direction
    Vec3d targetCenter(worldPos.x, worldPos.y, worldPos.z);
    Vec3d targetEye(worldPos.x - dir.x * focalDist,
                    worldPos.y - dir.y * focalDist,
                    worldPos.z - dir.z * focalDist);

    startCameraAnimation(targetEye, targetCenter);
}

//===========================================================================
// Phase H: Spin Animation
//===========================================================================

void OsgVerseViewer::updateSpinAnimation()
{
    if (!_isSpinning || !_viewer) {
        return;
    }

    auto* manip = dynamic_cast<osgGA::TrackballManipulator*>(_viewer->getCameraManipulator());
    if (!manip) {
        _isSpinning = false;
        return;
    }

    // Apply rotation based on spin velocity
    osg::Quat currentRotation = manip->getRotation();
    osg::Quat deltaRotation =
        osg::Quat(_spinVelocityX * 0.01, osg::Vec3d(0, 1, 0)) *
        osg::Quat(_spinVelocityY * 0.01, osg::Vec3d(1, 0, 0));
    manip->setRotation(currentRotation * deltaRotation);

    // Decay velocity
    _spinVelocityX *= _spinDecay;
    _spinVelocityY *= _spinDecay;

    // Stop when velocity is negligible
    if (std::abs(_spinVelocityX) + std::abs(_spinVelocityY) < 0.1) {
        _isSpinning = false;
    }
}

//===========================================================================
// Phase I: Drag-Drop Support
//===========================================================================

void OsgVerseViewer::ViewerWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void OsgVerseViewer::ViewerWidget::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        for (const QUrl& url : mimeData->urls()) {
            QString filePath = url.toLocalFile();
            if (!filePath.isEmpty()) {
                App::GetApplication().openDocument(filePath.toUtf8().constData());
            }
        }
        event->acceptProposedAction();
    }
}

//===========================================================================
// Phase I: Clip Planes
//===========================================================================

void OsgVerseViewer::setClipPlane(int index, double a, double b, double c, double d)
{
    if (index < 0 || index >= 6) {
        Base::Console().warning("OsgVerseViewer::setClipPlane: index %d out of range [0,5]\n", index);
        return;
    }

    if (!_viewer || !_viewer->getSceneData()) {
        return;
    }

    // Create clip node if needed, attach to scene root
    if (!_clipNode) {
        _clipNode = new osg::ClipNode();
        auto* sceneRoot = dynamic_cast<osg::Group*>(_viewer->getSceneData());
        if (sceneRoot) {
            sceneRoot->addChild(_clipNode.get());
        }
    }

    // Remove existing clip plane at this index if any
    if (_clipPlanes[index]) {
        _clipNode->removeClipPlane(_clipPlanes[index].get());
    }

    // Create and add new clip plane
    _clipPlanes[index] = new osg::ClipPlane(GL_CLIP_PLANE0 + index);
    _clipPlanes[index]->setClipPlane(osg::Vec4d(a, b, c, d));
    _clipNode->addClipPlane(_clipPlanes[index].get());
    _clipPlaneEnabled[index] = true;

    Base::Console().log("OsgVerseViewer: Clip plane %d set to (%.2f, %.2f, %.2f, %.2f)\n",
                        index, a, b, c, d);
}

void OsgVerseViewer::removeClipPlane(int index)
{
    if (index < 0 || index >= 6) {
        return;
    }

    if (_clipPlanes[index] && _clipNode) {
        _clipNode->removeClipPlane(_clipPlanes[index].get());
    }
    _clipPlanes[index] = nullptr;
    _clipPlaneEnabled[index] = false;
}

bool OsgVerseViewer::isClipPlaneEnabled(int index) const
{
    if (index < 0 || index >= 6) {
        return false;
    }
    return _clipPlaneEnabled[index];
}

void OsgVerseViewer::toggleClipPlane(int index)
{
    if (index < 0 || index >= 6) {
        return;
    }

    if (_clipPlaneEnabled[index]) {
        // Disable: remove from clip node but keep the plane object
        if (_clipPlanes[index] && _clipNode) {
            _clipNode->removeClipPlane(_clipPlanes[index].get());
        }
        _clipPlaneEnabled[index] = false;
    } else {
        // Re-enable: add back to clip node if plane object exists
        if (_clipPlanes[index] && _clipNode) {
            _clipNode->addClipPlane(_clipPlanes[index].get());
            _clipPlaneEnabled[index] = true;
        }
    }
}

//-----------------------------------------------------------------------
// Event Callback System / 事件回调系统
//-----------------------------------------------------------------------

void OsgVerseViewer::addEventCallback(EventType type, EventCallbackFunc cb, void* userData)
{
    _eventCallbacks.push_back({type, cb, userData});
}

void OsgVerseViewer::removeEventCallback(EventType type, EventCallbackFunc cb, void* userData)
{
    _eventCallbacks.remove_if([&](const EventCallbackEntry& entry) {
        return entry.type == type && entry.userData == userData;
    });
}

bool OsgVerseViewer::dispatchEventCallbacks(EventType type, void* event)
{
    for (auto& entry : _eventCallbacks) {
        if (entry.type == type || entry.type == EventType::Any) {
            if (entry.callback(type, event, entry.userData)) {
                return true;  // Event was handled
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------------
// Editing Mode Extensions / 编辑模式扩展
//-----------------------------------------------------------------------

void OsgVerseViewer::setEditing(bool edit)
{
    _editingFlag = edit;
    if (_widget) {
        if (edit && !_editingCursor.shape()) {
            _widget->setCursor(Qt::CrossCursor);
        }
    }
}

void OsgVerseViewer::setEditingCursor(const QCursor& cursor)
{
    _editingCursor = cursor;
    if (_widget && _editingFlag) {
        _widget->setCursor(cursor);
    }
}

void OsgVerseViewer::setComponentCursor(const QCursor& cursor)
{
    _componentCursor = cursor;
}

void OsgVerseViewer::setRedirectToSceneGraph(bool redirect)
{
    _redirectToSceneGraph = redirect;
}

void OsgVerseViewer::setSelectionEnabled(bool enable)
{
    _selectionEnabled = enable;
}

void OsgVerseViewer::setPopupMenuEnabled(bool on)
{
    _popupMenuEnabled = on;
}

//-----------------------------------------------------------------------
// Graphics Overlay System
//-----------------------------------------------------------------------

void OsgVerseViewer::addGraphicsItem(GraphicsItemHandle item)
{
    if (item && std::find(_graphicsItems.begin(), _graphicsItems.end(), item) == _graphicsItems.end()) {
        _graphicsItems.push_back(item);
    }
}

void OsgVerseViewer::removeGraphicsItem(GraphicsItemHandle item)
{
    _graphicsItems.erase(std::remove(_graphicsItems.begin(), _graphicsItems.end(), item), _graphicsItems.end());
}

void OsgVerseViewer::clearGraphicsItems()
{
    _graphicsItems.clear();
}

//-----------------------------------------------------------------------
// Dimension Annotations (stubs)
//-----------------------------------------------------------------------

void OsgVerseViewer::addDimension3d(const Base::Vector3d&, const Base::Vector3d&, const Base::Vector3d&)
{
    Base::Console().log("OsgVerseViewer::addDimension3d: Not yet implemented\n");
}

void OsgVerseViewer::addDimensionDelta(const Base::Vector3d&, const Base::Vector3d&, const Base::Vector3d&)
{
    Base::Console().log("OsgVerseViewer::addDimensionDelta: Not yet implemented\n");
}

void OsgVerseViewer::turnAllDimensionsOn()
{
    _dimensionsVisible = true;
    Base::Console().log("OsgVerseViewer::turnAllDimensionsOn: Not yet implemented\n");
}

void OsgVerseViewer::turnAllDimensionsOff()
{
    _dimensionsVisible = false;
    Base::Console().log("OsgVerseViewer::turnAllDimensionsOff: Not yet implemented\n");
}

void OsgVerseViewer::eraseAllDimensions()
{
    Base::Console().log("OsgVerseViewer::eraseAllDimensions: Not yet implemented\n");
}

void OsgVerseViewer::setDimensionsVisible(bool visible)
{
    _dimensionsVisible = visible;
    Base::Console().log("OsgVerseViewer::setDimensionsVisible: Not yet implemented\n");
}

//-----------------------------------------------------------------------
// Save Picture with Multi-sampling
//-----------------------------------------------------------------------

void OsgVerseViewer::savePicture(int width, int height, int samples, const QColor& bg, QImage& img) const
{
    (void)samples; // Multi-sampling not yet used; reserved for future FBO enhancement

    if (width <= 0 || height <= 0) {
        if (_widget) {
            width = _widget->width();
            height = _widget->height();
        } else {
            width = 800;
            height = 600;
        }
    }

    img = const_cast<OsgVerseViewer*>(this)->grabImage(width, height);

    // Composite transparent pixels over the requested background color
    if (bg.isValid() && bg.alpha() > 0) {
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QColor pixel = img.pixelColor(x, y);
                if (pixel.alpha() < 255) {
                    float alpha = pixel.alphaF();
                    int r = static_cast<int>(pixel.red() * alpha + bg.red() * (1.0f - alpha));
                    int g = static_cast<int>(pixel.green() * alpha + bg.green() * (1.0f - alpha));
                    int b = static_cast<int>(pixel.blue() * alpha + bg.blue() * (1.0f - alpha));
                    img.setPixelColor(x, y, QColor(r, g, b, 255));
                }
            }
        }
    }
}

//-----------------------------------------------------------------------
// Box Zoom
//-----------------------------------------------------------------------

void OsgVerseViewer::boxZoom(int x1, int y1, int x2, int y2)
{
    ensureInitialized();
    if (!_viewer || !_widget) return;

    int left   = std::min(x1, x2);
    int right  = std::max(x1, x2);
    int top    = std::min(y1, y2);
    int bottom = std::max(y1, y2);

    if (right - left < 2 || bottom - top < 2) return;

    int cx = (left + right) / 2;
    int cy = (top + bottom) / 2;

    Base::Vector3d newTarget = getPointOnFocalPlane(cx, cy);

    float boxW = static_cast<float>(right - left);
    float boxH = static_cast<float>(bottom - top);
    float vpW  = static_cast<float>(_widget->width());
    float vpH  = static_cast<float>(_widget->height());

    float zoomFactor = std::min(vpW / boxW, vpH / boxH);

    CameraParams cam = getCamera();
    Base::Vector3d dir(cam.target.x - cam.position.x,
                       cam.target.y - cam.position.y,
                       cam.target.z - cam.position.z);
    double dist = dir.Length();
    dir.Normalize();

    double newDist = dist / zoomFactor;

    Base::Vector3d newPos = newTarget - dir * newDist;
    cam.target   = Vec3f(static_cast<float>(newTarget.x), static_cast<float>(newTarget.y), static_cast<float>(newTarget.z));
    cam.position = Vec3f(static_cast<float>(newPos.x), static_cast<float>(newPos.y), static_cast<float>(newPos.z));

    if (cam.orthographic) {
        cam.height /= zoomFactor;
    }

    setCamera(cam);
}

//-----------------------------------------------------------------------
// Align to Selection
//-----------------------------------------------------------------------

void OsgVerseViewer::alignToSelection()
{
    auto sels = Gui::Selection().getSelectionEx();
    if (sels.empty()) {
        fitSelection();
        return;
    }

    auto& sel = sels.front();
    auto* obj = sel.getObject();
    if (!obj) {
        fitSelection();
        return;
    }

    auto* geoFeature = dynamic_cast<App::GeoFeature*>(obj);
    if (geoFeature) {
        Base::Placement plc = geoFeature->Placement.getValue();
        Base::Rotation rot = plc.getRotation();
        Base::Vector3d zAxis(0, 0, 1);
        rot.multVec(zAxis, zAxis);

        CameraParams cam = getCamera();
        Base::Vector3d center = plc.getPosition();
        Base::Vector3d camPos(cam.position.x, cam.position.y, cam.position.z);
        Base::Vector3d camTgt(cam.target.x, cam.target.y, cam.target.z);
        double dist = (camPos - camTgt).Length();

        Base::Vector3d newPos = center + zAxis * dist;
        cam.target   = Vec3f(static_cast<float>(center.x), static_cast<float>(center.y), static_cast<float>(center.z));
        cam.position = Vec3f(static_cast<float>(newPos.x), static_cast<float>(newPos.y), static_cast<float>(newPos.z));
        cam.upVector = Vec3f(0.0f, 0.0f, 1.0f);

        // Adjust up vector if parallel to view direction
        Base::Vector3d viewDir = center - newPos;
        viewDir.Normalize();
        Base::Vector3d upDir(cam.upVector.x, cam.upVector.y, cam.upVector.z);
        if (std::abs(viewDir.Dot(upDir)) > 0.99) {
            cam.upVector = Vec3f(0.0f, 1.0f, 0.0f);
        }

        setCamera(cam);
    } else {
        fitSelection();
    }
}

//===========================================================================
// Selection Polygon / 选择多边形
//===========================================================================

std::vector<std::pair<int,int>> OsgVerseViewer::getSelectionPolygon(bool* isClosed) const
{
    if (isClosed) {
        *isClosed = _selectionPolygonClosed;
    }

    // If we have a stored polygon from lasso selection, return it
    if (!_selectionPolygon.empty()) {
        return _selectionPolygon;
    }

    // Otherwise, build polygon from rubber band rectangle if active
    if (_widget) {
        // The rubber band defines a rectangle; return its 4 corners
        // as a closed polygon in screen coordinates
        QPoint start = _widget->property("rubberBandStart").toPoint();
        QPoint end = _widget->property("rubberBandEnd").toPoint();
        if (!start.isNull() && !end.isNull()) {
            std::vector<std::pair<int,int>> poly;
            poly.emplace_back(start.x(), start.y());
            poly.emplace_back(end.x(), start.y());
            poly.emplace_back(end.x(), end.y());
            poly.emplace_back(start.x(), end.y());
            if (isClosed) *isClosed = true;
            return poly;
        }
    }

    return {};
}

std::vector<std::pair<float,float>> OsgVerseViewer::getSelectionPolygonNormalized(bool* isClosed) const
{
    auto screenPoly = getSelectionPolygon(isClosed);
    if (screenPoly.empty() || !_viewer) {
        return {};
    }

    const_cast<OsgVerseViewer*>(this)->ensureInitialized();
    osg::Camera* cam = _viewer->getCamera();
    if (!cam || !cam->getViewport()) {
        return {};
    }

    int vpWidth = static_cast<int>(cam->getViewport()->width());
    int vpHeight = static_cast<int>(cam->getViewport()->height());
    if (vpWidth <= 0 || vpHeight <= 0) {
        return {};
    }

    std::vector<std::pair<float,float>> result;
    result.reserve(screenPoly.size());
    for (const auto& pt : screenPoly) {
        float nx = static_cast<float>(pt.first) / vpWidth;
        float ny = static_cast<float>(pt.second) / vpHeight;
        result.emplace_back(nx, ny);
    }
    return result;
}

//===========================================================================
// Ray Picking / 射线拾取
//===========================================================================

Base::Vector3d OsgVerseViewer::getPointOnRay(const QPoint& screenPos,
                                              const Gui::ViewProvider* vp) const
{
    if (!_viewer || !vp) {
        return Base::Vector3d();
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    // Get the OSG node for this ViewProvider
    osg::Node* vpNode = nullptr;
    auto it = _vpToNodeMap.find(const_cast<Gui::ViewProvider*>(vp));
    if (it != _vpToNodeMap.end()) {
        vpNode = it->second.get();
    }

    // If editing VP, use editing root
    if (vp == _editingVP && _editingRootNode.valid() && _editingRootNode->getNumChildren() > 0) {
        vpNode = _editingRootNode.get();
    }

    if (!vpNode) {
        return Base::Vector3d();
    }

    // Construct ray from screen position
    osg::Vec3d rayOrigin, rayDir;
    if (!screenToWorldRay(_viewer, screenPos.x(), screenPos.y(), rayOrigin, rayDir)) {
        return Base::Vector3d();
    }

    // Create line segment for intersection
    osg::Vec3d nearPt = rayOrigin;
    osg::Vec3d farPt = rayOrigin + rayDir * 100000.0;

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, nearPt, farPt);

    osgUtil::IntersectionVisitor iv(intersector.get());
    vpNode->accept(iv);

    if (intersector->containsIntersections()) {
        auto hit = intersector->getFirstIntersection();
        osg::Vec3d worldPt = hit.getWorldIntersectPoint();
        return Base::Vector3d(worldPt.x(), worldPt.y(), worldPt.z());
    }

    return Base::Vector3d();
}

Base::Vector3d OsgVerseViewer::getPointOnRay(const Base::Vector3d& rayOrigin,
                                              const Base::Vector3d& rayDir,
                                              const Gui::ViewProvider* vp) const
{
    if (!_viewer || !vp) {
        return Base::Vector3d();
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    // Get the OSG node for this ViewProvider
    osg::Node* vpNode = nullptr;
    auto it = _vpToNodeMap.find(const_cast<Gui::ViewProvider*>(vp));
    if (it != _vpToNodeMap.end()) {
        vpNode = it->second.get();
    }

    // If editing VP, use editing root
    if (vp == _editingVP && _editingRootNode.valid() && _editingRootNode->getNumChildren() > 0) {
        vpNode = _editingRootNode.get();
    }

    if (!vpNode) {
        return Base::Vector3d();
    }

    // Create line segment from ray origin along ray direction
    osg::Vec3d start(rayOrigin.x, rayOrigin.y, rayOrigin.z);
    osg::Vec3d dir(rayDir.x, rayDir.y, rayDir.z);
    dir.normalize();
    osg::Vec3d end = start + dir * 100000.0;

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, start, end);

    osgUtil::IntersectionVisitor iv(intersector.get());
    vpNode->accept(iv);

    if (intersector->containsIntersections()) {
        auto hit = intersector->getFirstIntersection();
        osg::Vec3d worldPt = hit.getWorldIntersectPoint();
        return Base::Vector3d(worldPt.x(), worldPt.y(), worldPt.z());
    }

    return Base::Vector3d();
}

//===========================================================================
// Viewport on Placement Plane / 视口投影到放置平面
//===========================================================================

Base::BoundBox2d OsgVerseViewer::getViewportOnXYPlaneOfPlacement(const Base::Placement& plc) const
{
    if (!_viewer) {
        return Base::BoundBox2d(0, 0, 0, 0);
    }
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();

    osg::Camera* cam = _viewer->getCamera();
    if (!cam || !cam->getViewport()) {
        return Base::BoundBox2d(0, 0, 0, 0);
    }

    // Get the placement's XY plane: normal is Z axis of placement
    Base::Vector3d pos = plc.getPosition();
    Base::Rotation rot = plc.getRotation();
    Base::Vector3d zAxis;
    rot.multVec(Base::Vector3d(0, 0, 1), zAxis);

    osg::Vec3d planePoint(pos.x, pos.y, pos.z);
    osg::Vec3d planeNormal(zAxis.x, zAxis.y, zAxis.z);

    int vpWidth = static_cast<int>(cam->getViewport()->width());
    int vpHeight = static_cast<int>(cam->getViewport()->height());

    // Project the four corners of the viewport onto the placement's XY plane
    auto projBBox = Base::BoundBox3d();
    projBBox.SetVoid();

    auto projectCorner = [&](int sx, int sy) {
        osg::Vec3d rayOrigin, rayDir;
        if (!screenToWorldRay(_viewer, sx, sy, rayOrigin, rayDir)) {
            return;
        }
        osg::Vec3d hitPoint;
        if (!rayPlaneIntersect(rayOrigin, rayDir, planePoint, planeNormal, hitPoint)) {
            return;
        }
        projBBox.Add(Base::Vector3d(hitPoint.x(), hitPoint.y(), hitPoint.z()));
    };

    projectCorner(0, 0);
    projectCorner(vpWidth, 0);
    projectCorner(0, vpHeight);
    projectCorner(vpWidth, vpHeight);

    if (!projBBox.IsValid()) {
        return Base::BoundBox2d(0, 0, 0, 0);
    }

    // Project the 3D bounding box onto the placement's local XY plane
    Base::Placement invPlc = plc;
    invPlc.invert();
    Base::ViewOrthoProjMatrix proj(invPlc.toMatrix());
    return projBBox.ProjectBox(&proj);
}
