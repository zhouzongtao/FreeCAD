# View3D abstraction layer

set(View3D_SRCS
    PreCompiled.h
    PreCompiled.cpp
    IViewer3D.h
    ViewerFactory.h
    ViewerFactory.cpp
)

# Interface layer (Phase 1 - Backend Modularization)
set(View3D_Interface_SRCS
    Interfaces/PreCompiled.h
    Interfaces/IViewer3D.h
    Interfaces/IBackendFactory.h
    Interfaces/BackendRegistry.h
    Interfaces/BackendRegistry.cpp
    Interfaces/BackendRegistryPy.cpp
)

set(View3D_Coin_SRCS
    Backends/Coin/CoinViewer.h
    Backends/Coin/CoinViewer.cpp
)

# 添加到 FreeCADGui 目标
target_sources(FreeCADGui PRIVATE
    ${View3D_SRCS}
    ${View3D_Interface_SRCS}
    ${View3D_Coin_SRCS}
)

# 包含目录
target_include_directories(FreeCADGui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Interfaces
)

# 如果启用 OsgVerse，添加 OsgVerse 后端
if(BUILD_WITH_OSGVERSE)
    # 查找 OpenSceneGraph（如果还没有找到）
    if(NOT OPENSCENEGRAPH_FOUND)
        find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgUtil osgViewer osgGA)
    endif()
    
    set(View3D_OsgVerse_SRCS
        Backends/OsgVerse/PreCompiled.h
        Backends/OsgVerse/PreCompiled.cpp
        Backends/OsgVerse/OsgVerseViewerImpl.h
        Backends/OsgVerse/OsgVerseViewerImpl.cpp
        Backends/OsgVerse/GeometryConverter.h
        Backends/OsgVerse/GeometryConverter.cpp
    )
    
    target_sources(FreeCADGui PRIVATE
        ${View3D_OsgVerse_SRCS}
    )
    
    # 调试：打印 OSG 包含目录
    message(STATUS "View3D: OSG_INCLUDE_DIR = ${OSG_INCLUDE_DIR}")
    message(STATUS "View3D: OSGDB_INCLUDE_DIR = ${OSGDB_INCLUDE_DIR}")
    message(STATUS "View3D: OPENSCENEGRAPH_INCLUDE_DIRS = ${OPENSCENEGRAPH_INCLUDE_DIRS}")
    
    # OsgVerse 后端需要 OCCT 和 OSG 头文件
    # 使用各个 OSG 组件的 include 目录
    target_include_directories(FreeCADGui
        SYSTEM
        PRIVATE
        ${OCC_INCLUDE_DIR}
        ${OSG_INCLUDE_DIR}
        ${OSGDB_INCLUDE_DIR}
        ${OSGUTIL_INCLUDE_DIR}
        ${OSGVIEWER_INCLUDE_DIR}
        ${OSGGA_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/src  # 添加 src 目录以访问 Mod/Part/App
    )
    
    # 链接 OCCT 库（用于几何体转换）
    # 注意：不链接 Part 模块，因为它是 Python 扩展，不是独立库
    target_link_libraries(FreeCADGui PRIVATE
        ${OCC_LIBRARIES}
    )
    
    message(STATUS "View3D: OsgVerse backend enabled (Phase 2 - Real Geometry)")
    message(STATUS "View3D: OsgVerse using OCC include dir: ${OCC_INCLUDE_DIR}")
endif()

# 安装头文件（如果需要）
install(FILES
    IViewer3D.h
    ViewerFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Gui/View3D
)
