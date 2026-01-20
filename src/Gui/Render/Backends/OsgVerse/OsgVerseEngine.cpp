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
#endif

#include "OsgVerseEngine.h"
#include "OsgVerseNode.h"
#include "OsgVerseMaterial.h"
#include "OsgVerseGeometry.h"

#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Switch>
#include <osg/Camera>
#include <osg/Light>
#include <osg/LightSource>
#include <osgViewer/Viewer>
#include <osgDB/Registry>

using namespace Gui::Render;

//===========================================================================
// OsgVerseEngine Implementation
//===========================================================================

OsgVerseEngine::OsgVerseEngine()
{
    Base::Console().log("OsgVerseEngine: Constructor START\n");
    try {
        // 构造函数中不做任何初始化
        // 所有初始化延迟到 initialize() 方法
        Base::Console().log("OsgVerseEngine: Constructor completed successfully\n");
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseEngine: Constructor exception: %s\n", e.what());
        throw;
    }
    catch (...) {
        Base::Console().error("OsgVerseEngine: Constructor unknown exception\n");
        throw;
    }
}

OsgVerseEngine::~OsgVerseEngine()
{
    shutdown();
}

//-----------------------------------------------------------------------
// Engine Information
//-----------------------------------------------------------------------

std::string OsgVerseEngine::getName() const
{
    return "OsgVerse";
}

std::string OsgVerseEngine::getVersion() const
{
    // 返回硬编码的版本号，避免调用 OSG 函数
    // Return hardcoded version to avoid calling OSG functions
    // 这样可以避免在获取信息时触发 OSG 初始化
    // This avoids triggering OSG initialization when getting info
    return "3.6.5";  // OSG version
}

BackendInfo OsgVerseEngine::getInfo() const
{
    Base::Console().log("OsgVerseEngine::getInfo: START\n");
    
    BackendInfo info;
    info.type = BackendType::OsgVerse;
    
    Base::Console().log("OsgVerseEngine::getInfo: Getting name...\n");
    info.name = getName();
    
    Base::Console().log("OsgVerseEngine::getInfo: Getting version...\n");
    info.version = getVersion();
    
    Base::Console().log("OsgVerseEngine::getInfo: Setting description...\n");
    info.description = "Modern rendering backend based on OpenSceneGraph and OsgVerse";
    info.supportsPBR = true;
    info.supportsDeferredRendering = true;
    info.supportsRaytracing = false;
    
    Base::Console().log("OsgVerseEngine::getInfo: COMPLETE\n");
    return info;
}

bool OsgVerseEngine::isAvailable() const
{
    // Check if OSG is properly initialized
    return osgDB::Registry::instance() != nullptr;
}

bool OsgVerseEngine::initialize()
{
    if (_initialized) {
        return true;
    }

    Base::Console().log("OsgVerseEngine::initialize: Starting initialization...\n");

    try {
        // Initialize OSG database
        Base::Console().log("OsgVerseEngine::initialize: Initializing OSG Registry...\n");
        osgDB::Registry::instance();
        Base::Console().log("OsgVerseEngine::initialize: OSG Registry initialized\n");

        // Create scene root
        Base::Console().log("OsgVerseEngine::initialize: Creating scene root...\n");
        _sceneRoot = new osg::Group();
        _sceneRoot->setName("OsgVerseSceneRoot");
        Base::Console().log("OsgVerseEngine::initialize: Scene root created\n");

        // Initialize rendering pipeline
        Base::Console().log("OsgVerseEngine::initialize: Initializing rendering pipeline...\n");
        initializeRenderingPipeline();
        Base::Console().log("OsgVerseEngine::initialize: Rendering pipeline initialized\n");

        _initialized = true;
        Base::Console().log("OsgVerseEngine::initialize: Initialization complete\n");
        return true;
    }
    catch (const std::exception& e) {
        // Log error
        Base::Console().error("OsgVerseEngine::initialize: Exception caught: %s\n", e.what());
        _initialized = false;
        return false;
    }
    catch (...) {
        Base::Console().error("OsgVerseEngine::initialize: Unknown exception caught\n");
        _initialized = false;
        return false;
    }
}

void OsgVerseEngine::shutdown()
{
    if (!_initialized) {
        return;
    }

    // Release resources
    if (_sceneRoot) {
        _sceneRoot->removeChildren(0, _sceneRoot->getNumChildren());
        _sceneRoot = nullptr;
    }

    _abstractSceneRoot.reset();
    _viewer = nullptr;

    _initialized = false;
}

//-----------------------------------------------------------------------
// Node Creation
//-----------------------------------------------------------------------

RenderGroup::Ptr OsgVerseEngine::createGroup()
{
    return std::make_shared<OsgVerseGroup>();
}

RenderSeparator::Ptr OsgVerseEngine::createSeparator()
{
    return std::make_shared<OsgVerseSeparator>();
}

RenderNode::Ptr OsgVerseEngine::createTransform()
{
    auto* transform = new osg::MatrixTransform();
    return std::make_shared<OsgVerseNode>(transform, true, NodeType::Transform);
}

RenderNode::Ptr OsgVerseEngine::createSwitch()
{
    auto* switchNode = new osg::Switch();
    return std::make_shared<OsgVerseNode>(switchNode, true, NodeType::Switch);
}

RenderNode::Ptr OsgVerseEngine::createMaterial()
{
    return std::make_shared<OsgVerseMaterial>();
}

RenderNode::Ptr OsgVerseEngine::createGeometry()
{
    return std::make_shared<OsgVerseGeometry>();
}

RenderNode::Ptr OsgVerseEngine::createCamera()
{
    auto* camera = new osg::Camera();
    return std::make_shared<OsgVerseNode>(camera, true, NodeType::Camera);
}

RenderNode::Ptr OsgVerseEngine::createLight(LightType type)
{
    auto* light = new osg::Light();

    switch (type) {
        case LightType::Directional:
            light->setPosition(osg::Vec4(0.0f, 0.0f, 1.0f, 0.0f));
            break;
        case LightType::Point:
            light->setPosition(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
            break;
        case LightType::Spot:
            light->setPosition(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
            light->setSpotCutoff(45.0f);
            break;
        default:
            break;
    }

    auto* lightSource = new osg::LightSource();
    lightSource->setLight(light);

    return std::make_shared<OsgVerseNode>(lightSource, true, NodeType::Light);
}

//-----------------------------------------------------------------------
// Scene Management
//-----------------------------------------------------------------------

void OsgVerseEngine::setSceneRoot(RenderNode::Ptr root)
{
    _abstractSceneRoot = root;

    if (_sceneRoot && root) {
        // Extract OSG node from abstract node
        auto* osgNode = unwrapNode(root.get());
        if (osgNode) {
            _sceneRoot->removeChildren(0, _sceneRoot->getNumChildren());
            _sceneRoot->addChild(osgNode);
        }
    }
}

RenderNode::Ptr OsgVerseEngine::getSceneRoot() const
{
    return _abstractSceneRoot;
}

void OsgVerseEngine::updateScene()
{
    if (_viewer) {
        _viewer->requestRedraw();
    }
}

//-----------------------------------------------------------------------
// Rendering Control
//-----------------------------------------------------------------------

void OsgVerseEngine::render()
{
    if (_viewer && _initialized) {
        _viewer->frame();
        updateStats();
    }
}

void OsgVerseEngine::setRenderMode(RenderMode mode)
{
    _renderMode = mode;

    // Update OSG rendering mode
    if (_sceneRoot) {
        osg::StateSet* stateSet = _sceneRoot->getOrCreateStateSet();

        switch (mode) {
            case RenderMode::Wireframe:
                stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
                break;
            case RenderMode::Points:
                stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
                break;
            case RenderMode::Flat:
                stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
                break;
            case RenderMode::BoundingBox:
                stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
                break;
            case RenderMode::Shaded:
            case RenderMode::Gouraud:
            case RenderMode::Phong:
            case RenderMode::Default:
            default:
                stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
                break;
        }
    }
}

RenderMode OsgVerseEngine::getRenderMode() const
{
    return _renderMode;
}

void OsgVerseEngine::setBackgroundColor(const Color& color)
{
    _backgroundColor = color;

    if (_viewer) {
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            camera->setClearColor(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
}

Color OsgVerseEngine::getBackgroundColor() const
{
    return _backgroundColor;
}

RenderStats OsgVerseEngine::getStats() const
{
    return _stats;
}

void OsgVerseEngine::resetStats()
{
    _stats = RenderStats();
}

//-----------------------------------------------------------------------
// Resource Management
//-----------------------------------------------------------------------

void OsgVerseEngine::releaseUnusedResources()
{
    if (_viewer) {
        // Release GL objects
        _viewer->getCamera()->releaseGLObjects();
    }
}

size_t OsgVerseEngine::getMemoryUsage() const
{
    // TODO: Implement proper memory usage calculation
    return 0;
}

//-----------------------------------------------------------------------
// OsgVerse-specific Interface
//-----------------------------------------------------------------------

RenderNode::Ptr OsgVerseEngine::wrapNode(osg::Node* osgNode, bool takeOwnership)
{
    if (!osgNode) {
        return nullptr;
    }
    return std::make_shared<OsgVerseNode>(osgNode, takeOwnership, NodeType::Node);
}

osg::Node* OsgVerseEngine::unwrapNode(RenderNode* renderNode) const
{
    if (!renderNode) {
        return nullptr;
    }

    // Try to cast to OsgVerseNode
    auto* osgVerseNode = dynamic_cast<OsgVerseNode*>(renderNode);
    if (osgVerseNode) {
        return osgVerseNode->getOsgNode();
    }

    return nullptr;
}

void OsgVerseEngine::setViewer(osgViewer::Viewer* viewer)
{
    _viewer = viewer;

    if (_viewer && _sceneRoot) {
        _viewer->setSceneData(_sceneRoot);
    }
}

void OsgVerseEngine::setPBREnabled(bool enabled)
{
    _pbrEnabled = enabled;
    // TODO: Update rendering pipeline
}

void OsgVerseEngine::setHDREnabled(bool enabled)
{
    _hdrEnabled = enabled;
    // TODO: Update rendering pipeline
}

void OsgVerseEngine::setAntiAliasingMode(int samples)
{
    _aaSamples = samples;

    if (_viewer) {
        osg::DisplaySettings* ds = _viewer->getDisplaySettings();
        if (ds) {
            ds->setNumMultiSamples(samples);
        }
    }
}

void OsgVerseEngine::setShadowsEnabled(bool enabled)
{
    _shadowsEnabled = enabled;
    // TODO: Update shadow rendering
}

void OsgVerseEngine::setShadowQuality(int quality)
{
    _shadowQuality = quality;
    // TODO: Update shadow quality settings
}

//-----------------------------------------------------------------------
// Private Methods
//-----------------------------------------------------------------------

void OsgVerseEngine::initializeRenderingPipeline()
{
    // Setup default rendering states
    if (_sceneRoot) {
        osg::StateSet* stateSet = _sceneRoot->getOrCreateStateSet();
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    }

    // Setup post-processing if enabled
    if (_hdrEnabled || _pbrEnabled) {
        setupPostProcessing();
    }
}

void OsgVerseEngine::setupPostProcessing()
{
    // TODO: Implement post-processing setup
    // - HDR tone mapping
    // - SSAO
    // - Bloom
    // - TAA
}

void OsgVerseEngine::updateStats()
{
    if (!_viewer) {
        return;
    }

    // Update frame statistics
    osgViewer::ViewerBase::Scenes scenes;
    _viewer->getScenes(scenes);
    if (!scenes.empty()) {
        osg::Node* sceneData = scenes[0]->getSceneData();
        if (sceneData) {
            // Count nodes and drawables
            // TODO: Implement proper statistics gathering
        }
    }
}

//===========================================================================
// Engine Registration
//===========================================================================

// 注意：不使用静态自动注册，因为它会在 DLL 加载时执行，可能导致初始化问题
// Note: Don't use static auto-registration as it executes during DLL load and may cause initialization issues
// 改为在 RenderManager 初始化时手动注册
// Instead, manually register during RenderManager initialization

namespace Gui {
namespace Render {
    // 手动注册函数，由 RenderManager 调用
    // Manual registration function, called by RenderManager
    void registerOsgVerseEngine() {
        Base::Console().log("registerOsgVerseEngine: START\n");
        try {
            Base::Console().log("registerOsgVerseEngine: Getting factory instance...\n");
            auto& factory = RenderEngineFactory::instance();
            
            Base::Console().log("registerOsgVerseEngine: Registering OsgVerse engine...\n");
            factory.registerEngine(
                BackendType::OsgVerse,
                []() -> RenderEngine::Ptr {
                    Base::Console().log("registerOsgVerseEngine: Creating OsgVerseEngine instance...\n");
                    return std::make_shared<OsgVerseEngine>();
                }
            );
            
            Base::Console().log("registerOsgVerseEngine: Registration complete\n");
            
            // Verify registration
            bool isRegistered = factory.isEngineRegistered(BackendType::OsgVerse);
            Base::Console().log("registerOsgVerseEngine: Verification: %s\n", 
                                isRegistered ? "SUCCESS" : "FAILED");
        }
        catch (const std::exception& e) {
            Base::Console().error("registerOsgVerseEngine: Exception: %s\n", e.what());
            throw;
        }
        catch (...) {
            Base::Console().error("registerOsgVerseEngine: Unknown exception\n");
            throw;
        }
    }
}
}
