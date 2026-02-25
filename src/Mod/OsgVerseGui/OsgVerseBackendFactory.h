// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_BACKENDACTORY_H
#define OSGVERSEGUI_BACKENDACTORY_H

#include "PreCompiled.h"
#include <Gui/View3D/Interfaces/IBackendFactory.h>

namespace OsgVerseGui {

/**
 * @brief Factory for creating OsgVerse rendering backend viewers
 * 
 * This factory creates OsgVerseViewer instances that use OSG/OsgVerse
 * for 3D rendering.
 */
class OsgVerseGuiExport OsgVerseBackendFactory : public Gui::IBackendFactory {
public:
    OsgVerseBackendFactory();
    virtual ~OsgVerseBackendFactory();
    
    // IBackendFactory interface
    std::string getName() const override;
    std::string getDescription() const override;
    Gui::IViewer3D* createViewer(QWidget* parent = nullptr) override;
    void destroyViewer(Gui::IViewer3D* viewer) override;
    bool isAvailable() const override;
    std::string getVersion() const override;
    int getPriority() const override;
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_BACKENDACTORY_H
