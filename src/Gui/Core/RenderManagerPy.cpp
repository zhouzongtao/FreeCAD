/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
 *                                                                         *
 *   Python bindings for RenderManager                                     *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include "RenderManager.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>

namespace Gui {
namespace Core {

//===========================================================================
// Python 绑定 / Python Bindings
//===========================================================================

/**
 * @brief 切换渲染后端 / Switch render backend
 * 
 * Usage:
 *   switchRenderBackend(backendType)
 *   
 * Parameters:
 *   backendType: int - 0=None, 1=Coin3D, 2=OsgVerse
 *   
 * Returns:
 *   bool - True if successful
 */
static PyObject* switchRenderBackend(PyObject* /*self*/, PyObject* args)
{
    int backendType;
    if (!PyArg_ParseTuple(args, "i", &backendType)) {
        return nullptr;
    }

    if (backendType < 0 || backendType > 2) {
        PyErr_SetString(PyExc_ValueError, "Invalid backend type. Must be 0 (None), 1 (Coin3D), or 2 (OsgVerse)");
        return nullptr;
    }

    auto& manager = RenderManager::instance();
    bool success = manager.switchBackend(static_cast<Render::BackendType>(backendType));

    return PyBool_FromLong(success ? 1 : 0);
}

/**
 * @brief 获取当前渲染后端 / Get current render backend
 * 
 * Returns:
 *   int - 0=None, 1=Coin3D, 2=OsgVerse
 */
static PyObject* getCurrentRenderBackend(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto& manager = RenderManager::instance();
    int backend = static_cast<int>(manager.getCurrentBackend());

    return PyLong_FromLong(backend);
}

/**
 * @brief 检查后端是否可用 / Check if backend is available
 * 
 * Parameters:
 *   backendType: int - 0=None, 1=Coin3D, 2=OsgVerse
 *   
 * Returns:
 *   bool - True if available
 */
static PyObject* isRenderBackendAvailable(PyObject* /*self*/, PyObject* args)
{
    int backendType;
    if (!PyArg_ParseTuple(args, "i", &backendType)) {
        return nullptr;
    }

    if (backendType < 0 || backendType > 2) {
        PyErr_SetString(PyExc_ValueError, "Invalid backend type");
        return nullptr;
    }

    auto& manager = RenderManager::instance();
    bool available = manager.isBackendAvailable(static_cast<Render::BackendType>(backendType));

    return PyBool_FromLong(available ? 1 : 0);
}

/**
 * @brief 获取渲染器信息 / Get renderer info
 * 
 * Returns:
 *   str - Renderer name and version
 */
static PyObject* getRendererInfo(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto& manager = RenderManager::instance();
    std::string info = manager.getRendererInfo();

    return PyUnicode_FromString(info.c_str());
}

/**
 * @brief 获取渲染统计信息 / Get render statistics
 * 
 * Returns:
 *   dict - Statistics dictionary
 */
static PyObject* getRenderStats(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto& manager = RenderManager::instance();
    auto stats = manager.getStats();

    PyObject* dict = PyDict_New();
    PyDict_SetItemString(dict, "frameCount", PyLong_FromLong(stats.frameCount));
    PyDict_SetItemString(dict, "drawCalls", PyLong_FromLong(stats.drawCalls));
    PyDict_SetItemString(dict, "triangleCount", PyLong_FromLong(stats.triangleCount));
    PyDict_SetItemString(dict, "vertexCount", PyLong_FromLong(stats.vertexCount));
    PyDict_SetItemString(dict, "frameTime", PyFloat_FromDouble(stats.frameTime));
    PyDict_SetItemString(dict, "fps", PyFloat_FromDouble(stats.fps));

    return dict;
}

/**
 * @brief 重置统计信息 / Reset statistics
 */
static PyObject* resetRenderStats(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto& manager = RenderManager::instance();
    manager.resetStats();

    Py_RETURN_NONE;
}

//===========================================================================
// 方法表 / Method Table
//===========================================================================

static PyMethodDef RenderManager_methods[] = {
    {"switchRenderBackend", switchRenderBackend, METH_VARARGS,
     "switchRenderBackend(backendType) -> bool\n"
     "Switch to a different render backend.\n"
     "backendType: 0=None, 1=Coin3D, 2=OsgVerse\n"
     "Returns True if successful."},

    {"getCurrentRenderBackend", getCurrentRenderBackend, METH_VARARGS,
     "getCurrentRenderBackend() -> int\n"
     "Get the current render backend type.\n"
     "Returns: 0=None, 1=Coin3D, 2=OsgVerse"},

    {"isRenderBackendAvailable", isRenderBackendAvailable, METH_VARARGS,
     "isRenderBackendAvailable(backendType) -> bool\n"
     "Check if a render backend is available.\n"
     "backendType: 0=None, 1=Coin3D, 2=OsgVerse"},

    {"getRendererInfo", getRendererInfo, METH_VARARGS,
     "getRendererInfo() -> str\n"
     "Get information about the current renderer."},

    {"getRenderStats", getRenderStats, METH_VARARGS,
     "getRenderStats() -> dict\n"
     "Get rendering statistics."},

    {"resetRenderStats", resetRenderStats, METH_VARARGS,
     "resetRenderStats()\n"
     "Reset rendering statistics."},

    {nullptr, nullptr, 0, nullptr}  // Sentinel
};

//===========================================================================
// 模块初始化 / Module Initialization
//===========================================================================

PyDoc_STRVAR(module_doc,
    "RenderManager module\n"
    "\n"
    "This module provides access to FreeCAD's render backend management.\n"
    "\n"
    "Backend Types:\n"
    "  0 - None\n"
    "  1 - Coin3D (default, stable)\n"
    "  2 - OsgVerse (experimental, modern features)\n"
    "\n"
    "Example:\n"
    "  import FreeCADGui\n"
    "  # Check if OsgVerse is available\n"
    "  if FreeCADGui.isRenderBackendAvailable(2):\n"
    "      # Switch to OsgVerse\n"
    "      FreeCADGui.switchRenderBackend(2)\n"
    "      print('Switched to OsgVerse')\n"
);

void initRenderManagerPy()
{
    // 将方法添加到 FreeCADGui 模块
    // Add methods to FreeCADGui module
    Base::Interpreter().addMethod(RenderManager_methods);
    
    Base::Console().log("RenderManager Python bindings initialized\n");
}

} // namespace Core
} // namespace Gui
