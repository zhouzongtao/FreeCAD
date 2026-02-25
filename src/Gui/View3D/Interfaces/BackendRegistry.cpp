// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "BackendRegistry.h"
#include "IBackendFactory.h"
#include "IViewer3D.h"
#include <Base/Console.h>
#include <Base/Parameter.h>

using namespace Gui;

// ========== Implementation Details ==========

struct BackendRegistry::Impl {
    std::map<std::string, IBackendFactory*> backends;
    std::string defaultBackend;
    
    ~Impl() {
        // Clean up all registered factories
        for (auto& pair : backends) {
            delete pair.second;
        }
        backends.clear();
    }
};

// ========== BackendRegistry Implementation ==========

BackendRegistry::BackendRegistry()
    : pImpl(new Impl())
{
    // Load default backend from preferences
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");
    std::string defaultName = hGrp->GetASCII("DefaultRenderingBackend", "Coin3D");
    pImpl->defaultBackend = defaultName;
    
    Base::Console().message("BackendRegistry: Initialized with default backend '%s'\n", 
                       defaultName.c_str());
}

BackendRegistry::~BackendRegistry()
{
    // pImpl destructor will clean up
}

BackendRegistry& BackendRegistry::instance()
{
    static BackendRegistry registry;
    return registry;
}

bool BackendRegistry::registerBackend(IBackendFactory* factory)
{
    if (!factory) {
        Base::Console().warning("BackendRegistry: Cannot register null factory\n");
        return false;
    }
    
    std::string name = factory->getName();
    
    // Check if already registered
    if (pImpl->backends.find(name) != pImpl->backends.end()) {
        Base::Console().warning("BackendRegistry: Backend '%s' already registered\n", 
                               name.c_str());
        delete factory;
        return false;
    }
    
    // Check if backend is available
    if (!factory->isAvailable()) {
        Base::Console().warning("BackendRegistry: Backend '%s' is not available\n", 
                               name.c_str());
        delete factory;
        return false;
    }
    
    // Register
    pImpl->backends[name] = factory;
    
    Base::Console().message("BackendRegistry: Registered backend '%s' (version %s)\n",
                       name.c_str(), factory->getVersion().c_str());
    
    // If this is the first backend, make it default
    if (pImpl->backends.size() == 1) {
        pImpl->defaultBackend = name;
        Base::Console().message("BackendRegistry: Set '%s' as default backend\n", name.c_str());
    }
    
    return true;
}

bool BackendRegistry::unregisterBackend(const std::string& name)
{
    auto it = pImpl->backends.find(name);
    if (it == pImpl->backends.end()) {
        Base::Console().warning("BackendRegistry: Backend '%s' not found\n", name.c_str());
        return false;
    }
    
    delete it->second;
    pImpl->backends.erase(it);
    
    Base::Console().message("BackendRegistry: Unregistered backend '%s'\n", name.c_str());
    
    // If this was the default, choose another
    if (pImpl->defaultBackend == name) {
        if (!pImpl->backends.empty()) {
            pImpl->defaultBackend = pImpl->backends.begin()->first;
            Base::Console().message("BackendRegistry: Changed default backend to '%s'\n",
                               pImpl->defaultBackend.c_str());
        } else {
            pImpl->defaultBackend.clear();
            Base::Console().warning("BackendRegistry: No backends available\n");
        }
    }
    
    return true;
}

IBackendFactory* BackendRegistry::getBackend(const std::string& name) const
{
    auto it = pImpl->backends.find(name);
    if (it != pImpl->backends.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> BackendRegistry::getAvailableBackends() const
{
    std::vector<std::string> names;
    names.reserve(pImpl->backends.size());
    
    for (const auto& pair : pImpl->backends) {
        names.push_back(pair.first);
    }
    
    return names;
}

bool BackendRegistry::isBackendAvailable(const std::string& name) const
{
    auto it = pImpl->backends.find(name);
    if (it != pImpl->backends.end()) {
        return it->second->isAvailable();
    }
    return false;
}

bool BackendRegistry::setDefaultBackend(const std::string& name)
{
    if (!isBackendAvailable(name)) {
        Base::Console().warning("BackendRegistry: Cannot set default to unavailable backend '%s'\n",
                               name.c_str());
        return false;
    }
    
    pImpl->defaultBackend = name;
    
    // Save to preferences
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");
    hGrp->SetASCII("DefaultRenderingBackend", name.c_str());
    
    Base::Console().message("BackendRegistry: Set default backend to '%s'\n", name.c_str());
    
    return true;
}

std::string BackendRegistry::getDefaultBackend() const
{
    return pImpl->defaultBackend;
}

IViewer3D* BackendRegistry::createDefaultViewer(QWidget* parent)
{
    if (pImpl->defaultBackend.empty()) {
        Base::Console().error("BackendRegistry: No default backend set\n");
        return nullptr;
    }
    
    return createViewer(pImpl->defaultBackend, parent);
}

IViewer3D* BackendRegistry::createViewer(const std::string& backendName, QWidget* parent)
{
    IBackendFactory* factory = getBackend(backendName);
    if (!factory) {
        Base::Console().error("BackendRegistry: Backend '%s' not found\n", backendName.c_str());
        return nullptr;
    }
    
    if (!factory->isAvailable()) {
        Base::Console().error("BackendRegistry: Backend '%s' is not available\n", 
                             backendName.c_str());
        return nullptr;
    }
    
    try {
        IViewer3D* viewer = factory->createViewer(parent);
        if (viewer) {
            Base::Console().message("BackendRegistry: Created viewer using backend '%s'\n",
                               backendName.c_str());
        } else {
            Base::Console().error("BackendRegistry: Failed to create viewer for backend '%s'\n",
                                 backendName.c_str());
        }
        return viewer;
    }
    catch (const std::exception& e) {
        Base::Console().error("BackendRegistry: Exception creating viewer: %s\n", e.what());
        return nullptr;
    }
}

std::map<std::string, std::string> BackendRegistry::getBackendInfo(const std::string& name) const
{
    std::map<std::string, std::string> info;
    
    IBackendFactory* factory = getBackend(name);
    if (factory) {
        info["name"] = factory->getName();
        info["description"] = factory->getDescription();
        info["version"] = factory->getVersion();
        info["available"] = factory->isAvailable() ? "true" : "false";
        info["priority"] = std::to_string(factory->getPriority());
    }
    
    return info;
}
