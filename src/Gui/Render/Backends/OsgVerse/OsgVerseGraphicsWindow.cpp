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
# include <QCursor>
# include <QOpenGLContext>
#endif

#include "OsgVerseGraphicsWindow.h"
#include <Base/Console.h>

using namespace Gui::Render;

OsgVerseGraphicsWindow::OsgVerseGraphicsWindow(osg::GraphicsContext::Traits* traits,
                                               QOpenGLContext* context,
                                               QSurface* surface)
    : _qtContext(context)
    , _qtSurface(surface)
{
    _traits = traits;
    
    if (_traits) {
        _windowX = _traits->x;
        _windowY = _traits->y;
        _windowWidth = _traits->width;
        _windowHeight = _traits->height;
    }
    
    setState(new osg::State);
    getState()->setGraphicsContext(this);
    
    if (_traits.valid()) {
        _valid = true;
    }
    
    Base::Console().log("OsgVerseGraphicsWindow: Created (%dx%d)\n", _windowWidth, _windowHeight);
}

OsgVerseGraphicsWindow::~OsgVerseGraphicsWindow()
{
    Base::Console().log("OsgVerseGraphicsWindow: Destroyed\n");
    close();
}

bool OsgVerseGraphicsWindow::realizeImplementation()
{
    if (_realized) {
        return true;
    }
    
    Base::Console().log("OsgVerseGraphicsWindow: Realizing...\n");
    
    if (!_qtContext) {
        Base::Console().error("OsgVerseGraphicsWindow: No Qt OpenGL context available\n");
        return false;
    }
    
    if (!_qtSurface) {
        Base::Console().error("OsgVerseGraphicsWindow: No Qt surface available\n");
        return false;
    }
    
    // 确保 Qt 上下文是当前的
    // Make sure Qt context is current
    if (!_qtContext->makeCurrent(_qtSurface)) {
        Base::Console().error("OsgVerseGraphicsWindow: Failed to make Qt context current\n");
        return false;
    }
    
    _realized = true;
    Base::Console().log("OsgVerseGraphicsWindow: Realized successfully\n");
    
    return true;
}

void OsgVerseGraphicsWindow::closeImplementation()
{
    Base::Console().log("OsgVerseGraphicsWindow: Closing...\n");
    _realized = false;
}

bool OsgVerseGraphicsWindow::makeCurrentImplementation()
{
    if (!_qtContext || !_qtSurface) {
        return false;
    }
    
    return _qtContext->makeCurrent(_qtSurface);
}

bool OsgVerseGraphicsWindow::releaseContextImplementation()
{
    if (!_qtContext) {
        return false;
    }
    
    _qtContext->doneCurrent();
    return true;
}

void OsgVerseGraphicsWindow::swapBuffersImplementation()
{
    if (_qtContext && _qtSurface) {
        _qtContext->swapBuffers(_qtSurface);
    }
}

bool OsgVerseGraphicsWindow::setWindowRectangleImplementation(int x, int y, int width, int height)
{
    _windowX = x;
    _windowY = y;
    _windowWidth = width;
    _windowHeight = height;
    
    if (_traits) {
        _traits->x = x;
        _traits->y = y;
        _traits->width = width;
        _traits->height = height;
    }
    
    return true;
}

void OsgVerseGraphicsWindow::getWindowRectangle(int& x, int& y, int& width, int& height)
{
    x = _windowX;
    y = _windowY;
    width = _windowWidth;
    height = _windowHeight;
}

bool OsgVerseGraphicsWindow::setWindowDecorationImplementation(bool flag)
{
    if (_traits) {
        _traits->windowDecoration = flag;
    }
    return true;
}

void OsgVerseGraphicsWindow::setCursor(MouseCursor cursor)
{
    // Qt 光标映射
    // Qt cursor mapping
    Qt::CursorShape qtCursor = Qt::ArrowCursor;
    
    switch (cursor) {
        case NoCursor:
            qtCursor = Qt::BlankCursor;
            break;
        case RightArrowCursor:
        case LeftArrowCursor:
            qtCursor = Qt::ArrowCursor;
            break;
        case InfoCursor:
            qtCursor = Qt::WhatsThisCursor;
            break;
        case DestroyCursor:
            qtCursor = Qt::ForbiddenCursor;
            break;
        case HelpCursor:
            qtCursor = Qt::WhatsThisCursor;
            break;
        case CycleCursor:
            qtCursor = Qt::WaitCursor;
            break;
        case SprayCursor:
            qtCursor = Qt::CrossCursor;
            break;
        case WaitCursor:
            qtCursor = Qt::WaitCursor;
            break;
        case TextCursor:
            qtCursor = Qt::IBeamCursor;
            break;
        case CrosshairCursor:
            qtCursor = Qt::CrossCursor;
            break;
        case HandCursor:
            qtCursor = Qt::OpenHandCursor;
            break;
        case UpDownCursor:
            qtCursor = Qt::SizeVerCursor;
            break;
        case LeftRightCursor:
            qtCursor = Qt::SizeHorCursor;
            break;
        case TopSideCursor:
        case BottomSideCursor:
            qtCursor = Qt::SizeVerCursor;
            break;
        case LeftSideCursor:
        case RightSideCursor:
            qtCursor = Qt::SizeHorCursor;
            break;
        case TopLeftCorner:
        case BottomRightCorner:
            qtCursor = Qt::SizeFDiagCursor;
            break;
        case TopRightCorner:
        case BottomLeftCorner:
            qtCursor = Qt::SizeBDiagCursor;
            break;
        default:
            qtCursor = Qt::ArrowCursor;
            break;
    }
    
    // 注意：实际设置光标需要通过 Qt widget
    // Note: Actually setting cursor needs to be done through Qt widget
    // 这里只是记录，实际应用由 ViewerWidget 处理
    // This is just recorded, actual application handled by ViewerWidget
}

void OsgVerseGraphicsWindow::requestWarpPointer(float x, float y)
{
    // 鼠标指针移动请求
    // Mouse pointer warp request
    // 需要通过 Qt widget 实现
    // Needs to be implemented through Qt widget
}

void OsgVerseGraphicsWindow::setQtContext(QOpenGLContext* context, QSurface* surface)
{
    _qtContext = context;
    _qtSurface = surface;
    
    Base::Console().log("OsgVerseGraphicsWindow: Qt context set\n");
}
