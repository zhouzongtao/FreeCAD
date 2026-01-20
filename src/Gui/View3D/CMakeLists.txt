# View3D abstraction layer

set(View3D_SRCS
    PreCompiled.h
    PreCompiled.cpp
    IViewer3D.h
    ViewerFactory.h
    ViewerFactory.cpp
)

set(View3D_Coin_SRCS
    Backends/Coin/CoinViewer.h
    Backends/Coin/CoinViewer.cpp
)

# 添加到 FreeCADGui 目标
target_sources(FreeCADGui PRIVATE
    ${View3D_SRCS}
    ${View3D_Coin_SRCS}
)

# 包含目录
target_include_directories(FreeCADGui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 如果启用 OsgVerse，添加 OsgVerse 后端
if(BUILD_WITH_OSGVERSE)
    set(View3D_OsgVerse_SRCS
        Backends/OsgVerse/PreCompiled.h
        Backends/OsgVerse/PreCompiled.cpp
        Backends/OsgVerse/OsgVerseViewerImpl.h
        Backends/OsgVerse/OsgVerseViewerImpl.cpp
    )
    
    target_sources(FreeCADGui PRIVATE
        ${View3D_OsgVerse_SRCS}
    )
    
    message(STATUS "View3D: OsgVerse backend enabled (Phase 1 - Placeholder)")
endif()

# 安装头文件（如果需要）
install(FILES
    IViewer3D.h
    ViewerFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Gui/View3D
)
