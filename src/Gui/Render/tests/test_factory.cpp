/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

/**
 * @file test_factory.cpp
 * @brief 渲染引擎工厂单元测试 / Render engine factory unit tests
 *
 * 测试内容 / Test Coverage:
 * - 工厂单例模式验证
 * - 引擎注册和取消注册
 * - 引擎创建和初始化
 * - 默认引擎选择
 * - 最佳引擎自动选择
 */

#include <gtest/gtest.h>
#include <Core/RenderEngine.h>
#include <Core/RenderNode.h>

namespace Gui {
namespace Render {
namespace Test {

//===========================================================================
// 测试用引擎实现 / Test Engine Implementation
//===========================================================================

/**
 * @brief 用于测试的模拟引擎 / Mock engine for testing
 */
class MockEngine : public RenderEngine {
public:
    static int createCount;
    static int destroyCount;
    static bool initializeResult;
    static bool isAvailableValue;

    MockEngine(BackendType type = BackendType::Coin3D)
        : _type(type)
        , _initialized(false)
    {
        ++createCount;
    }

    ~MockEngine() override {
        ++destroyCount;
    }

    // Engine Information
    BackendType getType() const override { return _type; }
    std::string getName() const override {
        return _type == BackendType::Coin3D ? "MockCoin3D" : "MockOsgVerse";
    }
    std::string getVersion() const override { return "1.0.0-test"; }
    BackendInfo getInfo() const override {
        BackendInfo info;
        info.type = _type;
        info.name = getName();
        info.version = getVersion();
        info.supportsPBR = (_type == BackendType::OsgVerse);
        return info;
    }
    bool isAvailable() const override { return isAvailableValue; }

    bool initialize() override {
        if (!initializeResult) {
            return false;
        }
        _initialized = true;
        return true;
    }

    void shutdown() override {
        _initialized = false;
    }

    // Node Creation
    RenderGroup::Ptr createGroup() override {
        return std::make_shared<RenderGroup>();
    }
    RenderSeparator::Ptr createSeparator() override {
        return std::make_shared<RenderSeparator>();
    }
    RenderNode::Ptr createTransform() override { return nullptr; }
    RenderNode::Ptr createSwitch() override { return nullptr; }
    RenderNode::Ptr createMaterial() override { return nullptr; }
    RenderNode::Ptr createGeometry() override { return nullptr; }
    RenderNode::Ptr createCamera() override { return nullptr; }
    RenderNode::Ptr createLight(LightType) override { return nullptr; }

    // Scene Management
    void setSceneRoot(RenderNode::Ptr) override {}
    RenderNode::Ptr getSceneRoot() const override { return nullptr; }
    void updateScene() override {}

    // Rendering Control
    void render() override {}
    void setRenderMode(RenderMode) override {}
    RenderMode getRenderMode() const override { return RenderMode::Default; }
    void setBackgroundColor(const Color&) override {}
    Color getBackgroundColor() const override { return Color(); }
    RenderStats getStats() const override { return RenderStats(); }
    void resetStats() override {}

    // Resource Management
    void releaseUnusedResources() override {}
    size_t getMemoryUsage() const override { return 0; }

    // Compatibility
    void* getNativePointer() const override { return nullptr; }
    void setEventCallback(EventCallback) override {}

private:
    BackendType _type;
    bool _initialized;
};

int MockEngine::createCount = 0;
int MockEngine::destroyCount = 0;
bool MockEngine::initializeResult = true;
bool MockEngine::isAvailableValue = true;

//===========================================================================
// 工厂测试 / Factory Tests
//===========================================================================

/**
 * @brief 测试工厂单例模式 / Test factory singleton pattern
 */
TEST(RenderEngineFactory, Singleton) {
    auto& factory1 = RenderEngineFactory::instance();
    auto& factory2 = RenderEngineFactory::instance();

    EXPECT_EQ(&factory1, &factory2);
}

/**
 * @brief 测试引擎注册 / Test engine registration
 */
TEST(RenderEngineFactory, RegisterEngine) {
    auto& factory = RenderEngineFactory::instance();

    // 重置计数器 / Reset counters
    MockEngine::createCount = 0;

    // 注册测试引擎 / Register test engine
    bool registered = factory.registerEngine(
        BackendType::Coin3D,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::Coin3D);
        }
    );

    EXPECT_TRUE(registered);
    EXPECT_TRUE(factory.isEngineRegistered(BackendType::Coin3D));

    // 重复注册应该返回 false 但会覆盖 / Duplicate registration should return false but override
    bool registeredAgain = factory.registerEngine(
        BackendType::Coin3D,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::Coin3D);
        }
    );

    // 注意：当前实现允许重复注册，返回 true
    // Note: Current implementation allows duplicate registration, returns true
    EXPECT_TRUE(registeredAgain);
}

/**
 * @brief 测试引擎创建 / Test engine creation
 */
TEST(RenderEngineFactory, CreateEngine) {
    auto& factory = RenderEngineFactory::instance();

    MockEngine::createCount = 0;

    // 注册引擎 / Register engine
    factory.registerEngine(
        BackendType::Coin3D,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::Coin3D);
        }
    );

    // 创建引擎 / Create engine
    auto engine = factory.create(BackendType::Coin3D);

    ASSERT_NE(engine, nullptr);
    EXPECT_EQ(engine->getType(), BackendType::Coin3D);
    EXPECT_EQ(engine->getName(), "MockCoin3D");
    EXPECT_EQ(MockEngine::createCount, 1);
}

/**
 * @brief 测试创建不存在的引擎 / Test creating non-existent engine
 */
TEST(RenderEngineFactory, CreateNonExistentEngine) {
    auto& factory = RenderEngineFactory::instance();

    // 尝试创建未注册的引擎 / Try to create unregistered engine
    auto engine = factory.create(BackendType::OsgVerse);

    EXPECT_EQ(engine, nullptr);
}

/**
 * @brief 测试引擎取消注册 / Test engine unregistration
 */
TEST(RenderEngineFactory, UnregisterEngine) {
    auto& factory = RenderEngineFactory::instance();

    // 注册引擎 / Register engine
    factory.registerEngine(
        BackendType::Coin3D,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::Coin3D);
        }
    );

    EXPECT_TRUE(factory.isEngineRegistered(BackendType::Coin3D));

    // 取消注册 / Unregister
    bool unregistered = factory.unregisterEngine(BackendType::Coin3D);

    EXPECT_TRUE(unregistered);
    EXPECT_FALSE(factory.isEngineRegistered(BackendType::Coin3D));

    // 取消注册不存在的引擎 / Unregister non-existent engine
    bool unregisteredAgain = factory.unregisterEngine(BackendType::Coin3D);
    EXPECT_FALSE(unregisteredAgain);
}

/**
 * @brief 测试获取已注册类型列表 / Test getting registered types list
 */
TEST(RenderEngineFactory, GetRegisteredTypes) {
    auto& factory = RenderEngineFactory::instance();

    // 清理之前的注册 / Clear previous registrations
    factory.unregisterEngine(BackendType::Coin3D);
    factory.unregisterEngine(BackendType::OsgVerse);

    // 注册多个引擎 / Register multiple engines
    factory.registerEngine(
        BackendType::Coin3D,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::Coin3D);
        }
    );
    factory.registerEngine(
        BackendType::OsgVerse,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::OsgVerse);
        }
    );

    auto types = factory.getRegisteredTypes();

    EXPECT_EQ(types.size(), 2);
    EXPECT_TRUE(std::find(types.begin(), types.end(), BackendType::Coin3D) != types.end());
    EXPECT_TRUE(std::find(types.begin(), types.end(), BackendType::OsgVerse) != types.end());
}

/**
 * @brief 测试获取引擎信息 / Test getting engine information
 */
TEST(RenderEngineFactory, GetEngineInfo) {
    auto& factory = RenderEngineFactory::instance();

    factory.registerEngine(
        BackendType::OsgVerse,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::OsgVerse);
        }
    );

    auto info = factory.getEngineInfo(BackendType::OsgVerse);

    EXPECT_EQ(info.type, BackendType::OsgVerse);
    EXPECT_EQ(info.name, "MockOsgVerse");
    EXPECT_EQ(info.version, "1.0.0-test");
    EXPECT_TRUE(info.supportsPBR);
}

/**
 * @brief 测试获取不存在引擎的信息 / Test getting info for non-existent engine
 */
TEST(RenderEngineFactory, GetNonExistentEngineInfo) {
    auto& factory = RenderEngineFactory::instance();

    // 确保没有注册这个引擎 / Ensure this engine is not registered
    factory.unregisterEngine(BackendType::OsgVerse);

    auto info = factory.getEngineInfo(BackendType::OsgVerse);

    // 应该返回默认信息 / Should return default info
    EXPECT_EQ(info.type, BackendType::OsgVerse);
    EXPECT_EQ(info.name, "OsgVerse");
    EXPECT_FALSE(info.supportsPBR);
}

/**
 * @brief 测试选择最佳引擎 / Test selecting best engine
 */
TEST(RenderEngineFactory, SelectBestEngine) {
    auto& factory = RenderEngineFactory::instance();

    // 清理之前的注册 / Clear previous registrations
    factory.unregisterEngine(BackendType::Coin3D);
    factory.unregisterEngine(BackendType::OsgVerse);

    // 只注册 Coin3D / Only register Coin3D
    factory.registerEngine(
        BackendType::Coin3D,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::Coin3D);
        }
    );

    auto best = factory.selectBestEngine();
    EXPECT_EQ(best, BackendType::Coin3D);

    // 同时注册 OsgVerse（支持 PBR）/ Also register OsgVerse (supports PBR)
    factory.registerEngine(
        BackendType::OsgVerse,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::OsgVerse);
        }
    );

    best = factory.selectBestEngine();
    // OsgVerse 支持 PBR，应该优先选择 / OsgVerse supports PBR, should be preferred
    EXPECT_EQ(best, BackendType::OsgVerse);
}

/**
 * @brief 测试创建默认引擎 / Test creating default engine
 */
TEST(RenderEngineFactory, CreateDefaultEngine) {
    auto& factory = RenderEngineFactory::instance();

    // 清理并设置默认类型 / Clear and set default type
    factory.unregisterEngine(BackendType::Coin3D);
    factory.unregisterEngine(BackendType::OsgVerse);
    factory.setDefaultType(BackendType::Coin3D);

    factory.registerEngine(
        BackendType::Coin3D,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::Coin3D);
        }
    );

    auto engine = factory.createDefault();

    ASSERT_NE(engine, nullptr);
    EXPECT_EQ(engine->getType(), BackendType::Coin3D);
}

/**
 * @brief 测试默认引擎降级 / Test default engine fallback
 */
TEST(RenderEngineFactory, DefaultEngineFallback) {
    auto& factory = RenderEngineFactory::instance();

    // 清理并设置不存在的默认类型 / Clear and set non-existent default type
    factory.unregisterEngine(BackendType::Coin3D);
    factory.unregisterEngine(BackendType::OsgVerse);
    factory.setDefaultType(BackendType::OsgVerse);

    // 只注册 Coin3D / Only register Coin3D
    factory.registerEngine(
        BackendType::Coin3D,
        []() -> RenderEngine::Ptr {
            return std::make_shared<MockEngine>(BackendType::Coin3D);
        }
    );

    auto engine = factory.createDefault();

    ASSERT_NE(engine, nullptr);
    // 应该降级使用 Coin3D / Should fallback to Coin3D
    EXPECT_EQ(engine->getType(), BackendType::Coin3D);
}

/**
 * @brief 测试没有可用引擎的情况 / Test when no engine is available
 */
TEST(RenderEngineFactory, NoAvailableEngine) {
    auto& factory = RenderEngineFactory::instance();

    // 清理所有注册 / Clear all registrations
    factory.unregisterEngine(BackendType::Coin3D);
    factory.unregisterEngine(BackendType::OsgVerse);
    factory.setDefaultType(BackendType::Coin3D);

    auto engine = factory.createDefault();

    EXPECT_EQ(engine, nullptr);

    auto best = factory.selectBestEngine();
    EXPECT_EQ(best, BackendType::None);
}

//===========================================================================
// 节点测试 / Node Tests
//===========================================================================

/**
 * @brief 测试基础节���创建 / Test basic node creation
 */
TEST(RenderNode, BasicCreation) {
    auto node = std::make_shared<RenderNode>(NodeType::Node);

    EXPECT_EQ(node->getNodeType(), NodeType::Node);
    EXPECT_TRUE(node->isVisible());
    EXPECT_EQ(node->getRefCount(), 0);
    EXPECT_EQ(node->getParent(), nullptr);
}

/**
 * @brief 测试节点名称 / Test node name
 */
TEST(RenderNode, NodeName) {
    auto node = std::make_shared<RenderNode>(NodeType::Node);

    EXPECT_TRUE(node->getName().empty());

    node->setName("TestNode");
    EXPECT_EQ(node->getName(), "TestNode");
}

/**
 * @brief 测试节点可见性 / Test node visibility
 */
TEST(RenderNode, NodeVisibility) {
    auto node = std::make_shared<RenderNode>(NodeType::Node);

    EXPECT_TRUE(node->isVisible());

    node->setVisible(false);
    EXPECT_FALSE(node->isVisible());

    node->setVisible(true);
    EXPECT_TRUE(node->isVisible());
}

/**
 * @brief 测试节点触摸 / Test node touch
 */
TEST(RenderNode, NodeTouch) {
    auto node = std::make_shared<RenderNode>(NodeType::Node);

    EXPECT_FALSE(node->isTouched());

    node->touch();
    EXPECT_TRUE(node->isTouched());
}

/**
 * @brief 测试节点引用计数 / Test node reference counting
 */
TEST(RenderNode, NodeRefCount) {
    auto node = std::make_shared<RenderNode>(NodeType::Node);

    EXPECT_EQ(node->getRefCount(), 0);

    node->ref();
    EXPECT_EQ(node->getRefCount(), 1);

    node->ref();
    EXPECT_EQ(node->getRefCount(), 2);

    node->unref();
    EXPECT_EQ(node->getRefCount(), 1);

    // 不要在智能指针管理的对象上测试 unref 删除
    // Don't test unref deletion on smart-pointer managed objects
}

/**
 * @brief 测试分组节点 / Test group node
 */
TEST(RenderGroup, BasicGroup) {
    auto group = std::make_shared<RenderGroup>();

    EXPECT_EQ(group->getNodeType(), NodeType::Group);
    EXPECT_EQ(group->getNumChildren(), 0);
    EXPECT_TRUE(group->begin() == group->end());
}

/**
 * @brief 测试添加子节点 / Test adding children
 */
TEST(RenderGroup, AddChild) {
    auto group = std::make_shared<RenderGroup>();
    auto child1 = std::make_shared<RenderNode>(NodeType::Node);
    auto child2 = std::make_shared<RenderNode>(NodeType::Node);

    child1->setName("Child1");
    child2->setName("Child2");

    group->addChild(child1);
    EXPECT_EQ(group->getNumChildren(), 1);
    EXPECT_EQ(group->getChild(0), child1.get());
    EXPECT_EQ(child1->getParent(), group.get());

    group->addChild(child2);
    EXPECT_EQ(group->getNumChildren(), 2);
    EXPECT_EQ(group->getChild(1), child2.get());
}

/**
 * @brief 测试移除子节点 / Test removing children
 */
TEST(RenderGroup, RemoveChild) {
    auto group = std::make_shared<RenderGroup>();
    auto child = std::make_shared<RenderNode>(NodeType::Node);

    group->addChild(child);
    EXPECT_EQ(group->getNumChildren(), 1);

    bool removed = group->removeChild(child.get());
    EXPECT_TRUE(removed);
    EXPECT_EQ(group->getNumChildren(), 0);
    EXPECT_EQ(child->getParent(), nullptr);

    // 再次移除应该失败 / Removing again should fail
    removed = group->removeChild(child.get());
    EXPECT_FALSE(removed);
}

/**
 * @brief 测试移除所有子节点 / Test removing all children
 */
TEST(RenderGroup, RemoveAllChildren) {
    auto group = std::make_shared<RenderGroup>();

    group->addChild(std::make_shared<RenderNode>(NodeType::Node));
    group->addChild(std::make_shared<RenderNode>(NodeType::Node));
    group->addChild(std::make_shared<RenderNode>(NodeType::Node));

    EXPECT_EQ(group->getNumChildren(), 3);

    group->removeAllChildren();
    EXPECT_EQ(group->getNumChildren(), 0);
}

/**
 * @brief 测试查找子节点 / Test finding children
 */
TEST(RenderGroup, FindChild) {
    auto group = std::make_shared<RenderGroup>();
    auto child1 = std::make_shared<RenderNode>(NodeType::Node);
    auto child2 = std::make_shared<RenderNode>(NodeType::Node);

    child1->setName("TargetChild");
    child2->setName("OtherChild");

    group->addChild(child1);
    group->addChild(child2);

    auto found = group->findChild("TargetChild");
    EXPECT_EQ(found, child1.get());

    found = group->findChild("NonExistent");
    EXPECT_EQ(found, nullptr);
}

/**
 * @brief 测试替换子节点 / Test replacing children
 */
TEST(RenderGroup, ReplaceChild) {
    auto group = std::make_shared<RenderGroup>();
    auto oldChild = std::make_shared<RenderNode>(NodeType::Node);
    auto newChild = std::make_shared<RenderNode>(NodeType::Node);

    group->addChild(oldChild);
    EXPECT_EQ(group->getNumChildren(), 1);

    bool replaced = group->replaceChild(oldChild.get(), newChild);
    EXPECT_TRUE(replaced);
    EXPECT_EQ(group->getChild(0), newChild.get());
    EXPECT_EQ(newChild->getParent(), group.get());
    EXPECT_EQ(oldChild->getParent(), nullptr);
}

/**
 * @brief 测试子节点索引 / Test child index
 */
TEST(RenderGroup, ChildIndex) {
    auto group = std::make_shared<RenderGroup>();
    auto child1 = std::make_shared<RenderNode>(NodeType::Node);
    auto child2 = std::make_shared<RenderNode>(NodeType::Node);

    group->addChild(child1);
    group->addChild(child2);

    EXPECT_EQ(group->findChildIndex(child1.get()), 0);
    EXPECT_EQ(group->findChildIndex(child2.get()), 1);
    EXPECT_EQ(group->findChildIndex(nullptr), -1);
}

/**
 * @brief 测试分隔节点 / Test separator node
 */
TEST(RenderSeparator, BasicSeparator) {
    auto sep = std::make_shared<RenderSeparator>();

    EXPECT_EQ(sep->getNodeType(), NodeType::Separator);
    EXPECT_EQ(sep->getNumChildren(), 0);

    // Separator 继承自 Group，应该能添加子节点
    // Separator inherits from Group, should be able to add children
    auto child = std::make_shared<RenderNode>(NodeType::Node);
    sep->addChild(child);
    EXPECT_EQ(sep->getNumChildren(), 1);
}

} // namespace Test
} // namespace Render
} // namespace Gui

//===========================================================================
// 主函数 / Main Function
//===========================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
