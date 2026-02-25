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

#include "ViewerFactory.h"
#include "IViewer3D.h"
#include <Gui/Render/Core/RenderTypes.h>
#include <Gui/Core/RenderManager.h>
#include <Base/Console.h>

using namespace Gui::View3D;
namespace Render = Gui::Render;

std::unordered_map<Render::BackendType, ViewerFactory::CreatorFunc>& 
ViewerFactory::getCreators()
{
    static std::unordered_map<Render::BackendType, CreatorFunc> creators;
    return creators;
}

std::unique_ptr<IViewer3D> ViewerFactory::create(
    Render::BackendType type,
    QWidget* parent,
    const QOpenGLWidget* shareWidget)
{
    auto& creators = getCreators();
    auto it = creators.find(type);
    
    if (it == creators.end()) {
        std::string msg = "Viewer backend not registered: " + std::to_string(static_cast<int>(type));
        Base::Console().error("%s\n", msg.c_str());
        throw std::runtime_error(msg);
    }
    
    Base::Console().log("ViewerFactory: Creating viewer for backend type %d\n", 
                        static_cast<int>(type));
    
    return it->second(parent, shareWidget);
}

std::unique_ptr<IViewer3D> ViewerFactory::createDefault(
    QWidget* parent,
    const QOpenGLWidget* shareWidget)
{
    // 从 RenderManager 获取当前后端
    auto& renderMgr = Gui::Core::RenderManager::instance();
    auto backendType = renderMgr.getCurrentBackend();
    
    Base::Console().log("ViewerFactory: Creating default viewer (backend: %d)\n",
                        static_cast<int>(backendType));
    
    // 如果当前后端是 None，使用 Coin3D 作为默认
    if (backendType == Render::BackendType::None) {
        backendType = Render::BackendType::Coin3D;
        Base::Console().warning("ViewerFactory: No backend selected, using Coin3D as default\n");
    }
    
    // 检查请求的后端是否已注册
    if (!isRegistered(backendType)) {
        Base::Console().warning(
            "ViewerFactory: Requested backend %d is not registered, falling back to Coin3D\n",
            static_cast<int>(backendType)
        );
        backendType = Render::BackendType::Coin3D;
        
        // 如果 Coin3D 也没注册，抛出异常
        if (!isRegistered(backendType)) {
            std::string msg = "ViewerFactory: No viewer backends registered!";
            Base::Console().error("%s\n", msg.c_str());
            throw std::runtime_error(msg);
        }
    }
    
    return create(backendType, parent, shareWidget);
}

void ViewerFactory::registerCreator(Render::BackendType type, CreatorFunc creator)
{
    auto& creators = getCreators();
    creators[type] = std::move(creator);
    
    Base::Console().log("ViewerFactory: Registered creator for backend type %d\n",
                        static_cast<int>(type));
}

void ViewerFactory::unregisterCreator(Render::BackendType type)
{
    auto& creators = getCreators();
    creators.erase(type);
    
    Base::Console().log("ViewerFactory: Unregistered creator for backend type %d\n",
                        static_cast<int>(type));
}

bool ViewerFactory::isRegistered(Render::BackendType type)
{
    auto& creators = getCreators();
    return creators.find(type) != creators.end();
}

std::vector<Render::BackendType> ViewerFactory::getRegisteredBackends()
{
    auto& creators = getCreators();
    std::vector<Render::BackendType> backends;
    backends.reserve(creators.size());
    
    for (auto it = creators.begin(); it != creators.end(); ++it) {
        backends.push_back(it->first);
    }
    
    return backends;
}
