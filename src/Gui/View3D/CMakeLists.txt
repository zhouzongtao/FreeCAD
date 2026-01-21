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

# 安装头文件（如果需要）
install(FILES
    IViewer3D.h
    ViewerFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Gui/View3D
)
