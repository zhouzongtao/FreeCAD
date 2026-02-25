// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#ifndef _PreComp_
# include <Python.h>
#endif

#include "CoinBackendFactory.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/View3D/Interfaces/BackendRegistry.h>

namespace CoinGui {

/**
 * @brief Initialize the CoinGui module
 * 
 * This function is called when the module is loaded.
 * It registers the Coin3D backend with the BackendRegistry.
 */
void initCoinGui()
{
    Base::Console().message("CoinGui: Initializing module\n");
    
    // Register the Coin3D backend
    CoinBackendFactory* factory = new CoinBackendFactory();
    bool registered = Gui::BackendRegistry::instance().registerBackend(factory);
    
    if (registered) {
        Base::Console().message("CoinGui: Backend registered successfully\n");
        
        // Set as default backend (for backward compatibility)
        Gui::BackendRegistry::instance().setDefaultBackend("Coin3D");
    } else {
        Base::Console().error("CoinGui: Failed to register backend\n");
    }
}

PyObject* initModule()
{
    // Create module definition
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "CoinGui",
        "Coin3D rendering backend module",
        -1,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };
    
    PyObject* mod = PyModule_Create(&moduledef);
    if (!mod) {
        return nullptr;
    }
    
    // Initialize the backend
    initCoinGui();
    
    return mod;
}

} // namespace CoinGui

// Python module initialization
PyMOD_INIT_FUNC(CoinGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load CoinGui module in console application.");
        PyMOD_Return(nullptr);
    }
    
    PyObject* mod = CoinGui::initModule();
    
    Base::Console().message("CoinGui: Module loaded successfully\n");
    
    PyMOD_Return(mod);
}
