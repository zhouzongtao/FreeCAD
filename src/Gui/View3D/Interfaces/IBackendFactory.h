// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * IBackendFactory.h - Backend Factory Interface
 * 
 * This interface defines how rendering backends are created and destroyed.
 */

#ifndef GUI_VIEW3D_INTERFACES_IBACKENDFACTORY_H
#define GUI_VIEW3D_INTERFACES_IBACKENDFACTORY_H

#include <string>
#include <QWidget>

namespace Gui {

class IViewer3D;

/**
 * @brief Abstract factory for creating 3D viewer backends
 * 
 * Each rendering backend (Coin3D, OsgVerse) must provide a factory
 * that implements this interface.
 */
class GuiExport IBackendFactory {
public:
    virtual ~IBackendFactory() = default;
    
    /**
     * @brief Get the name of this backend
     * @return Backend name (e.g., "Coin3D", "OsgVerse")
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Get a human-readable description
     * @return Description string
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Create a new viewer instance
     * @param parent Parent widget
     * @return New IViewer3D instance (caller takes ownership)
     */
    virtual IViewer3D* createViewer(QWidget* parent = nullptr) = 0;
    
    /**
     * @brief Destroy a viewer instance
     * @param viewer Viewer to destroy
     */
    virtual void destroyViewer(IViewer3D* viewer) = 0;
    
    /**
     * @brief Check if this backend is available
     * @return True if backend can be used, false otherwise
     */
    virtual bool isAvailable() const = 0;
    
    /**
     * @brief Get backend version
     * @return Version string
     */
    virtual std::string getVersion() const = 0;
    
    /**
     * @brief Get initialization priority (lower = higher priority)
     * @return Priority value (0-100, default backends use 50)
     */
    virtual int getPriority() const { return 50; }
};

} // namespace Gui

#endif // GUI_VIEW3D_INTERFACES_IBACKENDFACTORY_H
