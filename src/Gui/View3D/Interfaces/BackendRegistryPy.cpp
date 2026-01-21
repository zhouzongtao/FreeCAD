// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "BackendRegistry.h"
#include "IBackendFactory.h"
#include "IViewer3D.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>

using namespace Gui;

namespace Gui {

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
            
            // TODO: Wrap IViewer3D in Python object
            // For now, just return a placeholder
            Base::Console().warning("BackendRegistry.createViewer: Python wrapper not yet implemented\n");
            Py_RETURN_NONE;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return nullptr;
        }
    }
    
    static PyObject* createDefaultViewer(PyObject* /*self*/, PyObject* /*args*/)
    {
        try {
            IViewer3D* viewer = BackendRegistry::instance().createDefaultViewer();
            
            if (!viewer) {
                Py_RETURN_NONE;
            }
            
            // TODO: Wrap IViewer3D in Python object
            // For now, just return a placeholder
            Base::Console().warning("BackendRegistry.createDefaultViewer: Python wrapper not yet implemented\n");
            Py_RETURN_NONE;
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
    {nullptr, nullptr, 0, nullptr}
};

// Module initialization
void initBackendRegistryPython()
{
    Base::Console().message("BackendRegistry: Initializing Python bindings\n");
    
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
        
        Base::Console().message("BackendRegistry: Python bindings initialized successfully\n");
    }
    catch (const std::exception& e) {
        Base::Console().error("BackendRegistry: Exception during initialization: %s\n", e.what());
    }
    catch (...) {
        Base::Console().error("BackendRegistry: Unknown exception during initialization\n");
    }
}

} // namespace Gui
