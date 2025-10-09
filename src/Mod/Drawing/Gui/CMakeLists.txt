add_library(DrawingGui SHARED)

target_include_directories(DrawingGui PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_include_directories(DrawingGui SYSTEM PRIVATE
    ${QtCore_INCLUDE_DIRS}
    ${QtWidgets_INCLUDE_DIRS}
    ${QtOpenGL_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${SOQT_INCLUDE_DIRS}
)

set(DrawingGui_LIBS
    FreeCADGui
    Drawing
)

target_link_libraries(DrawingGui ${DrawingGui_LIBS})

# Note: GUI components typically don't need Python bindings
# ViewProviders are usually accessed through the App objects they represent
# Workbench is registered through InitGui.py

# Source files
SET(Python_SRCS
    # No Python bindings needed for GUI components in this implementation
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Module_SRCS
    AppDrawingGui.cpp
    AppDrawingGuiPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

SET(Command_SRCS
    # Command.cpp  # Temporarily disabled due to compilation issues
    # Command.h
)
SOURCE_GROUP("Command" FILES ${Command_SRCS})

SET(ViewProvider_SRCS
    ViewProviderDrawing.cpp
    ViewProviderDrawing.h
)
SOURCE_GROUP("ViewProvider" FILES ${ViewProvider_SRCS})

SET(Workbench_SRCS
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Workbench" FILES ${Workbench_SRCS})

target_sources(DrawingGui PRIVATE
    ${Module_SRCS}
    # ${Command_SRCS}  # Temporarily disabled
    ${ViewProvider_SRCS}
    ${Workbench_SRCS}
)

SET_BIN_DIR(DrawingGui DrawingGui /bin)

if(WIN32)
    set_target_properties(DrawingGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(DrawingGui PROPERTIES DEBUG_OUTPUT_NAME "DrawingGui_d")
    set_target_properties(DrawingGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(DrawingGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif(WIN32)

INSTALL(TARGETS DrawingGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
