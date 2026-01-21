// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_EXPORT_H
#define OSGVERSEGUI_EXPORT_H

#include <FCConfig.h>

// Export/Import macros
#ifdef FC_OS_WIN32
# define OsgVerseGuiExport __declspec(dllexport)
#else // for Linux
# define OsgVerseGuiExport
#endif

#endif // OSGVERSEGUI_EXPORT_H
