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

#include "OsgVerseViewerImpl.h"
#include <Base/Console.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <App/DocumentObject.h>
#include <osg/Version>
#include <osg/Camera>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/MatrixTransform>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osgGA/TrackballManipulator>
#include <osgViewer/GraphicsWindow>

using namespace Gui::View3D::OsgVerse;

// 引入 View3D 命名空间的类型
using Gui::View3D::CameraParams;
using Gui::View3D::PickResult;
using Gui::View3D::RenderMode;
using Gui::View3D::SelectionMode;
using Gui::View3D::RenderMode;
using Gui::View3D::SelectionMode;

//===========================================================================
// OsgVerseViewerImpl::ViewerWidget Implementation
//===========================================================================

OsgVerseViewerImpl::ViewerWidget::ViewerWidget(osgViewer::Viewer* viewer, QWidget* parent)
    : QOpenGLWidget(parent)
    , _viewer(viewer)
{
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Creating widget\n");
    
    // 设置 OpenGL 格式 - 使用兼容性配置
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);  // 4x MSAA
    format.setVersion(2, 1);  // 降低到 OpenGL 2.1 以提高兼容性
    format.setProfile(QSurfaceFormat::CompatibilityProfile);  // 使用兼容性配置
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    setFormat(format);
    
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: OpenGL format set (v2.1, Compatibility)\n");
    
    // 设置焦点策略
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // 设置更新策略 - 确保持续更新
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
}

OsgVerseViewerImpl::ViewerWidget::~ViewerWidget()
{
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Destroying widget\n");
}

void OsgVerseViewerImpl::ViewerWidget::initializeGL()
{
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: ===== initializeGL CALLED =====\n");
    
    if (!_viewer) {
        Base::Console().log("OsgVerseViewerImpl::ViewerWidget: ERROR - Viewer is null!\n");
        return;
    }
    
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Creating GraphicsWindow...\n");
    
    // 创建 GraphicsWindow::Traits
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->x = 0;
    traits->y = 0;
    traits->width = width();
    traits->height = height();
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = nullptr;
    traits->vsync = true;
    
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Traits: %dx%d\n", traits->width, traits->height);
    
    // 创建 GraphicsWindowEmbedded
    _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(traits.get());
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: GraphicsWindowEmbedded created\n");
    
    // 设置 viewer 的 graphics context
    _viewer->getCamera()->setGraphicsContext(_graphicsWindow.get());
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Graphics context set\n");
    
    // 设置视口
    _viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width(), height()));
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Viewport set\n");
    
    // 重要：初始化 viewer（这会设置渲染状态）
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Checking if viewer is realized...\n");
    if (!_viewer->isRealized()) {
        Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Calling realize()...\n");
        _viewer->realize();
        Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Viewer realized!\n");
    }
    else {
        Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Viewer already realized\n");
    }
    
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: ===== initializeGL COMPLETE =====\n");
}

void OsgVerseViewerImpl::ViewerWidget::paintGL()
{
    static int frameCount = 0;
    
    // 如果 GraphicsWindow 还没有创建，先初始化
    if (!_graphicsWindow && _viewer) {
        Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Initializing GraphicsWindow in paintGL...\n");
        
        // 创建 GraphicsWindow::Traits
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
        traits->x = 0;
        traits->y = 0;
        traits->width = width();
        traits->height = height();
        traits->windowDecoration = false;
        traits->doubleBuffer = true;
        traits->sharedContext = nullptr;
        traits->vsync = true;
        
        Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Creating GraphicsWindow %dx%d\n",
                           traits->width, traits->height);
        
        // 创建 GraphicsWindowEmbedded
        _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(traits.get());
        
        // 设置 viewer 的 graphics context
        _viewer->getCamera()->setGraphicsContext(_graphicsWindow.get());
        
        // 设置视口 - 使用当前 widget 的实际大小
        _viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width(), height()));
        
        // 更新投影矩阵
        double aspectRatio = static_cast<double>(width()) / static_cast<double>(height());
        _viewer->getCamera()->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.01, 10000.0);
        
        // 初始化 viewer
        if (!_viewer->isRealized()) {
            _viewer->realize();
            Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Viewer realized\n");
        }
        
        Base::Console().log("OsgVerseViewerImpl::ViewerWidget: GraphicsWindow initialized\n");
    }
    
    if (_viewer && _graphicsWindow) {
        // 渲染一帧
        _viewer->frame();
    }
}

void OsgVerseViewerImpl::ViewerWidget::resizeGL(int width, int height)
{
    Base::Console().log("OsgVerseViewerImpl::ViewerWidget: Resize to %dx%d\n", width, height);
    
    if (_graphicsWindow) {
        _graphicsWindow->resized(0, 0, width, height);
    }
    
    if (_viewer && _viewer->getCamera()) {
        _viewer->getCamera()->setViewport(0, 0, width, height);
        
        // 更新投影矩阵
        double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
        _viewer->getCamera()->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.01, 10000.0);
    }
}

void OsgVerseViewerImpl::ViewerWidget::mousePressEvent(QMouseEvent* event)
{
    // TODO: 在 Phase 3 实现鼠标事件处理
    QOpenGLWidget::mousePressEvent(event);
}

void OsgVerseViewerImpl::ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QOpenGLWidget::mouseReleaseEvent(event);
}

void OsgVerseViewerImpl::ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
    QOpenGLWidget::mouseMoveEvent(event);
}

void OsgVerseViewerImpl::ViewerWidget::wheelEvent(QWheelEvent* event)
{
    QOpenGLWidget::wheelEvent(event);
}

void OsgVerseViewerImpl::ViewerWidget::keyPressEvent(QKeyEvent* event)
{
    QOpenGLWidget::keyPressEvent(event);
}

void OsgVerseViewerImpl::ViewerWidget::keyReleaseEvent(QKeyEvent* event)
{
    QOpenGLWidget::keyReleaseEvent(event);
}

//===========================================================================
// OsgVerseViewerImpl Implementation
//===========================================================================

OsgVerseViewerImpl::OsgVerseViewerImpl(QWidget* parent, const QOpenGLWidget* shareWidget)
    : _widget(nullptr)
    , _viewer(nullptr)
    , _sceneRoot(nullptr)
    , _backgroundColor(0.2f, 0.2f, 0.3f, 1.0f)
    , _renderMode(RenderMode::Shaded)
    , _initialized(false)
    , _orthographic(false)
{
    Base::Console().log("OsgVerseViewerImpl: Creating OsgVerse viewer (Phase 2 - Basic Rendering)\n");
    
    // 创建 OSG viewer
    _viewer = new osgViewer::Viewer();
    
    // 创建场景根节点
    _sceneRoot = new osg::Group();
    _sceneRoot->setName("FreeCAD_SceneRoot");
    
    // 创建 ViewProvider 容器节点
    _vpContainerNode = new osg::Group();
    _vpContainerNode->setName("ViewProviders");
    
    // 确保容器节点的渲染状态正确
    osg::StateSet* vpStateSet = _vpContainerNode->getOrCreateStateSet();
    vpStateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    vpStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    _sceneRoot->addChild(_vpContainerNode.get());
    
    Base::Console().log("OsgVerseViewerImpl: ViewProvider container created\n");
    
    // 设置场景数据
    _viewer->setSceneData(_sceneRoot.get());
    
    // 创建 Qt widget
    _widget = new ViewerWidget(_viewer.get(), parent);
    
    // 初始化 viewer
    initializeViewer();
    
    Base::Console().log("OsgVerseViewerImpl: OsgVerse viewer created successfully\n");
}

OsgVerseViewerImpl::~OsgVerseViewerImpl()
{
    Base::Console().log("OsgVerseViewerImpl: Destroying OsgVerse viewer\n");
    
    // 先清空 widget 的 viewer 指针，防止在销毁过程中访问
    if (_widget) {
        _widget->setViewer(nullptr);
    }
    
    // 清空 viewer（OSG 使用智能指针自动管理）
    _viewer = nullptr;
    _sceneRoot = nullptr;
    
    // Qt 会自动删除 widget
}

//-----------------------------------------------------------------------
// 基础渲染接口
//-----------------------------------------------------------------------

void OsgVerseViewerImpl::render()
{
    if (_widget) {
        _widget->update();
    }
}

void OsgVerseViewerImpl::resize(int width, int height)
{
    if (_widget) {
        _widget->resize(width, height);
    }
}

QWidget* OsgVerseViewerImpl::getWidget()
{
    return _widget;
}

QOpenGLWidget* OsgVerseViewerImpl::getGLWidget()
{
    return _widget;
}

//-----------------------------------------------------------------------
// 场景管理
//-----------------------------------------------------------------------

void OsgVerseViewerImpl::setSceneGraph(void* root)
{
    if (root) {
        auto* node = static_cast<osg::Node*>(root);
        _sceneRoot->removeChildren(0, _sceneRoot->getNumChildren());
        _sceneRoot->addChild(node);
        Base::Console().log("OsgVerseViewerImpl: Scene graph set\n");
    }
}

void* OsgVerseViewerImpl::getSceneGraph()
{
    return _sceneRoot.get();
}

void OsgVerseViewerImpl::updateScene()
{
    render();
}

//-----------------------------------------------------------------------
// 相机控制
//-----------------------------------------------------------------------

void OsgVerseViewerImpl::setCamera(const CameraParams& params)
{
    if (!_viewer || !_viewer->getCamera()) {
        return;
    }
    
    osg::Camera* camera = _viewer->getCamera();
    
    // 设置视图矩阵
    osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
    osg::Vec3d center(params.target.x, params.target.y, params.target.z);
    osg::Vec3d up(params.upVector.x, params.upVector.y, params.upVector.z);
    
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 设置投影矩阵
    if (params.orthographic) {
        double halfHeight = params.height / 2.0;
        double halfWidth = halfHeight * params.aspectRatio;
        camera->setProjectionMatrixAsOrtho(-halfWidth, halfWidth, -halfHeight, halfHeight,
                                           params.nearPlane, params.farPlane);
        _orthographic = true;
    } else {
        camera->setProjectionMatrixAsPerspective(params.fieldOfView, params.aspectRatio,
                                                 params.nearPlane, params.farPlane);
        _orthographic = false;
    }
}

CameraParams OsgVerseViewerImpl::getCamera() const
{
    CameraParams params;
    
    if (!_viewer || !_viewer->getCamera()) {
        return params;
    }
    
    osg::Camera* camera = _viewer->getCamera();
    
    // 获取视图矩阵
    osg::Vec3d eye, center, up;
    camera->getViewMatrixAsLookAt(eye, center, up);
    
    params.position = Base::Vector3d(eye.x(), eye.y(), eye.z());
    params.target = Base::Vector3d(center.x(), center.y(), center.z());
    params.upVector = Base::Vector3d(up.x(), up.y(), up.z());
    
    // 获取投影矩阵参数
    params.orthographic = _orthographic;
    
    if (_orthographic) {
        double left, right, bottom, top, zNear, zFar;
        camera->getProjectionMatrixAsOrtho(left, right, bottom, top, zNear, zFar);
        params.height = top - bottom;
        params.aspectRatio = (right - left) / (top - bottom);
        params.nearPlane = zNear;
        params.farPlane = zFar;
    } else {
        double fovy, aspectRatio, zNear, zFar;
        camera->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
        params.fieldOfView = fovy;
        params.aspectRatio = aspectRatio;
        params.nearPlane = zNear;
        params.farPlane = zFar;
    }
    
    return params;
}

void OsgVerseViewerImpl::viewAll()
{
    if (!_viewer || !_sceneRoot) {
        return;
    }
    
    // 计算场景包围盒
    osg::ComputeBoundsVisitor cbv;
    _sceneRoot->accept(cbv);
    osg::BoundingBox bb = cbv.getBoundingBox();
    
    if (!bb.valid()) {
        // 场景为空或边界框无效，使用默认相机位置
        Base::Console().log("OsgVerseViewerImpl: Scene is empty, using default camera position\n");
        resetCamera();
        return;
    }
    
    // 计算包围球
    osg::BoundingSphere bs;
    bs.expandBy(bb);
    
    // 设置相机位置 - 增加距离以避免裁剪
    osg::Vec3d center = bs.center();
    double radius = bs.radius();
    double distance = radius / std::tan(osg::DegreesToRadians(45.0 / 2.0));
    
    // 增加距离系数从 1.5 到 2.5，确保整个对象可见
    osg::Vec3d eye = center + osg::Vec3d(0.0, -distance * 2.5, radius * 0.8);
    osg::Vec3d up(0.0, 0.0, 1.0);
    
    _viewer->getCamera()->setViewMatrixAsLookAt(eye, center, up);
    
    Base::Console().log("OsgVerseViewerImpl: View all - center(%.2f, %.2f, %.2f) radius=%.2f\n",
                        center.x(), center.y(), center.z(), radius);
    Base::Console().log("OsgVerseViewerImpl: Camera eye(%.2f, %.2f, %.2f) distance=%.2f\n",
                        eye.x(), eye.y(), eye.z(), distance * 2.5);
    
    // 强制更新
    updateScene();
}

void OsgVerseViewerImpl::resetCamera()
{
    if (!_viewer) {
        return;
    }
    
    // 重置到默认相机位置 - 更远的距离
    osg::Vec3d eye(0.0, -20.0, 10.0);
    osg::Vec3d center(0.0, 0.0, 0.0);
    osg::Vec3d up(0.0, 0.0, 1.0);
    
    _viewer->getCamera()->setViewMatrixAsLookAt(eye, center, up);
    
    Base::Console().log("OsgVerseViewerImpl: Camera reset to default position\n");
}

void OsgVerseViewerImpl::setCameraType(bool orthographic)
{
    _orthographic = orthographic;
    
    if (!_viewer || !_viewer->getCamera()) {
        return;
    }
    
    osg::Camera* camera = _viewer->getCamera();
    
    if (orthographic) {
        // 切换到正交投影
        double height = 10.0;
        double aspectRatio = static_cast<double>(_widget->width()) / static_cast<double>(_widget->height());
        double halfHeight = height / 2.0;
        double halfWidth = halfHeight * aspectRatio;
        camera->setProjectionMatrixAsOrtho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1, 1000.0);
        Base::Console().log("OsgVerseViewerImpl: Switched to orthographic projection\n");
    } else {
        // 切换到透视投影
        double aspectRatio = static_cast<double>(_widget->width()) / static_cast<double>(_widget->height());
        camera->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.1, 1000.0);
        Base::Console().log("OsgVerseViewerImpl: Switched to perspective projection\n");
    }
}

bool OsgVerseViewerImpl::isCameraOrthographic() const
{
    return _orthographic;
}

//-----------------------------------------------------------------------
// 事件处理
//-----------------------------------------------------------------------

bool OsgVerseViewerImpl::handleMouseEvent(QMouseEvent* event)
{
    // Phase 3 实现
    return false;
}

bool OsgVerseViewerImpl::handleKeyEvent(QKeyEvent* event)
{
    return false;
}

bool OsgVerseViewerImpl::handleWheelEvent(QWheelEvent* event)
{
    return false;
}

//-----------------------------------------------------------------------
// 拾取和选择
//-----------------------------------------------------------------------

PickResult OsgVerseViewerImpl::pick(const QPoint& pos)
{
    PickResult result;
    // Phase 3 实现
    return result;
}

//-----------------------------------------------------------------------
// 渲染设置
//-----------------------------------------------------------------------

void OsgVerseViewerImpl::setRenderMode(RenderMode mode)
{
    _renderMode = mode;
}

RenderMode OsgVerseViewerImpl::getRenderMode() const
{
    return _renderMode;
}

void OsgVerseViewerImpl::setBackgroundColor(const Base::Color& color)
{
    _backgroundColor = color;
    
    if (_viewer && _viewer->getCamera()) {
        osg::Vec4 clearColor(color.r, color.g, color.b, color.a);
        _viewer->getCamera()->setClearColor(clearColor);
        Base::Console().log("OsgVerseViewerImpl: Background color set to (%.2f, %.2f, %.2f, %.2f)\n",
                            color.r, color.g, color.b, color.a);
    }
}

Base::Color OsgVerseViewerImpl::getBackgroundColor() const
{
    return _backgroundColor;
}

//-----------------------------------------------------------------------
// 后端信息
//-----------------------------------------------------------------------

std::string OsgVerseViewerImpl::getBackendVersion() const
{
    return std::string("OSG ") + osgGetVersion();
}

//-----------------------------------------------------------------------
// 统计和调试
//-----------------------------------------------------------------------

Gui::Render::RenderStats OsgVerseViewerImpl::getStats() const
{
    Gui::Render::RenderStats stats;
    stats.frameCount = 0;
    stats.drawCalls = 0;
    stats.triangleCount = 0;
    stats.vertexCount = 0;
    stats.frameTime = 0.0;
    stats.fps = 0.0;
    return stats;
}

//-----------------------------------------------------------------------
// 高级功能
//-----------------------------------------------------------------------

QImage OsgVerseViewerImpl::grabImage(int width, int height)
{
    if (_widget) {
        return _widget->grabFramebuffer();
    }
    return QImage();
}

bool OsgVerseViewerImpl::saveScreenshot(const QString& filename, int width, int height)
{
    QImage img = grabImage(width, height);
    if (!img.isNull()) {
        return img.save(filename);
    }
    return false;
}

//-----------------------------------------------------------------------
// 私有方法
//-----------------------------------------------------------------------

void OsgVerseViewerImpl::initializeViewer()
{
    if (!_viewer) {
        return;
    }
    
    Base::Console().log("OsgVerseViewerImpl: Initializing viewer...\n");
    
    // 设置线程模型
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // 设置默认相机
    setupDefaultCamera();
    
    // 设置默认光照
    setupDefaultLighting();
    
    // 设置背景颜色
    _viewer->getCamera()->setClearColor(osg::Vec4(_backgroundColor.r, _backgroundColor.g, 
                                                   _backgroundColor.b, _backgroundColor.a));
    
    // 禁用默认的事件处理器
    _viewer->setKeyEventSetsDone(0);
    
    _initialized = true;
    
    Base::Console().log("OsgVerseViewerImpl: Viewer initialized successfully\n");
}

void OsgVerseViewerImpl::setupDefaultCamera()
{
    if (!_viewer) {
        return;
    }
    
    osg::Camera* camera = _viewer->getCamera();
    
    // 设置默认视图矩阵 - 相机位置更远，避免裁剪
    osg::Vec3d eye(0.0, -20.0, 10.0);  // 从 -10, 5 改为 -20, 10
    osg::Vec3d center(0.0, 0.0, 0.0);
    osg::Vec3d up(0.0, 0.0, 1.0);
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 设置默认投影矩阵（透视）
    double aspectRatio = 1.0;
    if (_widget) {
        aspectRatio = static_cast<double>(_widget->width()) / static_cast<double>(_widget->height());
    }
    camera->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.01, 10000.0);  // 近裁剪面从 0.1 改为 0.01
    
    // 设置清除掩码
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    Base::Console().log("OsgVerseViewerImpl: Default camera setup complete\n");
}

void OsgVerseViewerImpl::setupDefaultLighting()
{
    if (!_sceneRoot) {
        return;
    }
    
    // 创建光源
    osg::ref_ptr<osg::Light> light = new osg::Light();
    light->setLightNum(0);
    light->setPosition(osg::Vec4(0.0, 0.0, 10.0, 1.0));  // 位置光
    light->setAmbient(osg::Vec4(0.2, 0.2, 0.2, 1.0));
    light->setDiffuse(osg::Vec4(0.8, 0.8, 0.8, 1.0));
    light->setSpecular(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    
    osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource();
    lightSource->setLight(light.get());
    
    // 添加到场景根节点
    _sceneRoot->addChild(lightSource.get());
    
    // 启用光照和深度测试
    osg::StateSet* stateSet = _sceneRoot->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    Base::Console().log("OsgVerseViewerImpl: Default lighting setup complete\n");
}


//-----------------------------------------------------------------------
// ViewProvider 管理
//-----------------------------------------------------------------------

void OsgVerseViewerImpl::addViewProvider(ViewProvider* vp)
{
    if (!vp || hasViewProvider(vp)) {
        return;
    }
    
    // 尝试获取对象名称（如果是 ViewProviderDocumentObject）
    const char* objName = "unknown";
    auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
    if (vpDoc && vpDoc->getObject()) {
        objName = vpDoc->getObject()->getNameInDocument();
    }
    
    Base::Console().log("OsgVerseViewerImpl: Adding ViewProvider (%s)\n", objName);
    
    _viewProviders.push_back(vp);
    
    // 创建占位符节点（Phase 1: 简单的几何体表示）
    osg::ref_ptr<osg::Group> vpNode = new osg::Group();
    vpNode->setName(objName);
    
    // 添加简单的占位符几何体（一个球体）
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    // 使用非常大的球体（半径 5.0），确保能看到
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0, 0, 0), 5.0f);
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere.get());
    
    // 设置颜色（使用非常明亮的红色，最大亮度）
    drawable->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));  // 纯红色
    
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));  // 纯红色
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.5f, 0.0f, 0.0f, 1.0f));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    material->setShininess(osg::Material::FRONT_AND_BACK, 64.0f);
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2f, 0.0f, 0.0f, 1.0f));  // 添加自发光
    
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setAttribute(material.get());
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    
    geode->addDrawable(drawable.get());
    
    // 确保边界框被正确计算
    geode->dirtyBound();
    geode->computeBound();
    
    vpNode->addChild(geode.get());
    
    Base::Console().log("OsgVerseViewerImpl: Created LARGE RED sphere with radius 5.0 at origin\n");
    
    // 保存映射关系
    _vpNodeMap[vp] = vpNode;
    
    // 添加到场景
    if (_vpContainerNode) {
        _vpContainerNode->addChild(vpNode.get());
        
        // 强制更新边界框
        _vpContainerNode->dirtyBound();
        _sceneRoot->dirtyBound();
        
        Base::Console().log("OsgVerseViewerImpl: Added node to container (total children: %d)\n", 
                           _vpContainerNode->getNumChildren());
        Base::Console().log("OsgVerseViewerImpl: Scene root has %d children\n",
                           _sceneRoot->getNumChildren());
    }
    else {
        Base::Console().log("OsgVerseViewerImpl: WARNING - _vpContainerNode is null!\n");
    }
    
    updateScene();
    
    Base::Console().log("OsgVerseViewerImpl: ViewProvider added successfully\n");
}

void OsgVerseViewerImpl::removeViewProvider(ViewProvider* vp)
{
    auto it = std::find(_viewProviders.begin(), _viewProviders.end(), vp);
    if (it == _viewProviders.end()) {
        return;
    }
    
    Base::Console().log("OsgVerseViewerImpl: Removing ViewProvider\n");
    
    _viewProviders.erase(it);
    
    // 从场景中移除节点
    auto nodeIt = _vpNodeMap.find(vp);
    if (nodeIt != _vpNodeMap.end()) {
        if (_vpContainerNode) {
            _vpContainerNode->removeChild(nodeIt->second.get());
        }
        _vpNodeMap.erase(nodeIt);
    }
    
    updateScene();
    
    Base::Console().log("OsgVerseViewerImpl: ViewProvider removed successfully\n");
}

bool OsgVerseViewerImpl::hasViewProvider(ViewProvider* vp) const
{
    return std::find(_viewProviders.begin(), _viewProviders.end(), vp) != _viewProviders.end();
}

std::vector<Gui::ViewProvider*> OsgVerseViewerImpl::getViewProviders() const
{
    return _viewProviders;
}
