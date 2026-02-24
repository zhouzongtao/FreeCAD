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

// Only provide this class when OsgVerse backend is available
#ifdef RENDER_HAS_OSGVERSE_BACKEND

#include <memory>
#include <QImage>
#include <QWidget>

#include "View3DBase.h"
#include "View3D/IViewer3D.h"
#include <Base/Parameter.h>

class QOpenGLWidget;
class QPrinter;
class QStackedWidget;

namespace Gui
{

class Document;

/**
 * @brief 3D view using OsgVerse rendering backend
 * 
 * This view class uses the OsgVerse rendering backend for 3D visualization.
 * It provides similar functionality to View3DInventor but uses OpenSceneGraph
 * instead of Coin3D.
 */
class GuiExport View3DOsgVerse : public View3DBase, public ParameterGrp::ObserverType
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * @brief Constructor
     * @param pcDocument The document this view belongs to
     * @param parent Parent widget
     * @param wflags Window flags
     */
    View3DOsgVerse(Gui::Document* pcDocument, 
                   QWidget* parent, 
                   const QOpenGLWidget* shareWidget = nullptr,
                   Qt::WindowFlags wflags = Qt::WindowFlags());
    
    /**
     * @brief Destructor
     */
    ~View3DOsgVerse() override;

    //-----------------------------------------------------------------------
    // View3DBase interface
    //-----------------------------------------------------------------------
    
    /**
     * @brief Get the viewer interface
     * @return Pointer to IViewer3D interface
     */
    View3D::IViewer3D* getViewerInterface() override;
    
    /**
     * @brief Get the backend type
     * @return BackendType::OsgVerse
     */
    BackendType getBackendType() const override;

    //-----------------------------------------------------------------------
    // MDIView interface
    //-----------------------------------------------------------------------
    
    /**
     * @brief Update the view
     */
    void onUpdate() override;
    
    /**
     * @brief Get the view name
     * @return "View3DOsgVerse"
     */
    const char* getName() const override;
    
    /**
     * @brief Handle messages
     */
    bool onMsg(const char* pMsg, const char** ppReturn) override;
    
    /**
     * @brief Check if message is supported
     */
    bool onHasMsg(const char* pMsg) const override;
    
    /**
     * @brief Print the view
     */
    void print() override;
    
    /**
     * @brief Print to PDF
     */
    void printPdf() override;
    
    /**
     * @brief Show print preview
     */
    void printPreview() override;

    /**
     * @brief Print to a specific printer
     */
    void print(QPrinter* printer) override;

    /**
     * @brief Set view mode (Child/TopLevel/FullScreen)
     */
    void setCurrentViewMode(ViewMode mode) override;

    //-----------------------------------------------------------------------
    // View operations
    //-----------------------------------------------------------------------
    
    /**
     * @brief Fit all objects in view
     */
    void viewAll() override;

    void setOverlayWidget(QWidget*);
    void removeOverlayWidget();

    /**
     * @brief Toggle clipping plane through the focal plane
     */
    void toggleClippingPlane() override;

    bool setCamera(const char* pCamera);

    /**
     * @brief Check if clipping plane is active
     */
    bool hasClippingPlane() const override;

    /**
     * @brief Dump view information
     */
    void dump();
    
    /**
     * @brief Get Python object for this view
     * @return Python object
     */
    PyObject* getPyObject() override;

    bool containsViewProvider(const ViewProvider*) const override;
    void onRename(Gui::Document* pDoc) override;

public Q_SLOTS:
    void setOverrideCursor(const QCursor&) override;
    void restoreOverrideCursor() override;

protected Q_SLOTS:
    void stopAnimating();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void customEvent(QEvent* e) override;

    /// ParameterGrp::ObserverType
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

private:
    void applySettings();

    std::unique_ptr<View3D::IViewer3D> _viewer;  ///< The OsgVerse viewer
    bool _clippingPlaneActive{false};             ///< Whether clipping plane is active
    ParameterGrp::handle _hViewGrp;               ///< View preferences group
    ParameterGrp::handle _hNaviCubeGrp;           ///< NaviCube preferences group
    QStackedWidget* _stack{nullptr};              ///< Stack for overlay widget support
    PyObject* _viewerPy{nullptr};                 ///< Python binding object
};

} // namespace Gui

#endif // RENDER_HAS_OSGVERSE_BACKEND

#endif // GUI_VIEW3DOSGVERSE_H
