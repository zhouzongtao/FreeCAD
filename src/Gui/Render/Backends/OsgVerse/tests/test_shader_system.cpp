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
 * @file test_shader_system.cpp
 * @brief OsgVerse Shader系统测试
 *
 * 测试shader管理器和材质系统的集成
 */

#include <gtest/gtest.h>
#include "../OsgVerseShaderManager.h"
#include "../OsgVerseMaterial.h"
#include <osg/StateSet>

using namespace Gui::Render;

class ShaderSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前清除缓存
        OsgVerseShaderManager::instance().clearCache();
    }

    void TearDown() override {
        // 测试后清除缓存
        OsgVerseShaderManager::instance().clearCache();
    }
};

// 测试shader管理器单例
TEST_F(ShaderSystemTest, SingletonInstance)
{
    auto& mgr1 = OsgVerseShaderManager::instance();
    auto& mgr2 = OsgVerseShaderManager::instance();

    EXPECT_EQ(&mgr1, &mgr2) << "Shader manager should be singleton";
}

// 测试PBR shader创建
TEST_F(ShaderSystemTest, CreatePBRShader)
{
    auto& shaderMgr = OsgVerseShaderManager::instance();

    auto* program = shaderMgr.getProgram(ShaderType::PBR);

    ASSERT_NE(program, nullptr) << "PBR shader program should not be null";
    EXPECT_TRUE(shaderMgr.isCompiled(ShaderType::PBR)) << "PBR shader should be compiled";
}

// 测试标准Phong shader创建
TEST_F(ShaderSystemTest, CreateStandardShader)
{
    auto& shaderMgr = OsgVerseShaderManager::instance();

    auto* program = shaderMgr.getProgram(ShaderType::Standard);

    ASSERT_NE(program, nullptr) << "Standard shader program should not be null";
    EXPECT_TRUE(shaderMgr.isCompiled(ShaderType::Standard)) << "Standard shader should be compiled";
}

// 测试所有shader类型
TEST_F(ShaderSystemTest, CreateAllShaderTypes)
{
    auto& shaderMgr = OsgVerseShaderManager::instance();

    std::vector<ShaderType> types = {
        ShaderType::Standard,
        ShaderType::PBR,
        ShaderType::Wireframe,
        ShaderType::Flat,
        ShaderType::Unlit
    };

    for (auto type : types) {
        auto* program = shaderMgr.getProgram(type);
        ASSERT_NE(program, nullptr) << "Shader program should not be null for type " << static_cast<int>(type);
        EXPECT_TRUE(shaderMgr.isCompiled(type)) << "Shader should be compiled for type " << static_cast<int>(type);
    }
}

// 测试shader缓存
TEST_F(ShaderSystemTest, ShaderCaching)
{
    auto& shaderMgr = OsgVerseShaderManager::instance();

    // 第一次获取
    auto* program1 = shaderMgr.getProgram(ShaderType::PBR);
    ASSERT_NE(program1, nullptr);

    // 第二次获取应该返回相同的程序
    auto* program2 = shaderMgr.getProgram(ShaderType::PBR);
    EXPECT_EQ(program1, program2) << "Shader should be cached";
}

// 测试材质shader集成
TEST_F(ShaderSystemTest, MaterialShaderIntegration)
{
    auto material = std::make_shared<OsgVerseMaterial>();

    // 默认应该是PBR
    EXPECT_EQ(material->getShaderType(), ShaderType::PBR);

    // 切换到标准shader
    material->setShaderType(ShaderType::Standard);
    EXPECT_EQ(material->getShaderType(), ShaderType::Standard);

    // 检查StateSet是否有shader程序
    auto* stateSet = material->getStateSet();
    ASSERT_NE(stateSet, nullptr);

    auto* program = dynamic_cast<osg::Program*>(
        stateSet->getAttribute(osg::StateAttribute::PROGRAM)
    );
    EXPECT_NE(program, nullptr) << "StateSet should have shader program";
}

// 测试PBR材质参数
TEST_F(ShaderSystemTest, PBRMaterialParameters)
{
    auto material = std::make_shared<OsgVerseMaterial>();
    material->setShaderType(ShaderType::PBR);

    // 设置PBR参数
    Color baseColor{0.8f, 0.2f, 0.2f, 1.0f};
    material->setBaseColor(baseColor);
    material->setMetallic(0.5f);
    material->setRoughness(0.3f);

    // 验证参数
    EXPECT_EQ(material->getBaseColor(), baseColor);
    EXPECT_FLOAT_EQ(material->getMetallic(), 0.5f);
    EXPECT_FLOAT_EQ(material->getRoughness(), 0.3f);

    // 检查uniform是否设置
    auto* stateSet = material->getStateSet();
    ASSERT_NE(stateSet, nullptr);

    auto* uniform = stateSet->getUniform("baseColor");
    EXPECT_NE(uniform, nullptr) << "baseColor uniform should be set";
}

// 测试shader应用到StateSet
TEST_F(ShaderSystemTest, ApplyShaderToStateSet)
{
    auto& shaderMgr = OsgVerseShaderManager::instance();
    auto stateSet = new osg::StateSet();

    bool result = shaderMgr.applyShader(stateSet, ShaderType::PBR);

    EXPECT_TRUE(result) << "Shader should be applied successfully";

    auto* program = dynamic_cast<osg::Program*>(
        stateSet->getAttribute(osg::StateAttribute::PROGRAM)
    );
    EXPECT_NE(program, nullptr) << "StateSet should have shader program after apply";
}

// 测试自定义shader创建
TEST_F(ShaderSystemTest, CreateCustomShader)
{
    auto& shaderMgr = OsgVerseShaderManager::instance();

    const char* vertexSource = R"(
        #version 330 core
        in vec3 osg_Vertex;
        uniform mat4 osg_ModelViewProjectionMatrix;
        void main() {
            gl_Position = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";

    auto* program = shaderMgr.createProgram("TestShader", vertexSource, fragmentSource);

    ASSERT_NE(program, nullptr) << "Custom shader program should be created";
    EXPECT_EQ(program->getName(), "TestShader");
}

// 测试shader重载
TEST_F(ShaderSystemTest, ShaderReload)
{
    auto& shaderMgr = OsgVerseShaderManager::instance();

    // 创建shader
    auto* program1 = shaderMgr.getProgram(ShaderType::PBR);
    ASSERT_NE(program1, nullptr);

    // 重载所有shader
    shaderMgr.reloadAll();

    // 重新获取应该得到新的程序
    auto* program2 = shaderMgr.getProgram(ShaderType::PBR);
    ASSERT_NE(program2, nullptr);
    // 注意：重载后可能是新对象，也可能是相同对象（取决于实现）
}

// 测试PBR启用/禁用
TEST_F(ShaderSystemTest, PBREnableDisable)
{
    auto material = std::make_shared<OsgVerseMaterial>();

    // 默认PBR应该启用
    EXPECT_TRUE(material->isPBREnabled());
    EXPECT_EQ(material->getShaderType(), ShaderType::PBR);

    // 禁用PBR应该切换到标准shader
    material->setPBREnabled(false);
    EXPECT_FALSE(material->isPBREnabled());
    EXPECT_EQ(material->getShaderType(), ShaderType::Standard);

    // 重新启用PBR
    material->setPBREnabled(true);
    EXPECT_TRUE(material->isPBREnabled());
    EXPECT_EQ(material->getShaderType(), ShaderType::PBR);
}

// 主函数
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
