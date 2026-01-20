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

#ifndef GUI_VIEW3D_VIEWERFACTORY_H
#define GUI_VIEW3D_VIEWERFACTORY_H

#include <memory>
#include <functional>
#include <unordered_map>

#include <FCGlobal.h>
#include <Gui/Render/Core/RenderTypes.h>

class QWidget;
class QOpenGLWidget;

namespace Gui {
namespace View3D {

class IViewer3D;

/**
 * @brief 3D 视图渲染器工厂
 * 
 * 负责创建不同后端的 IViewer3D 实现
 * 
 * 使用方式：
 * ```cpp
 * // 创建默认后端的渲染器
 * auto viewer = ViewerFactory::createDefault(parent);
 * 
 * // 创建指定后端的渲染器
 * auto coinViewer = ViewerFactory::create(BackendType::Coin3D, parent);
 * auto osgViewer = ViewerFactory::create(BackendType::OsgVerse, parent);
 * ```
 */
class GuiExport ViewerFactory {
public:
    /**
     * @brief 渲染器创建函数类型
     */
    using CreatorFunc = std::function<std::unique_ptr<IViewer3D>(QWidget*, const QOpenGLWidget*)>;

    /**
     * @brief 创建指定后端的渲染器
     * 
     * @param type 后端类型
     * @param parent 父 widget
     * @param shareWidget 共享 OpenGL 上下文的 widget
     * @return std::unique_ptr<IViewer3D> 渲染器实例
     * 
     * @throws std::runtime_error 如果后端不可用
     */
    static std::unique_ptr<IViewer3D> create(
        Render::BackendType type,
        QWidget* parent = nullptr,
        const QOpenGLWidget* shareWidget = nullptr
    );

    /**
     * @brief 创建默认后端的渲染器
     * 
     * 默认后端由 RenderManager 决定
     */
    static std::unique_ptr<IViewer3D> createDefault(
        QWidget* parent = nullptr,
        const QOpenGLWidget* shareWidget = nullptr
    );

    /**
     * @brief 注册渲染器创建函数
     * 
     * 允许动态注册新的后端实现
     * 
     * @param type 后端类型
     * @param creator 创建函数
     */
    static void registerCreator(Render::BackendType type, CreatorFunc creator);

    /**
     * @brief 取消注册渲染器创建函数
     */
    static void unregisterCreator(Render::BackendType type);

    /**
     * @brief 检查后端是否已注册
     */
    static bool isRegistered(Render::BackendType type);

    /**
     * @brief 获取所有已注册的后端类型
     */
    static std::vector<Render::BackendType> getRegisteredBackends();

private:
    // 禁止实例化
    ViewerFactory() = delete;
    ~ViewerFactory() = delete;

    // 获取创建器映射表
    static std::unordered_map<Render::BackendType, CreatorFunc>& getCreators();
};

} // namespace View3D
} // namespace Gui

#endif // GUI_VIEW3D_VIEWERFACTORY_H
