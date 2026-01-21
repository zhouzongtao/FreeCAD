# SPDX-License-Identifier: LGPL-2.1-or-later

# OsgVerseGui - OsgVerse Rendering Backend Module

# Source files
set(OsgVerseGui_SRCS
    PreCompiled.h
    PreCompiled.cpp
    AppOsgVerseGui.cpp
    OsgVerseBackendFactory.h
    OsgVerseBackendFactory.cpp
    OsgVerseViewer.h
    OsgVerseViewer.cpp
    GeometryConverter.h
    GeometryConverter.cpp
)

# Create shared library
add_library(OsgVerseGui SHARED ${OsgVerseGui_SRCS})

# Include directories - MUST be set before precompiled headers
target_include_directories(OsgVerseGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Add OSG include directories - CRITICAL: Must be before PCH
find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgUtil osgViewer osgGA)
target_include_directories(OsgVerseGui SYSTEM PUBLIC
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

# Add OCCT include directories
target_include_directories(OsgVerseGui SYSTEM PUBLIC
    ${OCC_INCLUDE_DIR}
)

# Precompiled headers - AFTER include directories are set
if(FREECAD_USE_PCH)
    target_precompile_headers(OsgVerseGui PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

# Libraries to link
set(OsgVerseGui_LIBS
    FreeCADGui
    Part  # Can link Part module!
    ${OPENSCENEGRAPH_LIBRARIES}
    ${OCC_LIBRARIES}
)

# Link libraries
target_link_libraries(OsgVerseGui ${OsgVerseGui_LIBS})

# Compiler options
if (MSVC)
    target_compile_options(OsgVerseGui PRIVATE /wd4251 /wd4273)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(OsgVerseGui)
endif()

# Installation
SET_BIN_DIR(OsgVerseGui OsgVerseGui /Mod/OsgVerseGui)
SET_PYTHON_PREFIX_SUFFIX(OsgVerseGui)

INSTALL(TARGETS OsgVerseGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

message(STATUS "OsgVerseGui: Module configured")
message(STATUS "OsgVerseGui: OSG includes = ${OPENSCENEGRAPH_INCLUDE_DIRS}")
message(STATUS "OsgVerseGui: OSG libs = ${OPENSCENEGRAPH_LIBRARIES}")
message(STATUS "OsgVerseGui: OCCT includes = ${OCC_INCLUDE_DIR}")
