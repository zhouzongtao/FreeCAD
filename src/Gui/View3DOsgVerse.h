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

#ifndef GUI_VIEW3DOSGVERSE_H
#define GUI_VIEW3DOSGVERSE_H

#include <memory>
#include <QImage>

#include "View3DBase.h"
#include "View3D/IViewer3D.h"

class QOpenGLWidget;

namespace Gui
{

class Document;
namespace View3D
{
namespace OsgVerse
{
class OsgVerseViewerImpl;
}
}  // namespace View3D

/** The 3D view window (OsgVerse backend)
 *  It consists out of the 3D view using OsgVerse/OSG rendering
 */
class GuiExport View3DOsgVerse: public View3DBase
{
    Q_OBJECT

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    View3DOsgVerse(
        Gui::Document* pcDocument,
        QWidget* parent,
        const QOpenGLWidget* sharewidget = nullptr,
        Qt::WindowFlags wflags = Qt::WindowFlags()
    );
    ~View3DOsgVerse() override;

    View3DOsgVerse* clone() override;

    /// Message handler
    bool onMsg(const char* pMsg, const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;
    void deleteSelf() override;
    /// get called when the document is updated
    void onRename(Gui::Document* pDoc) override;
    void onUpdate() override;
    void viewAll() override;
    const char* getName() const override;

    /// print function of the view
    void print() override;
    void printPdf() override;
    void printPreview() override;
    void print(QPrinter*) override;

    PyObject* getPyObject() override;
    void setCurrentViewMode(ViewMode b) override;

    bool containsViewProvider(const ViewProvider*) const override;

    // View3DBase abstract interface implementation
    View3D::IViewer3D* getViewerInterface() override;
    View3DBase::BackendType getBackendType() const override
    {
        return View3DBase::BackendType::OsgVerse;
    }

    View3D::OsgVerse::OsgVerseViewerImpl* getViewer() const
    {
        return _viewer;
    }

public Q_SLOTS:
    /// override the cursor in this view
    void setOverrideCursor(const QCursor&) override;
    void restoreOverrideCursor() override;

protected:
    void windowStateChanged(QWidget* view) override;
    void dropEvent(QDropEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void customEvent(QEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private:
    View3D::OsgVerse::OsgVerseViewerImpl* _viewer;
    PyObject* _viewerPy;
};

}  // namespace Gui

#endif  // GUI_VIEW3DOSGVERSE_H
