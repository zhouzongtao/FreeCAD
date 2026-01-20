/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                    *
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

#include <QApplication>
#include <QKeyEvent>
#include <QEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QLayout>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QTimer>
#include <QUrl>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "View3DOsgVerse.h"
#include "View3D/Backends/OsgVerse/OsgVerseViewerImpl.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "MainWindow.h"

using namespace Gui;

/* TRANSLATOR Gui::View3DOsgVerse */

TYPESYSTEM_SOURCE_ABSTRACT(Gui::View3DOsgVerse, Gui::View3DBase)

View3DOsgVerse::View3DOsgVerse(
    Gui::Document* pcDocument,
    QWidget* parent,
    const QOpenGLWidget* sharewidget,
    Qt::WindowFlags wflags
)
    : View3DBase(pcDocument, parent, wflags)
    , _viewerPy(nullptr)
{
    // important for highlighting
    setMouseTracking(true);
    // accept drops on the window
    setAcceptDrops(true);

    Base::Console().log("View3DOsgVerse: Creating OsgVerse viewer\n");

    // Create OsgVerse viewer
    _viewer = new View3D::OsgVerse::OsgVerseViewerImpl(this, sharewidget);

    // Set central widget
    setCentralWidget(_viewer->getWidget());

    setWindowIcon(Gui::BitmapFactory().pixmap("Document"));

    Base::Console().log("View3DOsgVerse: OsgVerse viewer created successfully\n");
}

View3DOsgVerse::~View3DOsgVerse()
{
    if (_viewerPy) {
        Base::PyGILStateLocker lock;
        Py_DECREF(_viewerPy);
    }

    delete _viewer;
}

void View3DOsgVerse::deleteSelf()
{
    // Clean up viewer
    View3DBase::deleteSelf();
}

View3DOsgVerse* View3DOsgVerse::clone()
{
    // TODO: Implement cloning for OsgVerse view
    Base::Console().warning("View3DOsgVerse::clone() not yet implemented\n");
    return nullptr;
}

PyObject* View3DOsgVerse::getPyObject()
{
    // TODO: Implement Python wrapper for OsgVerse view
    if (!_viewerPy) {
        Base::Console().warning("View3DOsgVerse::getPyObject() not yet implemented\n");
        Py_INCREF(Py_None);
        return Py_None;
    }

    Py_INCREF(_viewerPy);
    return _viewerPy;
}

void View3DOsgVerse::onRename(Gui::Document* /*pDoc*/)
{
    // TODO: Implement document rename handling
}

void View3DOsgVerse::onUpdate()
{
    update();
    if (_viewer) {
        // OsgVerseViewerImpl doesn't have update() method, use redraw instead
        _viewer->getWidget()->update();
    }
}

void View3DOsgVerse::viewAll()
{
    if (_viewer) {
        _viewer->viewAll();
    }
}

const char* View3DOsgVerse::getName() const
{
    return "View3DOsgVerse";
}

void View3DOsgVerse::print()
{
    // TODO: Implement printing
    Base::Console().warning("View3DOsgVerse::print() not yet implemented\n");
}

void View3DOsgVerse::printPdf()
{
    // TODO: Implement PDF printing
    Base::Console().warning("View3DOsgVerse::printPdf() not yet implemented\n");
}

void View3DOsgVerse::printPreview()
{
    // TODO: Implement print preview
    Base::Console().warning("View3DOsgVerse::printPreview() not yet implemented\n");
}

void View3DOsgVerse::print(QPrinter* /*printer*/)
{
    // TODO: Implement printing to printer
    Base::Console().warning("View3DOsgVerse::print(QPrinter*) not yet implemented\n");
}

bool View3DOsgVerse::containsViewProvider(const ViewProvider* /*vp*/) const
{
    // TODO: Implement view provider containment check
    return false;
}

bool View3DOsgVerse::onMsg(const char* pMsg, const char** /*ppReturn*/)
{
    if (strcmp("ViewFit", pMsg) == 0) {
        viewAll();
        return true;
    }

    // TODO: Implement other message handlers
    return false;
}

bool View3DOsgVerse::onHasMsg(const char* pMsg) const
{
    if (strcmp("ViewFit", pMsg) == 0) {
        return true;
    }

    // TODO: Implement other message checks
    return false;
}

void View3DOsgVerse::setOverrideCursor(const QCursor& aCursor)
{
    if (_viewer && _viewer->getWidget()) {
        _viewer->getWidget()->setCursor(aCursor);
    }
}

void View3DOsgVerse::restoreOverrideCursor()
{
    if (_viewer && _viewer->getWidget()) {
        _viewer->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
    }
}

void View3DOsgVerse::windowStateChanged(QWidget* /*view*/)
{
    // TODO: Implement window state change handling
}

void View3DOsgVerse::dropEvent(QDropEvent* e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        getMainWindow()->loadUrls(getAppDocument(), data->urls());
    }
    else {
        View3DBase::dropEvent(e);
    }
}

void View3DOsgVerse::dragEnterEvent(QDragEnterEvent* e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        e->accept();
    }
    else {
        e->ignore();
    }
}

void View3DOsgVerse::setCurrentViewMode(ViewMode mode)
{
    ViewMode oldmode = currentViewMode();
    if (mode == oldmode) {
        return;
    }

    if (mode == Child) {
        QWindow* winHandle = this->windowHandle();
        if (winHandle) {
            winHandle->destroy();
        }
    }

    View3DBase::setCurrentViewMode(mode);

    // TODO: Implement focus proxy handling for OsgVerse viewer
}

void View3DOsgVerse::keyPressEvent(QKeyEvent* e)
{
    QMainWindow::keyPressEvent(e);
}

void View3DOsgVerse::keyReleaseEvent(QKeyEvent* e)
{
    QMainWindow::keyReleaseEvent(e);
}

void View3DOsgVerse::focusInEvent(QFocusEvent* /*e*/)
{
    if (_viewer && _viewer->getWidget()) {
        _viewer->getWidget()->setFocus();
    }
}

void View3DOsgVerse::contextMenuEvent(QContextMenuEvent* e)
{
    View3DBase::contextMenuEvent(e);
}

void View3DOsgVerse::customEvent(QEvent* /*e*/)
{
    // TODO: Implement custom event handling
}

View3D::IViewer3D* View3DOsgVerse::getViewerInterface()
{
    // OsgVerseViewerImpl implements IViewer3D interface
    return _viewer;
}

#include "moc_View3DOsgVerse.cpp"
