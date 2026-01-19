# FreeCAD OsgVerse Backend CMake Configuration
# OsgVerse 后端 CMake 配置

# 检测 OpenSceneGraph / Detect OpenSceneGraph
find_package(OpenSceneGraph QUIET COMPONENTS osgDB osgUtil osgViewer osgGA)

# 检测 OsgVerse / Detect OsgVerse
find_path(OSGVERSE_INCLUDE_DIR
    NAMES osgverse/pipeline/Pipeline.h
    PATHS
        E:/Repository/OSGVerse/osgverse
        ${CMAKE_SOURCE_DIR}/../OSGVerse/osgverse
        $ENV{OSGVERSE_ROOT}
    DOC "OsgVerse include directory"
)

# 配置选项 / Configuration option
if(OPENSCENEGRAPH_FOUND AND OSGVERSE_INCLUDE_DIR)
    option(BUILD_WITH_OSGVERSE "Build with OsgVerse rendering backend" OFF)
    message(STATUS "OpenSceneGraph found: ${OPENSCENEGRAPH_VERSION}")
    message(STATUS "OsgVerse found at: ${OSGVERSE_INCLUDE_DIR}")
else()
    set(BUILD_WITH_OSGVERSE OFF CACHE BOOL "Build with OsgVerse rendering backend" FORCE)
    if(NOT OPENSCENEGRAPH_FOUND)
        message(STATUS "OpenSceneGraph not found - OsgVerse backend disabled")
    endif()
    if(NOT OSGVERSE_INCLUDE_DIR)
        message(STATUS "OsgVerse not found - OsgVerse backend disabled")
    endif()
endif()

# 如果启用 OsgVerse 后端 / If OsgVerse backend is enabled
if(BUILD_WITH_OSGVERSE)
    message(STATUS "Building with OsgVerse rendering backend")

    # 收集头文件 / Collect header files
    set(OSGVERSE_BACKEND_HEADERS
        OsgVerseNode.h
        OsgVerseEngine.h
        OsgVerseMaterial.h
        OsgVerseGeometry.h
        OsgVerseUtils.h
        OsgVerseViewer.h
    )

    # 收集源文件 / Collect source files
    set(OSGVERSE_BACKEND_SOURCES
        OsgVerseNode.cpp
        OsgVerseEngine.cpp
        OsgVerseMaterial.cpp
        OsgVerseGeometry.cpp
        OsgVerseUtils.cpp
        OsgVerseViewer.cpp
    )

    # 添加到 FreeCADGui 库 / Add to FreeCADGui library
    target_sources(FreeCADGui PRIVATE
        ${OSGVERSE_BACKEND_HEADERS}
        ${OSGVERSE_BACKEND_SOURCES}
    )

    # 添加包含目录 / Add include directories
    target_include_directories(FreeCADGui PRIVATE
        ${OSGVERSE_INCLUDE_DIR}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
    )

    # 链接库 / Link libraries
    target_link_libraries(FreeCADGui PRIVATE
        ${OPENSCENEGRAPH_LIBRARIES}
    )

    # 定义宏 / Define macros
    target_compile_definitions(FreeCADGui PRIVATE
        RENDER_HAS_OSGVERSE_BACKEND
    )

    # 创建源组 / Create source groups
    source_group("Render\\Backends\\OsgVerse" FILES
        ${OSGVERSE_BACKEND_HEADERS}
        ${OSGVERSE_BACKEND_SOURCES}
    )

    # 单元测试 / Unit tests
    if(BUILD_TESTS)
        add_subdirectory(tests)
    endif()

    message(STATUS "OsgVerse backend configuration complete")
else()
    message(STATUS "OsgVerse backend disabled")
endif()
