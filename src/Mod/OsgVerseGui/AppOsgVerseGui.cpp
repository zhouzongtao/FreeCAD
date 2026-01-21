// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseBackendFactory.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/View3D/Interfaces/BackendRegistry.h>

namespace OsgVerseGui {

extern "C" {

/**
 * @brief Module initialization function
 * 
 * This function is called when the OsgVerseGui module is loaded.
 * It registers the OsgVerse rendering backend with the BackendRegistry.
 */
void OsgVerseGuiExport initOsgVerseGui()
{
    Base::Console().message("OsgVerseGui: Initializing module\n");
    
    // Register the OsgVerse backend
    auto* factory = new OsgVerseBackendFactory();
    bool registered = Gui::BackendRegistry::instance().registerBackend(factory);
    
    if (registered) {
        Base::Console().message("OsgVerseGui: Backend registered successfully\n");
        
        // Note: We don't set OsgVerse as default backend
        // Coin3D remains the default (priority 10 vs 5)
        // Users can switch to OsgVerse manually
    }
    else {
        Base::Console().error("OsgVerseGui: Failed to register backend\n");
        delete factory;
    }
    
    Base::Console().message("OsgVerseGui: Module initialized\n");
}

} // extern "C"

} // namespace OsgVerseGui

//===========================================================================
// Python module definition
//===========================================================================

namespace OsgVerseGui {

PyDoc_STRVAR(module_OsgVerseGui_doc,
"OsgVerseGui module - OsgVerse rendering backend for FreeCAD\n"
"\n"
"This module provides an alternative rendering backend using OpenSceneGraph\n"
"and OsgVerse. It can be used instead of the default Coin3D backend.\n"
"\n"
"Usage:\n"
"  import OsgVerseGui\n"
"  from Gui import BackendRegistry\n"
"  BackendRegistry.setDefaultBackend('OsgVerse')\n"
);

struct PyMethodDef OsgVerseGui_methods[] = {
    {nullptr, nullptr, 0, nullptr}  // Sentinel
};

struct PyModuleDef OsgVerseGuiModule = {
    PyModuleDef_HEAD_INIT,
    "OsgVerseGui",              // Module name
    module_OsgVerseGui_doc,     // Module documentation
    -1,                         // Size of per-interpreter state, -1 means global state
    OsgVerseGui_methods,        // Module methods
    nullptr,                    // m_reload
    nullptr,                    // m_traverse
    nullptr,                    // m_clear
    nullptr                     // m_free
};

} // namespace OsgVerseGui

PyMODINIT_FUNC PyInit_OsgVerseGui()
{
    // Initialize the module
    OsgVerseGui::initOsgVerseGui();
    
    // Create Python module
    PyObject* module = PyModule_Create(&OsgVerseGui::OsgVerseGuiModule);
    
    if (module == nullptr) {
        Base::Console().error("OsgVerseGui: Failed to create Python module\n");
        return nullptr;
    }
    
    Base::Console().message("OsgVerseGui: Python module created\n");
    
    return module;
}
