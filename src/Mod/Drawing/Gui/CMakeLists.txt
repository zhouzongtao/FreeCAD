# Drawing GUI module with Skia rendering

add_library(DrawingGui SHARED)

# Skia configuration - platform specific paths
if(WIN32)
    set(SKIA_DIR "E:/Repository/skia")
    set(SKIA_BUILD_DIR "${SKIA_DIR}/out/msvc.x64.release.md")
elseif(APPLE)
    set(SKIA_DIR "/Users/zhouzongtao/repository/skia")
    set(SKIA_BUILD_DIR "${SKIA_DIR}/out/Release")
else()
    set(SKIA_DIR "/usr/local/skia")
    set(SKIA_BUILD_DIR "${SKIA_DIR}/out/Release")
endif()

# Allow override via CMake variable
if(DEFINED SKIA_ROOT)
    set(SKIA_DIR "${SKIA_ROOT}")
endif()
if(DEFINED SKIA_BUILD_ROOT)
    set(SKIA_BUILD_DIR "${SKIA_BUILD_ROOT}")
endif()

message(STATUS "Skia directory: ${SKIA_DIR}")
message(STATUS "Skia build directory: ${SKIA_BUILD_DIR}")

# Skia include directories
target_include_directories(DrawingGui PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${SKIA_DIR}
)

target_include_directories(DrawingGui SYSTEM PRIVATE
    ${QtCore_INCLUDE_DIRS}
    ${QtWidgets_INCLUDE_DIRS}
    ${QtOpenGL_INCLUDE_DIRS}
    ${QtOpenGLWidgets_INCLUDE_DIRS}
)

# Find Skia static library
if(WIN32)
    find_library(SKIA_LIBRARY
        NAMES skia
        PATHS ${SKIA_BUILD_DIR}
        NO_DEFAULT_PATH
    )
    if(NOT SKIA_LIBRARY)
        set(SKIA_LIBRARY "${SKIA_BUILD_DIR}/skia.lib")
    endif()
else()
    find_library(SKIA_LIBRARY
        NAMES skia
        PATHS ${SKIA_BUILD_DIR}
        NO_DEFAULT_PATH
    )
    if(NOT SKIA_LIBRARY)
        set(SKIA_LIBRARY "${SKIA_BUILD_DIR}/libskia.a")
    endif()
endif()

message(STATUS "Skia library: ${SKIA_LIBRARY}")

# Find OpenGL
find_package(OpenGL REQUIRED)
message(STATUS "OpenGL found: ${OPENGL_LIBRARIES}")

# Platform-specific dependencies
if(APPLE)
    # macOS frameworks required by Skia with Metal backend
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
    find_library(CORETEXT_FRAMEWORK CoreText)
    find_library(APPKIT_FRAMEWORK AppKit)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(IOSURFACE_FRAMEWORK IOSurface)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    find_library(COCOA_FRAMEWORK Cocoa)
    
    set(SKIA_FRAMEWORKS
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
        ${CORETEXT_FRAMEWORK}
        ${APPKIT_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${IOSURFACE_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${COCOA_FRAMEWORK}
    )
elseif(WIN32)
    # Windows libraries required by Skia
    set(SKIA_WIN_LIBS
        opengl32
        user32
        gdi32
        ole32
        oleaut32
        uuid
        fontsub
        usp10
    )
endif()

# Libraries to link
set(DrawingGui_LIBS
    FreeCADGui
    Drawing
    ${SKIA_LIBRARY}
    ${OPENGL_LIBRARIES}
    Qt::OpenGL
    Qt::OpenGLWidgets
)

if(APPLE)
    list(APPEND DrawingGui_LIBS ${SKIA_FRAMEWORKS})
elseif(WIN32)
    list(APPEND DrawingGui_LIBS ${SKIA_WIN_LIBS})
endif()

target_link_libraries(DrawingGui ${DrawingGui_LIBS})

# Compiler definitions for Skia
# SK_GANESH: Enable GPU rendering
# SK_GL: Enable OpenGL backend (cross-platform)
# SK_METAL: Enable Metal backend (macOS only)
target_compile_definitions(DrawingGui PRIVATE
    SK_RELEASE
    SK_GANESH
    SK_GL
)

if(APPLE)
    target_compile_definitions(DrawingGui PRIVATE SK_METAL)
endif()

# Source files
SET(Module_SRCS
    AppDrawingGui.cpp
    AppDrawingGuiPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

SET(Command_SRCS
    Command.cpp
    Command.h
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

SET(Skia_SRCS
    SkiaCanvas.cpp
    SkiaCanvas.h
    SkiaTypes.h
    MDIViewDrawing.cpp
    MDIViewDrawing.h
)
SOURCE_GROUP("Skia" FILES ${Skia_SRCS})

# OpenGL GPU canvas (cross-platform)
SET(SkiaOpenGL_SRCS
    SkiaOpenGLCanvas.cpp
    SkiaOpenGLCanvas.h
)
SOURCE_GROUP("SkiaOpenGL" FILES ${SkiaOpenGL_SRCS})

# Metal GPU canvas (macOS only)
if(APPLE)
    SET(SkiaMetal_SRCS
        SkiaMetalCanvas.mm
        SkiaMetalCanvas.h
    )
    SOURCE_GROUP("SkiaMetal" FILES ${SkiaMetal_SRCS})
    
    # Enable Objective-C++ for .mm files
    set_source_files_properties(SkiaMetalCanvas.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

target_sources(DrawingGui PRIVATE
    ${Module_SRCS}
    ${Command_SRCS}
    ${ViewProvider_SRCS}
    ${Workbench_SRCS}
    ${Skia_SRCS}
    ${SkiaOpenGL_SRCS}
)

if(APPLE)
    target_sources(DrawingGui PRIVATE ${SkiaMetal_SRCS})
endif()

SET_BIN_DIR(DrawingGui DrawingGui /Mod/Drawing)
SET_PYTHON_PREFIX_SUFFIX(DrawingGui)

if(WIN32)
    set_target_properties(DrawingGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(DrawingGui PROPERTIES DEBUG_OUTPUT_NAME "DrawingGui_d")
endif(WIN32)

INSTALL(TARGETS DrawingGui DESTINATION ${CMAKE_INSTALL_LIBDIR}/Mod/Drawing)
