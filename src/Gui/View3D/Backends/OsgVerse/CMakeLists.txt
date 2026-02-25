# OsgVerse Viewer Adapter CMake Configuration

if(BUILD_WITH_OSGVERSE)
    message(STATUS "Building OsgVerse Viewer Adapter")

    set(OsgVerseAdapter_SRCS
        OsgVerseViewerAdapter.cpp
        PreCompiled.cpp
    )

    set(OsgVerseAdapter_HDRS
        OsgVerseViewerAdapter.h
        PreCompiled.h
    )

    # Add sources to parent FreeCADGui target
    target_sources(FreeCADGui PRIVATE
        ${OsgVerseAdapter_SRCS}
        ${OsgVerseAdapter_HDRS}
    )

    # Set source group for IDE organization
    source_group("View3D\\Backends\\OsgVerse" FILES
        ${OsgVerseAdapter_SRCS}
        ${OsgVerseAdapter_HDRS}
    )

    message(STATUS "OsgVerse Viewer Adapter configured")
endif()
