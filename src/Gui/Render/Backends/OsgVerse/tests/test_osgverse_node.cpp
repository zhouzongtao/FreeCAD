/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   Unit tests for OsgVerse backend node wrappers                        *
 ***************************************************************************/

#include <gtest/gtest.h>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Switch>

#include "Gui/Render/Backends/OsgVerse/OsgVerseNode.h"

using namespace Gui::Render;

//===========================================================================
// OsgVerseNode Tests
//===========================================================================

TEST(OsgVerseNodeTest, CreateNode) {
    auto* osgNode = new osg::Node();
    OsgVerseNode node(osgNode, true, RenderNode::NodeType::Unknown);

    EXPECT_NE(node.getOsgNode(), nullptr);
    EXPECT_EQ(node.getType(), RenderNode::NodeType::Unknown);
    EXPECT_TRUE(node.ownsNode());
}

TEST(OsgVerseNodeTest, SetGetName) {
    OsgVerseGroup group;

    group.setName("TestGroup");
    EXPECT_EQ(group.getName(), "TestGroup");
    EXPECT_EQ(group.getOsgNode()->getName(), "TestGroup");
}

TEST(OsgVerseNodeTest, Touch) {
    OsgVerseGroup group;

    // Touch should not crash
    EXPECT_NO_THROW(group.touch());
}

//===========================================================================
// OsgVerseGroup Tests
//===========================================================================

TEST(OsgVerseGroupTest, CreateGroup) {
    OsgVerseGroup group;

    EXPECT_NE(group.getOsgNode(), nullptr);
    EXPECT_NE(group.getOsgGroup(), nullptr);
    EXPECT_EQ(group.getType(), RenderNode::NodeType::Group);
    EXPECT_EQ(group.getNumChildren(), 0);
}

TEST(OsgVerseGroupTest, AddChild) {
    OsgVerseGroup parent;
    auto child = std::make_shared<OsgVerseGroup>();

    parent.addChild(child);

    EXPECT_EQ(parent.getNumChildren(), 1);
    EXPECT_EQ(parent.getChild(0), child.get());
    EXPECT_EQ(parent.getOsgGroup()->getNumChildren(), 1);
}

TEST(OsgVerseGroupTest, AddMultipleChildren) {
    OsgVerseGroup parent;
    auto child1 = std::make_shared<OsgVerseGroup>();
    auto child2 = std::make_shared<OsgVerseGroup>();
    auto child3 = std::make_shared<OsgVerseGroup>();

    parent.addChild(child1);
    parent.addChild(child2);
    parent.addChild(child3);

    EXPECT_EQ(parent.getNumChildren(), 3);
    EXPECT_EQ(parent.getChild(0), child1.get());
    EXPECT_EQ(parent.getChild(1), child2.get());
    EXPECT_EQ(parent.getChild(2), child3.get());
}

TEST(OsgVerseGroupTest, RemoveChild) {
    OsgVerseGroup parent;
    auto child1 = std::make_shared<OsgVerseGroup>();
    auto child2 = std::make_shared<OsgVerseGroup>();

    parent.addChild(child1);
    parent.addChild(child2);
    EXPECT_EQ(parent.getNumChildren(), 2);

    bool removed = parent.removeChild(child1.get());
    EXPECT_TRUE(removed);
    EXPECT_EQ(parent.getNumChildren(), 1);
    EXPECT_EQ(parent.getChild(0), child2.get());
}

TEST(OsgVerseGroupTest, RemoveAllChildren) {
    OsgVerseGroup parent;
    parent.addChild(std::make_shared<OsgVerseGroup>());
    parent.addChild(std::make_shared<OsgVerseGroup>());
    parent.addChild(std::make_shared<OsgVerseGroup>());

    EXPECT_EQ(parent.getNumChildren(), 3);

    parent.removeAllChildren();
    EXPECT_EQ(parent.getNumChildren(), 0);
    EXPECT_EQ(parent.getOsgGroup()->getNumChildren(), 0);
}

TEST(OsgVerseGroupTest, FindChild) {
    OsgVerseGroup parent;
    auto child1 = std::make_shared<OsgVerseGroup>();
    auto child2 = std::make_shared<OsgVerseGroup>();

    child1->setName("Child1");
    child2->setName("Child2");

    parent.addChild(child1);
    parent.addChild(child2);

    EXPECT_EQ(parent.findChild("Child1"), child1.get());
    EXPECT_EQ(parent.findChild("Child2"), child2.get());
    EXPECT_EQ(parent.findChild("NonExistent"), nullptr);
}

TEST(OsgVerseGroupTest, FindChildIndex) {
    OsgVerseGroup parent;
    auto child1 = std::make_shared<OsgVerseGroup>();
    auto child2 = std::make_shared<OsgVerseGroup>();

    parent.addChild(child1);
    parent.addChild(child2);

    EXPECT_EQ(parent.findChildIndex(child1.get()), 0);
    EXPECT_EQ(parent.findChildIndex(child2.get()), 1);
}

//===========================================================================
// OsgVerseSeparator Tests
//===========================================================================

TEST(OsgVerseSeparatorTest, CreateSeparator) {
    OsgVerseSeparator separator;

    EXPECT_NE(separator.getOsgNode(), nullptr);
    EXPECT_NE(separator.getOsgGroup(), nullptr);
    EXPECT_EQ(separator.getType(), RenderNode::NodeType::Separator);

    // Separator should have a StateSet for state isolation
    EXPECT_NE(separator.getOsgGroup()->getStateSet(), nullptr);
}

TEST(OsgVerseSeparatorTest, StateIsolation) {
    OsgVerseSeparator sep1;
    OsgVerseSeparator sep2;

    // Each separator should have its own StateSet
    EXPECT_NE(sep1.getOsgGroup()->getStateSet(),
              sep2.getOsgGroup()->getStateSet());
}

//===========================================================================
// OsgVerseTransform Tests
//===========================================================================

TEST(OsgVerseTransformTest, CreateTransform) {
    OsgVerseTransform transform;

    EXPECT_NE(transform.getOsgNode(), nullptr);
    EXPECT_NE(transform.getOsgTransform(), nullptr);
    EXPECT_EQ(transform.getType(), RenderNode::NodeType::Transform);
}

TEST(OsgVerseTransformTest, SetGetTranslation) {
    OsgVerseTransform transform;

    Vec3f translation(1.0f, 2.0f, 3.0f);
    transform.setTranslation(translation);

    Vec3f result = transform.getTranslation();
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST(OsgVerseTransformTest, SetGetScale) {
    OsgVerseTransform transform;

    Vec3f scale(2.0f, 3.0f, 4.0f);
    transform.setScale(scale);

    Vec3f result = transform.getScale();
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
}

TEST(OsgVerseTransformTest, Reset) {
    OsgVerseTransform transform;

    // Set some transformation
    transform.setTranslation(Vec3f(1, 2, 3));
    transform.setScale(Vec3f(2, 2, 2));

    // Reset to identity
    transform.reset();

    Vec3f translation = transform.getTranslation();
    Vec3f scale = transform.getScale();

    EXPECT_FLOAT_EQ(translation.x, 0.0f);
    EXPECT_FLOAT_EQ(translation.y, 0.0f);
    EXPECT_FLOAT_EQ(translation.z, 0.0f);
    EXPECT_FLOAT_EQ(scale.x, 1.0f);
    EXPECT_FLOAT_EQ(scale.y, 1.0f);
    EXPECT_FLOAT_EQ(scale.z, 1.0f);
}

//===========================================================================
// OsgVerseSwitch Tests
//===========================================================================

TEST(OsgVerseSwitchTest, CreateSwitch) {
    OsgVerseSwitch switchNode;

    EXPECT_NE(switchNode.getOsgNode(), nullptr);
    EXPECT_NE(switchNode.getOsgSwitch(), nullptr);
    EXPECT_EQ(switchNode.getType(), RenderNode::NodeType::Switch);
}

TEST(OsgVerseSwitchTest, SetWhichChild) {
    OsgVerseSwitch switchNode;

    // Add some children
    switchNode.addChild(std::make_shared<OsgVerseGroup>());
    switchNode.addChild(std::make_shared<OsgVerseGroup>());
    switchNode.addChild(std::make_shared<OsgVerseGroup>());

    // Show only first child
    switchNode.setWhichChild(0);
    EXPECT_TRUE(switchNode.getValue(0));
    EXPECT_FALSE(switchNode.getValue(1));
    EXPECT_FALSE(switchNode.getValue(2));

    // Show only second child
    switchNode.setWhichChild(1);
    EXPECT_FALSE(switchNode.getValue(0));
    EXPECT_TRUE(switchNode.getValue(1));
    EXPECT_FALSE(switchNode.getValue(2));
}

TEST(OsgVerseSwitchTest, AllChildrenOnOff) {
    OsgVerseSwitch switchNode;

    switchNode.addChild(std::make_shared<OsgVerseGroup>());
    switchNode.addChild(std::make_shared<OsgVerseGroup>());

    // Turn all on
    switchNode.setAllChildrenOn();
    EXPECT_TRUE(switchNode.getValue(0));
    EXPECT_TRUE(switchNode.getValue(1));

    // Turn all off
    switchNode.setAllChildrenOff();
    EXPECT_FALSE(switchNode.getValue(0));
    EXPECT_FALSE(switchNode.getValue(1));
}

TEST(OsgVerseSwitchTest, SetValue) {
    OsgVerseSwitch switchNode;

    switchNode.addChild(std::make_shared<OsgVerseGroup>());
    switchNode.addChild(std::make_shared<OsgVerseGroup>());

    // Set individual values
    switchNode.setValue(0, true);
    switchNode.setValue(1, false);

    EXPECT_TRUE(switchNode.getValue(0));
    EXPECT_FALSE(switchNode.getValue(1));
}

//===========================================================================
// Integration Tests
//===========================================================================

TEST(OsgVerseIntegrationTest, ComplexSceneGraph) {
    // Create a complex scene graph
    auto root = std::make_shared<OsgVerseSeparator>();
    auto transform = std::make_shared<OsgVerseTransform>();
    auto switchNode = std::make_shared<OsgVerseSwitch>();
    auto group1 = std::make_shared<OsgVerseGroup>();
    auto group2 = std::make_shared<OsgVerseGroup>();

    // Build hierarchy
    root->addChild(transform);
    transform->setTranslation(Vec3f(1, 0, 0));

    // Note: OsgVerseTransform doesn't inherit from OsgVerseGroup
    // so we need to add children differently
    auto transformGroup = std::make_shared<OsgVerseGroup>();
    root->addChild(transformGroup);
    transformGroup->addChild(switchNode);

    switchNode->addChild(group1);
    switchNode->addChild(group2);

    // Verify structure
    EXPECT_EQ(root->getNumChildren(), 2);
    EXPECT_EQ(transformGroup->getNumChildren(), 1);
    EXPECT_EQ(switchNode->getNumChildren(), 2);

    // Test switch functionality
    switchNode->setWhichChild(0);
    EXPECT_TRUE(switchNode->getValue(0));
    EXPECT_FALSE(switchNode->getValue(1));
}

//===========================================================================
// Main
//===========================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
