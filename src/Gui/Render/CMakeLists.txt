# FreeCAD Rendering Abstraction Layer CMake Configuration
# 渲染抽象层 CMake 配置

# 收集头文件 / Collect header files
set(RENDER_PUBLIC_HEADERS
    Core/RenderTypes.h
    Core/RenderNode.h
    Core/RenderEngine.h
    Core/RenderViewer.h
)

set(RENDER_PRIVATE_HEADERS
    Backends/Coin3D/Coin3DEngine.h
    Backends/Coin3D/Coin3DNode.h
    Backends/Coin3D/Coin3DMaterial.h
    Backends/Coin3D/Coin3DGeometry.h
    Backends/Coin3D/Coin3DUtils.h
    Backends/Coin3D/Coin3DViewer.h
)

set(RENDER_SOURCES
    Core/RenderNode.cpp
    Core/RenderEngine.cpp
)

# Coin3D 后端源文件 / Coin3D backend sources
# Coin3D is always available in FreeCAD, so always include these sources
list(APPEND RENDER_SOURCES
    Backends/Coin3D/Coin3DNode.cpp
    Backends/Coin3D/Coin3DMaterial.cpp
    Backends/Coin3D/Coin3DGeometry.cpp
    Backends/Coin3D/Coin3DUtils.cpp
    Backends/Coin3D/Coin3DEngine.cpp
    # Backends/Coin3D/Coin3DViewer.cpp  # TODO: Fix API compatibility issues
)

# 添加 Coin3D 包含目录 / Add Coin3D include directories
target_include_directories(FreeCADGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Backends/Coin3D
)

# OsgVerse 后端源文件 / OsgVerse backend sources
# OsgVerse backend is optional and configured in its own CMakeLists.txt
add_subdirectory(Backends/OsgVerse)

# 创建源组 / Create source groups
source_group("Render\\Core" FILES
    Core/RenderTypes.h
    Core/RenderNode.h
    Core/RenderNode.cpp
    Core/RenderEngine.h
    Core/RenderEngine.cpp
    Core/RenderViewer.h
)

source_group("Render\\Backends\\Coin3D" FILES
    Backends/Coin3D/Coin3DEngine.h
    Backends/Coin3D/Coin3DEngine.cpp
    Backends/Coin3D/Coin3DNode.h
    Backends/Coin3D/Coin3DNode.cpp
    Backends/Coin3D/Coin3DMaterial.h
    Backends/Coin3D/Coin3DMaterial.cpp
    Backends/Coin3D/Coin3DGeometry.h
    Backends/Coin3D/Coin3DGeometry.cpp
    Backends/Coin3D/Coin3DUtils.h
    Backends/Coin3D/Coin3DUtils.cpp
    Backends/Coin3D/Coin3DViewer.h
    Backends/Coin3D/Coin3DViewer.cpp
)

source_group("Render\\Backends\\OsgVerse" FILES
    # Backends/OsgVerse/OsgVerseEngine.h  # 在 Phase 3 创建 / Created in Phase 3
)

source_group("Render\\Utils" FILES
    Utils/GeometryConverter.h
    Utils/MaterialConverter.h
)

# 将源文件添加到 FreeCADGui 库
# Add source files to FreeCADGui library
target_sources(FreeCADGui PRIVATE
    ${RENDER_PUBLIC_HEADERS}
    ${RENDER_PRIVATE_HEADERS}
    ${RENDER_SOURCES}
)

# 添加包含目录 / Add include directories
target_include_directories(FreeCADGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Render
)

# Coin3D 依赖 / Coin3D dependencies
# Coin3D is always available in FreeCAD
target_compile_definitions(FreeCADGui PRIVATE
    RENDER_HAS_COIN3D_BACKEND
)

# OsgVerse 依赖 / OsgVerse dependencies
if(BUILD_WITH_OSGVERSE)
    target_compile_definitions(FreeCADGui PRIVATE
        RENDER_HAS_OSGVERSE_BACKEND
    )
    # TODO: 添加 OsgVerse 库链接
    # target_link_libraries(FreeCADGui PRIVATE ${OSGVERSE_LIBRARIES})
endif()

# 单元测试 / Unit tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
