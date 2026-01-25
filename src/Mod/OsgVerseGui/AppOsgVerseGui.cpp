// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseViewer.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/View3D/ViewerFactory.h>
#include <Gui/Render/Core/RenderTypes.h>

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
    
    // Note: We no longer register with BackendRegistry (old interface)
    // OsgVerseViewer now implements the new Gui::View3D::IViewer3D interface
    
    // Register with ViewerFactory (for View3DInventor and automatic 3D view creation)
    Gui::View3D::ViewerFactory::registerCreator(
        Gui::Render::BackendType::OsgVerse,
        [](QWidget* parent, const QOpenGLWidget* /*shareWidget*/) -> std::unique_ptr<Gui::View3D::IViewer3D> {
            Base::Console().message("OsgVerseGui: ViewerFactory creating OsgVerse viewer\n");
            try {
                return std::make_unique<OsgVerseViewer>(parent);
            }
            catch (const std::exception& e) {
                Base::Console().error("OsgVerseGui: Failed to create viewer: %s\n", e.what());
                return nullptr;
            }
        }
    );
    Base::Console().message("OsgVerseGui: Registered with ViewerFactory\n");
    
    Base::Console().message("OsgVerseGui: Module initialized successfully\n");
    Base::Console().message("OsgVerseGui: OsgVerse backend is now available for 3D views\n");
    Base::Console().message("OsgVerseGui: Use RenderManager to switch to OsgVerse backend\n");
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
