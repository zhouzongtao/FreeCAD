// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "BackendRegistry.h"
#include "IBackendFactory.h"
#include "IViewer3D.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>

using namespace Gui;

namespace Gui {

// Destructor for viewer capsule - ensures proper cleanup
static void viewer_capsule_destructor(PyObject* capsule) {
    IViewer3D* viewer = static_cast<IViewer3D*>(
        PyCapsule_GetPointer(capsule, "IViewer3D")
    );
    if (viewer) {
        delete viewer;
    }
}

// Python wrapper for BackendRegistry
class BackendRegistryPy {
public:
    static PyObject* getAvailableBackends(PyObject* /*self*/, PyObject* /*args*/)
    {
        try {
            std::vector<std::string> backends = BackendRegistry::instance().getAvailableBackends();
            
            PyObject* list = PyList_New(backends.size());
            for (size_t i = 0; i < backends.size(); i++) {
                PyList_SetItem(list, i, PyUnicode_FromString(backends[i].c_str()));
            }
            
            return list;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return nullptr;
        }
    }
    
    static PyObject* isBackendAvailable(PyObject* /*self*/, PyObject* args)
    {
        const char* name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }
        
        try {
            bool available = BackendRegistry::instance().isBackendAvailable(name);
            return PyBool_FromLong(available ? 1 : 0);
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return nullptr;
        }
    }
    
    static PyObject* getDefaultBackend(PyObject* /*self*/, PyObject* /*args*/)
    {
        try {
            std::string backend = BackendRegistry::instance().getDefaultBackend();
            return PyUnicode_FromString(backend.c_str());
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return nullptr;
        }
    }
    
    static PyObject* setDefaultBackend(PyObject* /*self*/, PyObject* args)
    {
        const char* name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }
        
        try {
            bool success = BackendRegistry::instance().setDefaultBackend(name);
            return PyBool_FromLong(success ? 1 : 0);
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return nullptr;
        }
    }
    
    static PyObject* getBackendInfo(PyObject* /*self*/, PyObject* args)
    {
        const char* name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }
        
        try {
            std::map<std::string, std::string> info = BackendRegistry::instance().getBackendInfo(name);
            
            PyObject* dict = PyDict_New();
            for (const auto& pair : info) {
                PyDict_SetItemString(dict, 
                                    pair.first.c_str(), 
                                    PyUnicode_FromString(pair.second.c_str()));
            }
            
            return dict;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return nullptr;
        }
    }
    
    // Helper function to get or create the viewer wrapper class (cached)
    static PyObject* getViewerWrapperClass() {
        static PyObject* wrapper_class = nullptr;
        
        if (!wrapper_class) {
            // Create wrapper class in Python (only once)
            const char* wrapper_code = 
                "class Viewer3DWrapper:\n"
                "    def __init__(self, data):\n"
                "        self._viewer_ptr = data['_viewer_ptr']\n"
                "        self._backend_name = data['_backend_name']\n"
                "    \n"
                "    def getBackendName(self):\n"
                "        return self._backend_name\n"
                "    \n"
                "    def getWidget(self):\n"
                "        import FreeCADGui\n"
                "        return FreeCADGui.BackendRegistry._call_viewer_method(self._viewer_ptr, 'getWidget')\n"
                "    \n"
                "    def render(self):\n"
                "        import FreeCADGui\n"
                "        FreeCADGui.BackendRegistry._call_viewer_method(self._viewer_ptr, 'render')\n"
                "    \n"
                "    def viewAll(self):\n"
                "        import FreeCADGui\n"
                "        FreeCADGui.BackendRegistry._call_viewer_method(self._viewer_ptr, 'viewAll')\n"
                "    \n"
                "    def setBackgroundColor(self, color):\n"
                "        import FreeCADGui\n"
                "        FreeCADGui.BackendRegistry._call_viewer_method(self._viewer_ptr, 'setBackgroundColor', color)\n"
                "    \n"
                "    def clearScene(self):\n"
                "        import FreeCADGui\n"
                "        FreeCADGui.BackendRegistry._call_viewer_method(self._viewer_ptr, 'clearScene')\n"
                "    \n"
                "    def getVersion(self):\n"
                "        import FreeCADGui\n"
                "        return FreeCADGui.BackendRegistry._call_viewer_method(self._viewer_ptr, 'getVersion')\n"
                "\n"
                "_viewer_wrapper = Viewer3DWrapper\n";
            
            PyObject* main_module = PyImport_AddModule("__main__");
            PyObject* global_dict = PyModule_GetDict(main_module);
            
            PyRun_String(wrapper_code, Py_file_input, global_dict, global_dict);
            
            wrapper_class = PyDict_GetItemString(global_dict, "_viewer_wrapper");
            if (wrapper_class) {
                Py_INCREF(wrapper_class);  // Keep permanent reference
            }
        }
        
        return wrapper_class;
    }
    
    static PyObject* createViewer(PyObject* /*self*/, PyObject* args)
    {
        const char* name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }
        
        try {
            IViewer3D* viewer = BackendRegistry::instance().createViewer(name);
            
            if (!viewer) {
                Py_RETURN_NONE;
            }
            
            // Wrap IViewer3D* as a PyCapsule with destructor
            PyObject* capsule = PyCapsule_New(viewer, "IViewer3D", viewer_capsule_destructor);
            if (!capsule) {
                delete viewer;
                return nullptr;
            }
            
            // Create a dict to hold viewer data
            PyObject* viewer_dict = PyDict_New();
            if (!viewer_dict) {
                Py_DECREF(capsule);
                return nullptr;
            }
            
            // Store the capsule
            PyDict_SetItemString(viewer_dict, "_viewer_ptr", capsule);
            Py_DECREF(capsule);
            
            // Store backend name
            std::string backend_name = viewer->getBackendName();
            PyObject* name_obj = PyUnicode_FromString(backend_name.c_str());
            PyDict_SetItemString(viewer_dict, "_backend_name", name_obj);
            Py_DECREF(name_obj);
            
            // Get cached wrapper class
            PyObject* wrapper_class = getViewerWrapperClass();
            if (!wrapper_class) {
                Base::Console().error("BackendRegistry: Failed to get wrapper class\n");
                Py_DECREF(viewer_dict);
                return nullptr;
            }
            
            // Create instance
            PyObject* viewer_obj = PyObject_CallFunction(wrapper_class, "O", viewer_dict);
            Py_DECREF(viewer_dict);
            
            if (!viewer_obj) {
                Base::Console().error("BackendRegistry: Failed to create wrapper instance\n");
                return nullptr;
            }
            
            return viewer_obj;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return nullptr;
        }
    }
    
    // Helper method to call viewer methods from Python
    static PyObject* callViewerMethod(PyObject* /*self*/, PyObject* args)
    {
        PyObject* capsule;
        const char* method_name;
        PyObject* arg = nullptr;
        
        if (!PyArg_ParseTuple(args, "Os|O", &capsule, &method_name, &arg)) {
            return nullptr;
        }
        
        IViewer3D* viewer = static_cast<IViewer3D*>(PyCapsule_GetPointer(capsule, "IViewer3D"));
        if (!viewer) {
            PyErr_SetString(PyExc_RuntimeError, "Invalid viewer pointer");
            return nullptr;
        }
        
        std::string method(method_name);
        
        if (method == "getWidget") {
            QWidget* widget = viewer->getWidget();
            return PyLong_FromVoidPtr(widget);
        }
        else if (method == "render") {
            viewer->render();
            Py_RETURN_NONE;
        }
        else if (method == "viewAll") {
            viewer->viewAll();
            Py_RETURN_NONE;
        }
        else if (method == "setBackgroundColor") {
            if (!arg) {
                PyErr_SetString(PyExc_TypeError, "setBackgroundColor requires color argument");
                return nullptr;
            }
            
            PyObject* red_obj = PyObject_CallMethod(arg, "red", nullptr);
            PyObject* green_obj = PyObject_CallMethod(arg, "green", nullptr);
            PyObject* blue_obj = PyObject_CallMethod(arg, "blue", nullptr);
            
            if (!red_obj || !green_obj || !blue_obj) {
                Py_XDECREF(red_obj);
                Py_XDECREF(green_obj);
                Py_XDECREF(blue_obj);
                PyErr_SetString(PyExc_TypeError, "Expected QColor object");
                return nullptr;
            }
            
            int r = PyLong_AsLong(red_obj);
            int g = PyLong_AsLong(green_obj);
            int b = PyLong_AsLong(blue_obj);
            
            Py_DECREF(red_obj);
            Py_DECREF(green_obj);
            Py_DECREF(blue_obj);
            
            viewer->setBackgroundColor(QColor(r, g, b));
            Py_RETURN_NONE;
        }
        else if (method == "clearScene") {
            viewer->clearScene();
            Py_RETURN_NONE;
        }
        else if (method == "getVersion") {
            std::string version = viewer->getVersion();
            return PyUnicode_FromString(version.c_str());
        }
        
        PyErr_SetString(PyExc_AttributeError, ("Unknown method: " + method).c_str());
        return nullptr;
    }
    
    static PyObject* createDefaultViewer(PyObject* /*self*/, PyObject* /*args*/)
    {
        try {
            IViewer3D* viewer = BackendRegistry::instance().createDefaultViewer();
            
            if (!viewer) {
                Py_RETURN_NONE;
            }
            
            // Use same wrapping logic as createViewer
            PyObject* capsule = PyCapsule_New(viewer, "IViewer3D", viewer_capsule_destructor);
            if (!capsule) {
                delete viewer;
                return nullptr;
            }
            
            PyObject* viewer_dict = PyDict_New();
            if (!viewer_dict) {
                Py_DECREF(capsule);
                return nullptr;
            }
            
            PyDict_SetItemString(viewer_dict, "_viewer_ptr", capsule);
            Py_DECREF(capsule);
            
            std::string backend_name = viewer->getBackendName();
            PyObject* name_obj = PyUnicode_FromString(backend_name.c_str());
            PyDict_SetItemString(viewer_dict, "_backend_name", name_obj);
            Py_DECREF(name_obj);
            
            PyObject* wrapper_class = getViewerWrapperClass();
            if (!wrapper_class) {
                Base::Console().error("BackendRegistry: Failed to get wrapper class\n");
                Py_DECREF(viewer_dict);
                return nullptr;
            }
            
            PyObject* viewer_obj = PyObject_CallFunction(wrapper_class, "O", viewer_dict);
            Py_DECREF(viewer_dict);
            
            return viewer_obj;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return nullptr;
        }
    }
};

// Python method definitions
static PyMethodDef BackendRegistry_methods[] = {
    {"getAvailableBackends", BackendRegistryPy::getAvailableBackends, METH_NOARGS,
     "Get list of available rendering backends"},
    {"isBackendAvailable", BackendRegistryPy::isBackendAvailable, METH_VARARGS,
     "Check if a backend is available"},
    {"getDefaultBackend", BackendRegistryPy::getDefaultBackend, METH_NOARGS,
     "Get the default backend name"},
    {"setDefaultBackend", BackendRegistryPy::setDefaultBackend, METH_VARARGS,
     "Set the default backend"},
    {"getBackendInfo", BackendRegistryPy::getBackendInfo, METH_VARARGS,
     "Get backend information"},
    {"createViewer", BackendRegistryPy::createViewer, METH_VARARGS,
     "Create a viewer using specified backend"},
    {"createDefaultViewer", BackendRegistryPy::createDefaultViewer, METH_NOARGS,
     "Create a viewer using default backend"},
    {"_call_viewer_method", BackendRegistryPy::callViewerMethod, METH_VARARGS,
     "Internal: Call a viewer method"},
    {nullptr, nullptr, 0, nullptr}
};

// Module initialization
void initBackendRegistryPython()
{
    try {
        // Get FreeCADGui module
        PyObject* gui_module = PyImport_ImportModule("FreeCADGui");
        if (!gui_module) {
            Base::Console().error("BackendRegistry: Failed to import FreeCADGui module\n");
            PyErr_Print();
            return;
        }
        
        // Create a types.SimpleNamespace-like object
        PyObject* types_module = PyImport_ImportModule("types");
        if (!types_module) {
            Base::Console().error("BackendRegistry: Failed to import types module\n");
            Py_DECREF(gui_module);
            return;
        }
        
        PyObject* simple_namespace = PyObject_GetAttrString(types_module, "SimpleNamespace");
        Py_DECREF(types_module);
        
        if (!simple_namespace) {
            Base::Console().error("BackendRegistry: Failed to get SimpleNamespace\n");
            Py_DECREF(gui_module);
            return;
        }
        
        // Create an instance of SimpleNamespace
        PyObject* backend_registry = PyObject_CallObject(simple_namespace, nullptr);
        Py_DECREF(simple_namespace);
        
        if (!backend_registry) {
            Base::Console().error("BackendRegistry: Failed to create BackendRegistry namespace\n");
            Py_DECREF(gui_module);
            return;
        }
        
        // Add all methods to the namespace
        for (PyMethodDef* method = BackendRegistry_methods; method->ml_name != nullptr; ++method) {
            PyObject* func = PyCFunction_New(method, nullptr);
            if (func) {
                PyObject_SetAttrString(backend_registry, method->ml_name, func);
                Py_DECREF(func);
            }
        }
        
        // Add BackendRegistry to FreeCADGui module
        PyObject_SetAttrString(gui_module, "BackendRegistry", backend_registry);
        Py_DECREF(backend_registry);
        Py_DECREF(gui_module);
    }
    catch (const std::exception& e) {
        Base::Console().error("BackendRegistry: Exception during initialization: %s\n", e.what());
    }
    catch (...) {
        Base::Console().error("BackendRegistry: Unknown exception during initialization\n");
    }
}

} // namespace Gui
