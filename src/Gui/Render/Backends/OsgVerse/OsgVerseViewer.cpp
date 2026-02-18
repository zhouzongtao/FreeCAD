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
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QApplication>
#include <fstream>
#endif

#include "OsgVerseViewer.h"
#include "OsgVerseLight.h"
#include "OsgVerseNaviCube.h"
#include "OsgVerseShaderManager.h"
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
#include <App/DocumentObject.h>
#include <App/Property.h>
#include <App/PropertyStandard.h>

// Shape geometry display via App-level ComplexGeoData interface
// (No direct Part module dependency — uses virtual dispatch at runtime)
#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>
#include <App/ComplexGeoData.h>
// OSG arrays for geometry creation
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Geometry>

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
    if (_viewer) {
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
}

//-----------------------------------------------------------------------
// Screenshot Functionality
//-----------------------------------------------------------------------

QImage OsgVerseViewer::grabImage(int width, int height)
{
    if (!_widget) {
        return QImage();
    }

    // Render to image
    QImage image = _widget->grabFramebuffer();

    if (width > 0 && height > 0 && (image.width() != width || image.height() != height)) {
        image = image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    return image;
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

    // Set default camera parameters
    // 设置更合适的默认相机参数，便于查看 3D 场景
    // Set more suitable default camera parameters for better 3D scene viewing

    // 相机位置：放在 Y 轴后方，稍向下方偏右
    // Camera position: placed back on right side, slightly downward
    // 视线方向：看向原点方向，从斜上方观察
    // Camera target: 放在原点
    // 视场角：更宽的视野（60 度）
    // 近裁剪面：足够远，能容纳场景中的物体

    _cameraParams.position = Vec3f(0.0f, -8.0f, 12.0f);  // 后退 8 单位，上方 12 单位
    _cameraParams.target = Vec3f(0.0f, 0.0f, 0.0f);        // 看向原点
    _cameraParams.upVector = Vec3f(0.0f, 1.0f, 0.0f);       // Y 轴向上
    _cameraParams.fieldOfView = 60.0f;                          // 60 度视场角（更宽）
    _cameraParams.aspectRatio = 1.333f;                        // 宽高比
    _cameraParams.nearPlane = 0.1f;                             // 近裁剪面距离（100mm）
    _cameraParams.farPlane = 1000.0f;

    // Apply camera parameters
    camera->setProjectionMatrixAsPerspective(
        _cameraParams.fieldOfView,
        _cameraParams.aspectRatio,
        _cameraParams.nearPlane,
        _cameraParams.farPlane
    );

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
    // Child geometry inherits this shader and overrides u_baseColor per-object.
    OsgVerseShaderManager::instance().applyShader(stateSet, ShaderType::Standard);

    // Default base color uniform (child geometry overrides this per-object)
    stateSet->addUniform(new osg::Uniform("u_baseColor", osg::Vec4(0.8f, 0.8f, 0.9f, 1.0f)));

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
    // Check NaviCube first - it has priority for mouse events
    if (_osgVerseViewer && _osgVerseViewer->_naviCube &&
        _osgVerseViewer->_naviCubeEnabled &&
        _osgVerseViewer->_naviCube->handleMouseEvent(event)) {
        event->accept();
        update();
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

        _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, button);
        event->accept();
    }
    update();
}

void OsgVerseViewer::ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // Check NaviCube first
    if (_osgVerseViewer && _osgVerseViewer->_naviCube &&
        _osgVerseViewer->_naviCubeEnabled &&
        _osgVerseViewer->_naviCube->handleMouseEvent(event)) {
        event->accept();
        update();
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
    }
    update();
}

void OsgVerseViewer::ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
    // Check NaviCube first
    if (_osgVerseViewer && _osgVerseViewer->_naviCube &&
        _osgVerseViewer->_naviCubeEnabled &&
        _osgVerseViewer->_naviCube->handleMouseEvent(event)) {
        event->accept();
        update();
        return;
    }

    // 正常处理场景鼠标移动
    // Handle normal scene mouse movement
    if (_graphicsWindow.valid()) {
        const float dpr = devicePixelRatio();
        _graphicsWindow->getEventQueue()->mouseMotion(
            static_cast<float>(event->position().x()) * dpr,
            static_cast<float>(event->position().y()) * dpr
        );
        event->accept();
    }
    update();
}

void OsgVerseViewer::ViewerWidget::wheelEvent(QWheelEvent* event)
{
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
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->keyPress(event->key());
    }
    update();
}

void OsgVerseViewer::ViewerWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->keyRelease(event->key());
    }
    update();
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
            // Check for "Shape" property (present on Part::Feature and derived objects)
            App::Property* shapeProp = obj->getPropertyByName("Shape");
            auto* geoDataProp = dynamic_cast<App::PropertyComplexGeoData*>(shapeProp);

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
                            //  so we use u_baseColor uniform per geometry instead)
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
                                ss->addUniform(new osg::Uniform("u_baseColor", grp.color));
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
                                vpGroup->addChild(geode);
                                hasGeometry = true;
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
                                stateSet->addUniform(new osg::Uniform("u_baseColor", diffuseColor));
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
    if (!hasGeometry) {
        Base::Console().log("OsgVerseViewer::addViewProvider: No geometry for %s, skipping\n", objName.c_str());
        _viewProviders.erase(vp);
        return;
    }

    // 添加到场景图
    // Add to scene graph
    if (_engine) {
        osg::Group* sceneRoot = _engine->getOsgSceneRoot();
        if (sceneRoot) {
            sceneRoot->addChild(vpGroup);
            Base::Console().log("OsgVerseViewer::addViewProvider: Added to scene graph\n");
        }
    }

    // 保存映射
    // Save mapping
    _vpToNodeMap[vp] = vpGroup;
    _nodeToVPMap[vpGroup.get()] = vp;

    Base::Console().log("OsgVerseViewer::addViewProvider: Complete\n");

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
