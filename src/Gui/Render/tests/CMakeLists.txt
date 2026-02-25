# FreeCAD Rendering Abstraction Layer Unit Tests
# 渲染抽象层单元测试

# 查找 Google Test / Find Google Test
find_package(GTest QUIET)

if(BUILD_TESTS AND GTEST_FOUND)
    # 渲染工厂测试 / Render factory tests
    add_executable(TestRenderFactory
        test_factory.cpp
        ../Core/RenderNode.cpp
        ../Core/RenderEngine.cpp
    )

    target_include_directories(TestRenderFactory PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${GTEST_INCLUDE_DIRS}
    )

    target_link_libraries(TestRenderFactory PRIVATE
        ${GTEST_BOTH_LIBRARIES}
        pthread
        FreeCADBase
    )

    # 添加测试 / Add test
    add_test(NAME RenderFactory TestRenderFactory)

    # 设置测试属性 / Set test properties
    set_tests_properties(RenderFactory PROPERTIES
        TIMEOUT 300
        LABELS "Render;Unit"
    )
else()
    message(STATUS "Render tests skipped: GTest not found or BUILD_TESTS=OFF")
endif()
