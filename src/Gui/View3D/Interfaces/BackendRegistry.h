// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * BackendRegistry.h - Registry for 3D Rendering Backends
 * 
 * This singleton manages all available rendering backends and provides
 * a unified interface for backend selection and creation.
 */

#ifndef GUI_VIEW3D_INTERFACES_BACKENDREGISTRY_H
#define GUI_VIEW3D_INTERFACES_BACKENDREGISTRY_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Gui {

class IBackendFactory;
class IViewer3D;

/**
 * @brief Registry for managing 3D rendering backends
 * 
 * This singleton class maintains a registry of all available rendering backends
 * and provides methods for backend selection and viewer creation.
 */
class GuiExport BackendRegistry {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the BackendRegistry instance
     */
    static BackendRegistry& instance();
    
    // Prevent copying
    BackendRegistry(const BackendRegistry&) = delete;
    BackendRegistry& operator=(const BackendRegistry&) = delete;
    
    /**
     * @brief Register a rendering backend
     * @param factory Backend factory (registry takes ownership)
     * @return True if registered successfully, false if name already exists
     */
    bool registerBackend(IBackendFactory* factory);
    
    /**
     * @brief Unregister a rendering backend
     * @param name Backend name
     * @return True if unregistered successfully, false if not found
     */
    bool unregisterBackend(const std::string& name);
    
    /**
     * @brief Get a backend factory by name
     * @param name Backend name
     * @return Factory pointer, or nullptr if not found
     */
    IBackendFactory* getBackend(const std::string& name) const;
    
    /**
     * @brief Get all available backend names
     * @return Vector of backend names
     */
    std::vector<std::string> getAvailableBackends() const;
    
    /**
     * @brief Check if a backend is available
     * @param name Backend name
     * @return True if backend exists and is available
     */
    bool isBackendAvailable(const std::string& name) const;
    
    /**
     * @brief Set the default backend
     * @param name Backend name
     * @return True if set successfully, false if backend not found
     */
    bool setDefaultBackend(const std::string& name);
    
    /**
     * @brief Get the default backend name
     * @return Default backend name, or empty string if none set
     */
    std::string getDefaultBackend() const;
    
    /**
     * @brief Create a viewer using the default backend
     * @param parent Parent widget
     * @return New viewer instance, or nullptr if no backend available
     */
    IViewer3D* createDefaultViewer(QWidget* parent = nullptr);
    
    /**
     * @brief Create a viewer using a specific backend
     * @param backendName Backend name
     * @param parent Parent widget
     * @return New viewer instance, or nullptr if backend not found
     */
    IViewer3D* createViewer(const std::string& backendName, QWidget* parent = nullptr);
    
    /**
     * @brief Get backend information
     * @param name Backend name
     * @return Map of info keys to values (name, description, version, etc.)
     */
    std::map<std::string, std::string> getBackendInfo(const std::string& name) const;
    
private:
    BackendRegistry();
    ~BackendRegistry();
    
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

// Python bindings initialization
GuiExport void initBackendRegistryPython();

} // namespace Gui

#endif // GUI_VIEW3D_INTERFACES_BACKENDREGISTRY_H
