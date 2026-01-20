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

#ifndef GUI_RENDER_VIEWERRENDERER_H
#define GUI_RENDER_VIEWERRENDERER_H

#include <memory>
#include <QEvent>
#include "RenderTypes.h"

class QOpenGLContext;

namespace Gui {
namespace Render {

/**
 * @brief Abstract interface for 3D viewer rendering
 * 
 * This interface abstracts the rendering backend (Coin3D or OsgVerse)
 * from the View3DInventorViewer, allowing runtime backend switching.
 */
class GuiExport ViewerRenderer {
public:
    virtual ~ViewerRenderer() = default;
    
    //-----------------------------------------------------------------------
    // Lifecycle
    //-----------------------------------------------------------------------
    
    /**
     * @brief Initialize the renderer
     * @param context OpenGL context to use
     * @return true if successful
     */
    virtual bool initialize(QOpenGLContext* context) = 0;
    
    /**
     * @brief Shutdown and cleanup the renderer
     */
    virtual void shutdown() = 0;
    
    //-----------------------------------------------------------------------
    // Rendering
    //-----------------------------------------------------------------------
    
    /**
     * @brief Render the scene
     */
    virtual void render() = 0;
    
    /**
     * @brief Handle viewport resize
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void resize(int width, int height) = 0;
    
    //-----------------------------------------------------------------------
    // Scene Graph
    //-----------------------------------------------------------------------
    
    /**
     * @brief Set the scene graph root node
     * @param sceneGraph Backend-specific scene graph (SoNode* or osg::Node*)
     */
    virtual void setSceneGraph(void* sceneGraph) = 0;
    
    /**
     * @brief Get the scene graph root node
     * @return Backend-specific scene graph pointer
     */
    virtual void* getSceneGraph() const = 0;
    
    /**
     * @brief Schedule a redraw
     */
    virtual void scheduleRedraw() = 0;
    
    //-----------------------------------------------------------------------
    // Camera
    //-----------------------------------------------------------------------
    
    /**
     * @brief Set camera position
     * @param position Camera position (x, y, z)
     */
    virtual void setCameraPosition(const Vector3& position) = 0;
    
    /**
     * @brief Get camera position
     * @return Camera position (x, y, z)
     */
    virtual Vector3 getCameraPosition() const = 0;
    
    /**
     * @brief Set camera orientation
     * @param orientation Camera orientation as quaternion (x, y, z, w)
     */
    virtual void setCameraOrientation(const Quaternion& orientation) = 0;
    
    /**
     * @brief Get camera orientation
     * @return Camera orientation as quaternion
     */
    virtual Quaternion getCameraOrientation() const = 0;
    
    /**
     * @brief View all - fit scene in viewport
     */
    virtual void viewAll() = 0;
    
    //-----------------------------------------------------------------------
    // Event Handling
    //-----------------------------------------------------------------------
    
    /**
     * @brief Process Qt event
     * @param event Qt event to process
     * @return true if event was handled
     */
    virtual bool processEvent(QEvent* event) = 0;
    
    //-----------------------------------------------------------------------
    // Properties
    //-----------------------------------------------------------------------
    
    /**
     * @brief Get the backend type
     * @return Backend type (Coin3D or OsgVerse)
     */
    virtual BackendType getBackendType() const = 0;
    
    /**
     * @brief Check if renderer is initialized
     * @return true if initialized
     */
    virtual bool isInitialized() const = 0;
    
    /**
     * @brief Set background color
     * @param color Background color (RGBA)
     */
    virtual void setBackgroundColor(const Color& color) = 0;
    
    /**
     * @brief Get background color
     * @return Background color (RGBA)
     */
    virtual Color getBackgroundColor() const = 0;
};

/**
 * @brief Factory for creating ViewerRenderer instances
 */
class GuiExport ViewerRendererFactory {
public:
    /**
     * @brief Create a renderer for the specified backend
     * @param type Backend type
     * @return Renderer instance or nullptr if backend not available
     */
    static std::unique_ptr<ViewerRenderer> create(BackendType type);
    
    /**
     * @brief Check if a backend is available
     * @param type Backend type to check
     * @return true if backend is available
     */
    static bool isAvailable(BackendType type);
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_VIEWERRENDERER_H
