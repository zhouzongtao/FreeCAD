// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef COINGUI_PRECOMPILED_H
#define COINGUI_PRECOMPILED_H

#include <FCConfig.h>

// Std
#include <string>
#include <vector>
#include <map>
#include <memory>

// Qt
#include <QWidget>
#include <QColor>
#include <QApplication>

// FreeCAD Base
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/DocumentObject.h>

// FreeCAD Gui
#include <Gui/ViewProvider.h>
#include <Gui/View3D/Interfaces/IViewer3D.h>
#include <Gui/View3D/Interfaces/IBackendFactory.h>
#include <Gui/View3D/Interfaces/BackendRegistry.h>

// Note: Coin3D headers are included in source files, not here
// to avoid precompiled header issues

#endif // COINGUI_PRECOMPILED_H
