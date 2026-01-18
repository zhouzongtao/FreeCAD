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

#include "Coin3DEngine.h"
#include "Coin3DNode.h"
#include "Coin3DMaterial.h"
#include "Coin3DGeometry.h"
#include "Coin3DUtils.h"

#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/sensors/SoTimerSensor.h>

#include <Base/Console.h>

namespace Gui {
namespace Render {

//===========================================================================
// Coin3DEngine - Coin3D 渲染引擎 / Coin3D Rendering Engine
//===========================================================================

Coin3DEngine::Coin3DEngine()
    : _nativePointer(nullptr)
    , _sceneManager(nullptr)
    , _renderAction(nullptr)
    , _renderMode(RenderMode::Default)
    , _backgroundColor(0.2f, 0.2f, 0.3f, 1.0f)
    , _sceneRoot(nullptr)
    , _stats{}
    , _initialized(false)
{
}

Coin3DEngine::~Coin3DEngine()
{
    shutdown();
}

//---------------------------------------------------------------------------
// 引擎信息 / Engine Information
//---------------------------------------------------------------------------

std::string Coin3DEngine::getName() const
{
    return "Coin3D";
}

std::string Coin3DEngine::getVersion() const
{
    return std::string("Coin3D ") + SoDB::getVersion();
}

BackendInfo Coin3DEngine::getInfo() const
{
    BackendInfo info;
    info.type = BackendType::Coin3D;
    info.name = "Coin3D";
    info.version = getVersion();
    info.description = "Open Inventor compatible rendering engine";
    info.supportsPBR = false;
    info.supportsDeferredRendering = false;
    info.supportsRaytracing = false;
    return info;
}

bool Coin3DEngine::isAvailable() const
{
    // Coin3D 应该总是可用的（在启用 Coin3D 的构建中）
    // Coin3D should always be available (in Coin3D-enabled builds)
    return true;
}

bool Coin3DEngine::initialize()
{
    if (_initialized) {
        return true;
    }

    // 初始化 Coin3D 数据库（如果尚未初始化）
    // Initialize Coin3D database (if not already initialized)
    if (!SoDB::isInitialized()) {
        SoDB::init();
    }

    _initialized = true;
    Base::Console().log("Coin3DEngine::initialize: Engine initialized\n");
    return true;
}

void Coin3DEngine::shutdown()
{
    if (!_initialized) {
        return;
    }

    // 清理场景根节点 / Clean up scene root
    _sceneRoot.reset();

    // 注意：不释放 _sceneManager 和 _renderAction，
    // 因为它们通常由外部（如 View3DInventorViewer）管理
    // Note: Don't free _sceneManager and _renderAction as they
    // are typically managed externally (e.g., by View3DInventorViewer)

    _initialized = false;
    Base::Console().log("Coin3DEngine::shutdown: Engine shutdown\n");
}

//---------------------------------------------------------------------------
// 节点创建 / Node Creation
//---------------------------------------------------------------------------

RenderGroup::Ptr Coin3DEngine::createGroup()
{
    return std::make_shared<Coin3DGroup>();
}

RenderSeparator::Ptr Coin3DEngine::createSeparator()
{
    return std::make_shared<Coin3DSeparator>();
}

RenderNode::Ptr Coin3DEngine::createTransform()
{
    return std::make_shared<Coin3DTransform>();
}

RenderNode::Ptr Coin3DEngine::createSwitch()
{
    return std::make_shared<Coin3DSwitch>();
}

RenderNode::Ptr Coin3DEngine::createMaterial()
{
    return std::make_shared<Coin3DMaterial>();
}

RenderNode::Ptr Coin3DEngine::createGeometry()
{
    // 默认创建一个空的面集
    // By default, create an empty face set
    return std::make_shared<Coin3DIndexedFaceSet>();
}

RenderNode::Ptr Coin3DEngine::createCamera()
{
    // Coin3D 的 SoCamera 创建
    // SoCamera creation for Coin3D
    // TODO: 实现 Coin3DCamera 包装器
    // TODO: Implement Coin3DCamera wrapper
    return nullptr;
}

RenderNode::Ptr Coin3DEngine::createLight(LightType type)
{
    // TODO: 实现 Coin3DLight 包装器
    // TODO: Implement Coin3DLight wrapper
    (void)type;
    return nullptr;
}

//---------------------------------------------------------------------------
// 场景管理 / Scene Management
//---------------------------------------------------------------------------

void Coin3DEngine::setSceneRoot(RenderNode::Ptr root)
{
    _sceneRoot = root;

    // 如果有场景管理器，同步设置
    // If there's a scene manager, sync the setting
    if (_sceneManager && _sceneRoot) {
        if (auto* coin3DNode = dynamic_cast<Coin3DNode*>(_sceneRoot.get())) {
            // 设置场景图到场景管理器
            // Set scene graph to scene manager
            // TODO: 根据场景管理器类型进行适当设置
            // TODO: Properly set based on scene manager type
        }
    }
}

RenderNode::Ptr Coin3DEngine::getSceneRoot() const
{
    return _sceneRoot;
}

void Coin3DEngine::updateScene()
{
    // 触发场景更新
    // Trigger scene update
    if (_sceneRoot) {
        _sceneRoot->touch();
    }
}

//---------------------------------------------------------------------------
// 渲染控制 / Rendering Control
//---------------------------------------------------------------------------

void Coin3DEngine::render()
{
    if (_renderAction) {
        // 执行渲染动作
        // Perform render action
        static_cast<SoGLRenderAction*>(_renderAction)->apply(
            static_cast<SoNode*>(_nativePointer)
        );
        _stats.frameCount++;
    }
}

void Coin3DEngine::setRenderMode(RenderMode mode)
{
    _renderMode = mode;
    // TODO: 更新渲染状态
    // TODO: Update render state
}

RenderMode Coin3DEngine::getRenderMode() const
{
    return _renderMode;
}

void Coin3DEngine::setBackgroundColor(const Color& color)
{
    _backgroundColor = color;
    // TODO: 设置到场景管理器
    // TODO: Set to scene manager
}

Color Coin3DEngine::getBackgroundColor() const
{
    return _backgroundColor;
}

RenderStats Coin3DEngine::getStats() const
{
    return _stats;
}

void Coin3DEngine::resetStats()
{
    _stats = RenderStats{};
}

//---------------------------------------------------------------------------
// 资源管理 / Resource Management
//---------------------------------------------------------------------------

void Coin3DEngine::releaseUnusedResources()
{
    // Coin3D 通常自动管理资源，这里可以触发显式清理
    // Coin3D typically manages resources automatically, trigger explicit cleanup here
    // TODO: 实现资源释放逻辑
    // TODO: Implement resource release logic
}

size_t Coin3DEngine::getMemoryUsage() const
{
    // 估算内存使用 / Estimate memory usage
    // TODO: 实现准确的内存统计
    // TODO: Implement accurate memory statistics
    return 0;
}

//---------------------------------------------------------------------------
// Coin3D 特定接口 / Coin3D-specific Interface
//---------------------------------------------------------------------------

RenderNode::Ptr Coin3DEngine::wrapNode(SoNode* coinNode, bool takeOwnership)
{
    return Coin3DUtils::wrapSoNode(coinNode, takeOwnership);
}

SoNode* Coin3DEngine::unwrapNode(RenderNode* renderNode) const
{
    return Coin3DUtils::unwrapToSoNode(renderNode);
}

void Coin3DEngine::setSceneManager(void* sceneManager)
{
    _sceneManager = sceneManager;
    _nativePointer = sceneManager;  // 设置为相同指针 / Set to same pointer
}

void Coin3DEngine::setRenderAction(void* renderAction)
{
    _renderAction = renderAction;
}

//===========================================================================
// 引擎自动注册 / Engine Auto-Registration
//===========================================================================

namespace {
    // 自动注册 Coin3D 引擎到工厂 / Auto-register Coin3D engine to factory
    RenderEngineRegistration<Coin3DEngine> g_coin3DEngineRegistration(BackendType::Coin3D);
}

} // namespace Render
} // namespace Gui
