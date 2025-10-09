add_library(Drawing SHARED)

target_include_directories(Drawing PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_include_directories(Drawing SYSTEM PRIVATE
    ${QtCore_INCLUDE_DIRS}
    ${QtWidgets_INCLUDE_DIRS}
)

set(Drawing_LIBS
    FreeCADApp
    Part
)

target_link_libraries(Drawing ${Drawing_LIBS})

# Generate Python bindings using AST/YAPTU - temporarily disabled
# generate_from_py(DrawingFeature)
# generate_from_py(Line)
# generate_from_py(Circle)

# Source files
SET(Python_SRCS
    # Temporarily disabled Python bindings
    # DrawingFeature.pyi
    # DrawingFeaturePyImp.cpp
    # Line.pyi
    # LinePyImp.cpp
    # Circle.pyi
    # CirclePyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Module_SRCS
    AppDrawing.cpp
    AppDrawingPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

SET(Drawing_SRCS
    DrawingFeature.cpp
    DrawingFeature.h
    # Temporarily disabled Python bindings
    # ${CMAKE_CURRENT_BINARY_DIR}/DrawingFeaturePy.h
    # ${CMAKE_CURRENT_BINARY_DIR}/DrawingFeaturePy.cpp
    # ${CMAKE_CURRENT_BINARY_DIR}/LinePy.h
    # ${CMAKE_CURRENT_BINARY_DIR}/LinePy.cpp
    # ${CMAKE_CURRENT_BINARY_DIR}/CirclePy.h
    # ${CMAKE_CURRENT_BINARY_DIR}/CirclePy.cpp
)
SOURCE_GROUP("Features" FILES ${Drawing_SRCS})

target_sources(Drawing PRIVATE
    ${Module_SRCS}
    ${Drawing_SRCS}
    ${Python_SRCS}
)

SET_BIN_DIR(Drawing Drawing /bin)

if(WIN32)
    set_target_properties(Drawing PROPERTIES SUFFIX ".pyd")
    set_target_properties(Drawing PROPERTIES DEBUG_OUTPUT_NAME "Drawing_d")
    set_target_properties(Drawing PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(Drawing PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif(WIN32)

INSTALL(TARGETS Drawing DESTINATION ${CMAKE_INSTALL_LIBDIR})
