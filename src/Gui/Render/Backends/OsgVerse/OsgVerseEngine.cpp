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
#include <set>
#endif

#include "OsgVerseEngine.h"
#include "OsgVerseNode.h"
#include "OsgVerseMaterial.h"
#include "OsgVerseGeometry.h"
#include "OsgVerseShadowMap.h"
#include "OsgVersePostProcessing.h"
#include "OsgVerseShaderManager.h"
#include "OsgVerseBloom.h"
#include "OsgVerseTAA.h"
#include "OsgVerseSSAO.h"

#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Switch>
#include <osg/Camera>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/NodeVisitor>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/Array>
#include <osg/Stats>
#include <osg/PrimitiveSet>
#include <osgViewer/Viewer>
#include <osgDB/Registry>

using namespace Gui::Render;

namespace {

// Visitor that traverses the scene graph to collect geometry statistics
class StatsVisitor : public osg::NodeVisitor {
public:
    uint32_t nodeCount{0};
    uint32_t geodeCount{0};
    uint32_t geometryCount{0};
    uint32_t vertexCount{0};
    uint32_t triangleCount{0};
    uint32_t drawCalls{0};

    StatsVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {}

    void apply(osg::Node& node) override
    {
        ++nodeCount;
        traverse(node);
    }

    void apply(osg::Geode& geode) override
    {
        ++nodeCount;
        ++geodeCount;
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i) {
            auto* geom = geode.getDrawable(i)->asGeometry();
            if (geom) {
                countGeometry(geom);
            }
        }
        traverse(geode);
    }

    void apply(osg::Group& group) override
    {
        ++nodeCount;
        // Check children that are Drawables attached via osg::Geometry directly
        for (unsigned int i = 0; i < group.getNumChildren(); ++i) {
            auto* geode = dynamic_cast<osg::Geode*>(group.getChild(i));
            if (!geode) {
                // In newer OSG, Geometry can be a child of Group directly
                auto* geom = dynamic_cast<osg::Geometry*>(group.getChild(i));
                if (geom) {
                    ++nodeCount;
                    countGeometry(geom);
                }
            }
        }
        traverse(group);
    }

private:
    void countGeometry(osg::Geometry* geom)
    {
        ++geometryCount;
        ++drawCalls;

        if (auto* verts = geom->getVertexArray()) {
            vertexCount += verts->getNumElements();
        }

        for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); ++i) {
            auto* pset = geom->getPrimitiveSet(i);
            if (!pset) continue;

            switch (pset->getMode()) {
                case GL_TRIANGLES:
                    triangleCount += pset->getNumIndices() / 3;
                    break;
                case GL_TRIANGLE_STRIP:
                case GL_TRIANGLE_FAN:
                    if (pset->getNumIndices() >= 3)
                        triangleCount += pset->getNumIndices() - 2;
                    break;
                case GL_QUADS:
                    triangleCount += (pset->getNumIndices() / 4) * 2;
                    break;
                case GL_QUAD_STRIP:
                    if (pset->getNumIndices() >= 4)
                        triangleCount += ((pset->getNumIndices() - 2) / 2) * 2;
                    break;
                default:
                    break;
            }
        }
    }
};

// Visitor that estimates GPU memory usage from geometry and texture data
class MemoryVisitor : public osg::NodeVisitor {
public:
    uint64_t vboMemory{0};
    uint64_t textureMemory{0};

    MemoryVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {}

    void apply(osg::Node& node) override
    {
        collectTextures(node.getStateSet());
        traverse(node);
    }

    void apply(osg::Geode& geode) override
    {
        collectTextures(geode.getStateSet());
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i) {
            auto* geom = geode.getDrawable(i)->asGeometry();
            if (geom) {
                collectGeometryMemory(geom);
            }
        }
        traverse(geode);
    }

private:
    std::set<const osg::Array*> _countedArrays;
    std::set<const osg::Texture*> _countedTextures;

    static uint64_t arrayBytes(const osg::Array* arr)
    {
        if (!arr) return 0;
        return static_cast<uint64_t>(arr->getNumElements()) * arr->getElementSize();
    }

    void countArray(const osg::Array* arr)
    {
        if (!arr || _countedArrays.count(arr)) return;
        _countedArrays.insert(arr);
        vboMemory += arrayBytes(arr);
    }

    void collectGeometryMemory(osg::Geometry* geom)
    {
        collectTextures(geom->getStateSet());
        countArray(geom->getVertexArray());
        countArray(geom->getNormalArray());
        countArray(geom->getColorArray());
        for (unsigned int i = 0; i < geom->getNumTexCoordArrays(); ++i) {
            countArray(geom->getTexCoordArray(i));
        }
        for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); ++i) {
            auto* pset = geom->getPrimitiveSet(i);
            if (auto* drawElements = dynamic_cast<const osg::DrawElements*>(pset)) {
                // Index buffer size
                vboMemory += static_cast<uint64_t>(drawElements->getTotalDataSize());
            }
        }
    }

    void collectTextures(const osg::StateSet* ss)
    {
        if (!ss) return;
        for (unsigned int i = 0; i < ss->getTextureAttributeList().size(); ++i) {
            auto* tex = dynamic_cast<const osg::Texture2D*>(
                ss->getTextureAttribute(i, osg::StateAttribute::TEXTURE));
            if (!tex || _countedTextures.count(tex)) continue;
            _countedTextures.insert(tex);
            if (auto* img = tex->getImage()) {
                textureMemory += static_cast<uint64_t>(img->getTotalSizeInBytes());
            }
        }
    }
};

} // anonymous namespace

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

        // Note: initializeRenderingPipeline() is deferred until setViewer() is called,
        // because post-processing passes create FBO cameras that require a GL context.

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
    if (_taa) {
        _taa->shutdown();
        _taa.reset();
    }

    if (_bloom) {
        _bloom->shutdown();
        _bloom.reset();
    }

    if (_ssao) {
        _ssao->shutdown();
        _ssao.reset();
    }

    if (_shadowMap) {
        _shadowMap->shutdown();
        _shadowMap.reset();
    }

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
        // TAA: apply jitter before rendering
        if (_taa && _taa->isEnabled()) {
            _taa->applyJitter(_viewer->getCamera(), _frameCount);
        }

        _viewer->frame();

        // TAA: update history after rendering
        if (_taa && _taa->isEnabled() && _viewer->getCamera()) {
            osg::Camera* cam = _viewer->getCamera();
            osg::Matrix currentMVP = cam->getViewMatrix() * cam->getProjectionMatrix();
            _taa->update(currentMVP, _prevMVP);
            _prevMVP = currentMVP;
        }

        ++_frameCount;
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
    _framesSinceLastTraversal = 0;
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
    if (!_sceneRoot) {
        return 0;
    }

    MemoryVisitor mv;
    _sceneRoot->accept(mv);
    return static_cast<size_t>(mv.vboMemory + mv.textureMemory);
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

        // Now that viewer and GL context are available, initialize rendering pipeline
        // (post-processing passes create FBO cameras that need GL context)
        Base::Console().log("OsgVerseEngine::setViewer: Initializing rendering pipeline...\n");
        initializeRenderingPipeline();
        Base::Console().log("OsgVerseEngine::setViewer: Rendering pipeline initialized\n");
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
    if (_postProcessChain) {
        _postProcessChain->setEnabled(enabled);
    }
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
    if (_shadowMap) {
        _shadowMap->setEnabled(enabled);
    }
}

void OsgVerseEngine::setShadowQuality(int quality)
{
    _shadowQuality = quality;
    if (_shadowMap) {
        _shadowMap->setQuality(quality);
    }
}

void OsgVerseEngine::setBloomEnabled(bool enabled)
{
    if (_bloom) {
        _bloom->setEnabled(enabled);
    }
}

bool OsgVerseEngine::isBloomEnabled() const
{
    return _bloom && _bloom->isEnabled();
}

void OsgVerseEngine::setBloomThreshold(float threshold)
{
    if (_bloom) {
        _bloom->setThreshold(threshold);
    }
}

void OsgVerseEngine::setBloomIntensity(float intensity)
{
    if (_bloom) {
        _bloom->setIntensity(intensity);
    }
}

void OsgVerseEngine::setTAAEnabled(bool enabled)
{
    if (_taa) {
        _taa->setEnabled(enabled);
    }
}

bool OsgVerseEngine::isTAAEnabled() const
{
    return _taa && _taa->isEnabled();
}

void OsgVerseEngine::setTAABlendFactor(float factor)
{
    if (_taa) {
        _taa->setBlendFactor(factor);
    }
}

void OsgVerseEngine::setSSAOEnabled(bool enabled)
{
    if (_ssao) {
        _ssao->setEnabled(enabled);
    }
}

bool OsgVerseEngine::isSSAOEnabled() const
{
    return _ssao && _ssao->isEnabled();
}

void OsgVerseEngine::setSSAORadius(float radius)
{
    if (_ssao) {
        _ssao->setRadius(radius);
    }
}

void OsgVerseEngine::setSSAOIntensity(float intensity)
{
    if (_ssao) {
        _ssao->setIntensity(intensity);
    }
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

    // TODO: Shadow mapping and post-processing are disabled for now.
    // They create FBO cameras that cause crashes during OSG scene compilation.
    // Need to investigate proper initialization timing with GL context.
    // See: OsgVerseShadowMap, PostProcessChain, OsgVerseSSAO, OsgVerseBloom, OsgVerseTAA
}

void OsgVerseEngine::setupPostProcessing()
{
    if (!_sceneRoot) return;

    // Create post-processing chain
    _postProcessChain = std::make_unique<PostProcessChain>();

    // Use a default viewport size; will be resized when viewer is set
    int width = 1024;
    int height = 768;

    if (!_postProcessChain->initialize(width, height, _sceneRoot)) {
        Base::Console().warning("OsgVerseEngine: Failed to initialize post-processing chain\n");
        _postProcessChain.reset();
        return;
    }

    // Create and add SSAO passes (before bloom and tone mapping)
    // Chain order: Scene -> SSAO -> SSAOBlur -> SSAOComposite -> Bloom -> ToneMap -> Gamma
    _ssao = std::make_unique<OsgVerseSSAO>();
    if (_ssao->initialize(width, height)) {
        _ssao->setDepthTexture(_postProcessChain->getSceneDepthTexture());
        _ssao->setSceneColorTexture(_postProcessChain->getSceneColorTexture());
        _postProcessChain->addPass(_ssao->getSSAOPass());
        _postProcessChain->addPass(_ssao->getBlurPass());
        _postProcessChain->addPass(_ssao->getCompositePass());
        Base::Console().log("OsgVerseEngine: SSAO passes added to chain\n");
    }

    // Create and add bloom passes (after SSAO, before tone mapping)
    // Chain order: Scene -> SSAO -> SSAOBlur -> SSAOComposite -> BloomExtract -> BlurH -> BlurV -> BloomComposite -> ToneMap -> Gamma
    _bloom = std::make_unique<OsgVerseBloom>();
    if (_bloom->initialize(width, height)) {
        _bloom->setSceneTexture(_postProcessChain->getSceneColorTexture());
        auto bloomPasses = _bloom->getPasses();
        for (auto& pass : bloomPasses) {
            _postProcessChain->addPass(pass);
        }
        Base::Console().log("OsgVerseEngine: Bloom passes added to chain\n");
    }

    // Create TAA resolve pass (after bloom, before tone mapping)
    // Chain: Scene -> [SSAO] -> [Bloom] -> TAA -> ToneMap -> Gamma
    _taa = std::make_unique<OsgVerseTAA>();
    if (_taa->initialize(width, height)) {
        _taa->setCurrentFrameTexture(_postProcessChain->getSceneColorTexture());
        _taa->setDepthTexture(_postProcessChain->getSceneDepthTexture());
        auto resolvePass = _taa->getResolvePass();
        if (resolvePass) {
            _postProcessChain->addPass(resolvePass);
        }
        Base::Console().log("OsgVerseEngine: TAA resolve pass added to chain\n");
    }

    // Create tone mapping pass
    auto toneMapPass = std::make_shared<PostProcessPass>("ToneMap");
    auto* toneMapProgram = OsgVerseShaderManager::instance().getProgram(ShaderType::ToneMap);
    if (toneMapProgram) {
        toneMapPass->setProgram(toneMapProgram);
        toneMapPass->setUniform("u_toneMapMode", 0);  // Reinhard by default
        toneMapPass->setUniform("u_exposure", 1.0f);
    }
    _postProcessChain->addPass(toneMapPass);

    // Create gamma correction pass (final pass)
    auto gammaPass = std::make_shared<PostProcessPass>("GammaCorrection");
    auto* gammaProgram = OsgVerseShaderManager::instance().getProgram(ShaderType::GammaCorrection);
    if (gammaProgram) {
        gammaPass->setProgram(gammaProgram);
        gammaPass->setUniform("u_gamma", 2.2f);
    }
    _postProcessChain->addPass(gammaPass);

    // Build the chain
    _postProcessChain->rebuildChain();

    // Add pass cameras to scene
    if (_postProcessChain->getPassRoot()) {
        _sceneRoot->addChild(_postProcessChain->getPassRoot());
    }

    // Initially disabled — enable via setHDREnabled(true)
    _postProcessChain->setEnabled(false);

    Base::Console().log("OsgVerseEngine: Post-processing chain initialized\n");
}

void OsgVerseEngine::updateStats()
{
    if (!_viewer) {
        return;
    }

    ++_stats.frameCount;
    ++_framesSinceLastTraversal;

    // Frame timing from osg::Stats
    osg::Stats* viewerStats = _viewer->getViewerStats();
    if (viewerStats) {
        double frameBegin = 0.0, frameEnd = 0.0;
        int fn = static_cast<int>(_viewer->getFrameStamp()->getFrameNumber());
        if (viewerStats->getAttribute(fn, "Frame begin time", frameBegin) &&
            viewerStats->getAttribute(fn, "Frame end time", frameEnd)) {
            _stats.frameTime = (frameEnd - frameBegin) * 1000.0;  // seconds -> ms
            if (_stats.frameTime > 0.0) {
                _stats.fps = 1000.0 / _stats.frameTime;
            }
        }
    }

    // Expensive scene traversal only every N frames
    if (_framesSinceLastTraversal >= _statsUpdateInterval && _sceneRoot) {
        _framesSinceLastTraversal = 0;

        StatsVisitor sv;
        _sceneRoot->accept(sv);

        _stats.nodeCount = sv.nodeCount;
        _stats.geodeCount = sv.geodeCount;
        _stats.geometryCount = sv.geometryCount;
        _stats.vertexCount = sv.vertexCount;
        _stats.triangleCount = sv.triangleCount;
        _stats.drawCalls = sv.drawCalls;

        // Also update memory stats during traversal
        MemoryVisitor mv;
        _sceneRoot->accept(mv);

        _stats.vboMemory = mv.vboMemory;
        _stats.textureMemory = mv.textureMemory;
        _stats.gpuMemoryUsed = mv.vboMemory + mv.textureMemory;
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
