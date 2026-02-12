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
#include <QWidget>

#include "View3DBase.h"
#include "View3D/IViewer3D.h"

class QOpenGLWidget;

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
class GuiExport View3DOsgVerse : public View3DBase
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

    //-----------------------------------------------------------------------
    // View operations
    //-----------------------------------------------------------------------
    
    /**
     * @brief Fit all objects in view
     */
    void viewAll() override;
    
    /**
     * @brief Dump view information
     */
    void dump();
    
    /**
     * @brief Get Python object for this view
     * @return Python object
     */
    PyObject* getPyObject() override;

protected:
    /**
     * @brief Handle resize events
     */
    void resizeEvent(QResizeEvent* event) override;

private:
    std::unique_ptr<View3D::IViewer3D> _viewer;  ///< The OsgVerse viewer
};

} // namespace Gui

#endif // RENDER_HAS_OSGVERSE_BACKEND

#endif // GUI_VIEW3DOSGVERSE_H
