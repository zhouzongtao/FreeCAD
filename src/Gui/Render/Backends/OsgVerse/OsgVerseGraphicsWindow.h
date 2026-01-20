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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEGRAPHICSWINDOW_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEGRAPHICSWINDOW_H

#include <osgViewer/GraphicsWindow>
#include <QOpenGLContext>
#include <QSurface>

namespace Gui {
namespace Render {

/**
 * @brief 自定义 OSG GraphicsWindow 实现，不依赖 osgQt
 * Custom OSG GraphicsWindow implementation without osgQt dependency
 *
 * 这个类将 OSG 的渲染上下文与 Qt 的 OpenGL 上下文集成。
 * This class integrates OSG's rendering context with Qt's OpenGL context.
 */
class OsgVerseGraphicsWindow : public osgViewer::GraphicsWindow {
public:
    /**
     * @brief 构造函数
     * @param traits 图形上下文特性
     * @param context Qt OpenGL 上下文
     * @param surface Qt 表面
     */
    OsgVerseGraphicsWindow(osg::GraphicsContext::Traits* traits,
                           QOpenGLContext* context = nullptr,
                           QSurface* surface = nullptr);

    virtual ~OsgVerseGraphicsWindow();

    //-----------------------------------------------------------------------
    // GraphicsWindow 接口实现
    //-----------------------------------------------------------------------

    bool isSameKindAs(const Object* object) const override {
        return dynamic_cast<const OsgVerseGraphicsWindow*>(object) != nullptr;
    }

    const char* libraryName() const override { return "FreeCAD"; }
    const char* className() const override { return "OsgVerseGraphicsWindow"; }

    bool valid() const override { return _valid; }

    bool realizeImplementation() override;
    bool isRealizedImplementation() const override { return _realized; }

    void closeImplementation() override;
    bool makeCurrentImplementation() override;
    bool releaseContextImplementation() override;

    void swapBuffersImplementation() override;

    void grabFocus() override {}
    void grabFocusIfPointerInWindow() override {}
    void raiseWindow() override {}

    void getWindowRectangle(int& x, int& y, int& width, int& height) override;

    bool setWindowRectangleImplementation(int x, int y, int width, int height) override;
    bool setWindowDecorationImplementation(bool flag) override;

    void setCursor(MouseCursor cursor) override;

    bool checkEvents() override { return false; }

    void requestWarpPointer(float x, float y) override;

    //-----------------------------------------------------------------------
    // Qt 集成接口
    //-----------------------------------------------------------------------

    /**
     * @brief 设置 Qt OpenGL 上下文
     */
    void setQtContext(QOpenGLContext* context, QSurface* surface);

    /**
     * @brief 获取 Qt OpenGL 上下文
     */
    QOpenGLContext* getQtContext() const { return _qtContext; }

    /**
     * @brief 获取 Qt 表面
     */
    QSurface* getQtSurface() const { return _qtSurface; }

private:
    bool _valid{false};
    bool _realized{false};
    
    QOpenGLContext* _qtContext{nullptr};
    QSurface* _qtSurface{nullptr};
    
    int _windowX{0};
    int _windowY{0};
    int _windowWidth{800};
    int _windowHeight{600};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEGRAPHICSWINDOW_H
