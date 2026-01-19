# OsgVerse Backend Unit Tests CMake Configuration
# OsgVerse 后端单元测试 CMake 配置

if(NOT BUILD_WITH_OSGVERSE)
    message(STATUS "OsgVerse backend disabled - skipping tests")
    return()
endif()

# 查找 GTest / Find GTest
find_package(GTest QUIET)

if(NOT GTEST_FOUND)
    message(STATUS "GTest not found - OsgVerse backend tests disabled")
    return()
endif()

message(STATUS "Building OsgVerse backend unit tests")

# 测试可执行文件 / Test executable
add_executable(OsgVerseBackend_tests
    test_osgverse_node.cpp
)

# 链接库 / Link libraries
target_link_libraries(OsgVerseBackend_tests PRIVATE
    FreeCADGui
    ${GTEST_LIBRARIES}
    ${OPENSCENEGRAPH_LIBRARIES}
)

# 包含目录 / Include directories
target_include_directories(OsgVerseBackend_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIRS}
    ${OSGVERSE_INCLUDE_DIR}
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

# 添加测试 / Add test
add_test(NAME OsgVerseBackend_tests COMMAND OsgVerseBackend_tests)

message(STATUS "OsgVerse backend tests configured")
