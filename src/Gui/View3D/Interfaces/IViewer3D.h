// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * IViewer3D.h - 3D Viewer Interface
 * 
 * This interface defines the contract for 3D rendering backends.
 * Both Coin3D and OsgVerse backends must implement this interface.
 */

#ifndef GUI_VIEW3D_INTERFACES_IVIEWER3D_H
#define GUI_VIEW3D_INTERFACES_IVIEWER3D_H

#include <string>
#include <vector>
#include <QWidget>
#include <QColor>

// Forward declarations
namespace App {
    class DocumentObject;
}

namespace Gui {
    class ViewProvider;
}

namespace Gui {

/**
 * @brief Abstract interface for 3D viewers
 * 
 * This interface provides a backend-agnostic way to interact with 3D viewers.
 * Concrete implementations (Coin3D, OsgVerse) must implement all pure virtual methods.
 */
class GuiExport IViewer3D {
public:
    virtual ~IViewer3D() = default;
    
    /**
     * @brief Get the name of the rendering backend
     * @return Backend name (e.g., "Coin3D", "OsgVerse")
     */
    virtual std::string getBackendName() const = 0;
    
    /**
     * @brief Get the Qt widget for this viewer
     * @return QWidget pointer that can be embedded in the UI
     */
    virtual QWidget* getWidget() = 0;
    
    // ========== Scene Management ==========
    
    /**
     * @brief Add a ViewProvider to the scene
     * @param vp ViewProvider to add
     */
    virtual void addViewProvider(ViewProvider* vp) = 0;
    
    /**
     * @brief Remove a ViewProvider from the scene
     * @param vp ViewProvider to remove
     */
    virtual void removeViewProvider(ViewProvider* vp) = 0;
    
    /**
     * @brief Update a ViewProvider (e.g., after property changes)
     * @param vp ViewProvider to update
     */
    virtual void updateViewProvider(ViewProvider* vp) = 0;
    
    /**
     * @brief Clear all objects from the scene
     */
    virtual void clearScene() = 0;
    
    // ========== Rendering Control ==========
    
    /**
     * @brief Trigger a render update
     */
    virtual void render() = 0;
    
    /**
     * @brief Set the background color
     * @param color Background color
     */
    virtual void setBackgroundColor(const QColor& color) = 0;
    
    /**
     * @brief Enable/disable anti-aliasing
     * @param enable True to enable, false to disable
     */
    virtual void setAntiAliasing(bool enable) = 0;
    
    // ========== Camera Control ==========
    
    /**
     * @brief Fit all objects in the view
     */
    virtual void viewAll() = 0;
    
    /**
     * @brief Set camera position and orientation
     * @param position Camera position (x, y, z)
     * @param orientation Camera orientation (axis-angle or quaternion)
     * @param up Up vector
     */
    virtual void setCamera(const float position[3], 
                          const float orientation[4],
                          const float up[3]) = 0;
    
    /**
     * @brief Get camera position and orientation
     * @param position Output: camera position
     * @param orientation Output: camera orientation
     * @param up Output: up vector
     */
    virtual void getCamera(float position[3], 
                          float orientation[4],
                          float up[3]) const = 0;
    
    // ========== Selection ==========
    
    /**
     * @brief Get currently selected objects
     * @return Vector of selected DocumentObjects
     */
    virtual std::vector<App::DocumentObject*> getSelection() const = 0;
    
    /**
     * @brief Set selection
     * @param objects Objects to select
     */
    virtual void setSelection(const std::vector<App::DocumentObject*>& objects) = 0;
    
    /**
     * @brief Clear selection
     */
    virtual void clearSelection() = 0;
    
    // ========== Interaction ==========
    
    /**
     * @brief Set navigation style
     * @param style Navigation style name (e.g., "CAD", "Blender", "Inventor")
     */
    virtual void setNavigationStyle(const std::string& style) = 0;
    
    /**
     * @brief Get current navigation style
     * @return Navigation style name
     */
    virtual std::string getNavigationStyle() const = 0;
    
    // ========== Capabilities ==========
    
    /**
     * @brief Check if a feature is supported
     * @param feature Feature name (e.g., "transparency", "shadows", "VR")
     * @return True if supported, false otherwise
     */
    virtual bool supportsFeature(const std::string& feature) const = 0;
    
    /**
     * @brief Get backend version information
     * @return Version string
     */
    virtual std::string getVersion() const = 0;
};

} // namespace Gui

#endif // GUI_VIEW3D_INTERFACES_IVIEWER3D_H
