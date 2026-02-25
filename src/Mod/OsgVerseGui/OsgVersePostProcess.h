// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_POSTPROCESS_H
#define OSGVERSEGUI_POSTPROCESS_H

#include "PreCompiled.h"

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Camera>
#include <osg/Texture2D>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include <osg/Geometry>
#include <osg/Geode>

#include <memory>
#include <string>

namespace OsgVerseGui {

/**
 * @brief 后处理效果基类
 *
 * 所有后处理效果的抽象基类，提供统一的接口。
 */
class OsgVerseGuiExport PostProcessEffect {
public:
    PostProcessEffect(const std::string& name);
    virtual ~PostProcessEffect();

    /**
     * @brief 获取效果名称
     */
    const std::string& getName() const { return _name; }

    /**
     * @brief 启用/禁用效果
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    /**
     * @brief 设置效果强度 (0.0 - 1.0)
     */
    virtual void setIntensity(float intensity);
    float getIntensity() const { return _intensity; }

    /**
     * @brief 初始化效果 (设置 RTT 相机和着色器)
     *
     * @param width 视口宽度
     * @param height 视口高度
     * @param inputTexture 输入纹理 (前一个 pass 的输出)
     * @return 输出纹理
     */
    virtual osg::ref_ptr<osg::Texture2D> setup(int width, int height,
                                                osg::Texture2D* inputTexture) = 0;

    /**
     * @brief 调整大小
     */
    virtual void resize(int width, int height) = 0;

    /**
     * @brief 获取渲染相机
     */
    osg::Camera* getCamera() const { return _camera.get(); }

    /**
     * @brief 获取输出纹理
     */
    osg::Texture2D* getOutputTexture() const { return _outputTexture.get(); }

protected:
    /**
     * @brief 创建全屏四边形 (用于后处理渲染)
     */
    osg::ref_ptr<osg::Geode> createScreenQuad();

    /**
     * @brief 创建 RTT 相机
     */
    osg::ref_ptr<osg::Camera> createRTTCamera(int width, int height,
                                               osg::Texture2D* outputTexture);

    /**
     * @brief 创建着色器程序
     */
    osg::ref_ptr<osg::Program> createProgram(const std::string& vertSource,
                                              const std::string& fragSource);

    /**
     * @brief 获取简单的全屏四边形顶点着色器
     */
    static std::string getSimpleVertexShader();

    std::string _name;
    bool _enabled{true};
    float _intensity{1.0f};

    osg::ref_ptr<osg::Camera> _camera;
    osg::ref_ptr<osg::Texture2D> _outputTexture;
    osg::ref_ptr<osg::Program> _program;

    int _width{0};
    int _height{0};
};

/**
 * @brief SSAO (屏幕空间环境光遮蔽) 效果
 *
 * 实现基于屏幕空间的环境光遮蔽，增强场景的深度感和真实感。
 *
 * 算法：
 * 1. 从深度缓冲重建位置
 * 2. 在半球内采样
 * 3. 计算遮蔽因子
 * 4. 可选的模糊 pass
 */
class OsgVerseGuiExport SSAOEffect : public PostProcessEffect {
public:
    SSAOEffect();
    ~SSAOEffect() override;

    // PostProcessEffect 接口
    osg::ref_ptr<osg::Texture2D> setup(int width, int height,
                                        osg::Texture2D* inputTexture) override;
    void resize(int width, int height) override;

    //=========================================================================
    // SSAO 特定参数
    //=========================================================================

    /**
     * @brief 设置采样半径
     */
    void setRadius(float radius);
    float getRadius() const { return _radius; }

    /**
     * @brief 设置采样数量 (影响质量和性能)
     */
    void setSampleCount(int count);
    int getSampleCount() const { return _sampleCount; }

    /**
     * @brief 设置偏移 (避免自遮蔽)
     */
    void setBias(float bias);
    float getBias() const { return _bias; }

    /**
     * @brief 启用/禁用模糊
     */
    void setBlurEnabled(bool enabled);
    bool isBlurEnabled() const { return _blurEnabled; }

    /**
     * @brief 设置深度纹理 (必须在 setup 之前调用)
     */
    void setDepthTexture(osg::Texture2D* depthTexture);

    /**
     * @brief 设置法线纹理 (可选，提高质量)
     */
    void setNormalTexture(osg::Texture2D* normalTexture);

private:
    void createSSAOPass(int width, int height, osg::Texture2D* inputTexture);
    void createBlurPass(int width, int height);
    void generateSampleKernel();
    void generateNoiseTexture();
    std::string getSSAOFragmentShader();
    std::string getBlurFragmentShader();

    float _radius{0.5f};
    int _sampleCount{16};
    float _bias{0.025f};
    bool _blurEnabled{true};

    osg::ref_ptr<osg::Texture2D> _depthTexture;
    osg::ref_ptr<osg::Texture2D> _normalTexture;
    osg::ref_ptr<osg::Texture2D> _noiseTexture;
    osg::ref_ptr<osg::Texture2D> _ssaoTexture;

    osg::ref_ptr<osg::Camera> _blurCamera;
    osg::ref_ptr<osg::Program> _blurProgram;

    osg::ref_ptr<osg::Uniform> _radiusUniform;
    osg::ref_ptr<osg::Uniform> _biasUniform;
    osg::ref_ptr<osg::Uniform> _intensityUniform;

    std::vector<osg::Vec3> _sampleKernel;
};

/**
 * @brief Bloom (泛光) 效果
 *
 * 实现 HDR 泛光效果，使明亮区域产生光晕。
 *
 * 算法：
 * 1. 亮度提取 (阈值过滤)
 * 2. 多级下采样 + 高斯模糊
 * 3. 多级上采样 + 混合
 * 4. 与原图合成
 */
class OsgVerseGuiExport BloomEffect : public PostProcessEffect {
public:
    BloomEffect();
    ~BloomEffect() override;

    // PostProcessEffect 接口
    osg::ref_ptr<osg::Texture2D> setup(int width, int height,
                                        osg::Texture2D* inputTexture) override;
    void resize(int width, int height) override;

    //=========================================================================
    // Bloom 特定参数
    //=========================================================================

    /**
     * @brief 设置亮度阈值
     */
    void setThreshold(float threshold);
    float getThreshold() const { return _threshold; }

    /**
     * @brief 设置模糊强度
     */
    void setBlurStrength(float strength);
    float getBlurStrength() const { return _blurStrength; }

    /**
     * @brief 设置下采样级数 (影响泛光范围)
     */
    void setMipLevels(int levels);
    int getMipLevels() const { return _mipLevels; }

private:
    void createBrightPassCamera(int width, int height, osg::Texture2D* inputTexture);
    void createBlurPassCameras(int width, int height);
    void createCompositeCamera(int width, int height, osg::Texture2D* inputTexture);

    std::string getBrightPassFragmentShader();
    std::string getBlurFragmentShader();
    std::string getCompositeFragmentShader();

    float _threshold{1.0f};
    float _blurStrength{1.0f};
    int _mipLevels{5};

    osg::ref_ptr<osg::Camera> _brightPassCamera;
    osg::ref_ptr<osg::Texture2D> _brightTexture;

    std::vector<osg::ref_ptr<osg::Camera>> _blurCameras;
    std::vector<osg::ref_ptr<osg::Texture2D>> _blurTextures;

    osg::ref_ptr<osg::Camera> _compositeCamera;

    osg::ref_ptr<osg::Uniform> _thresholdUniform;
    osg::ref_ptr<osg::Uniform> _bloomIntensityUniform;
};

/**
 * @brief 后处理管理器
 *
 * 管理所有后处理效果，构建渲染管线。
 */
class OsgVerseGuiExport PostProcessManager {
public:
    PostProcessManager();
    ~PostProcessManager();

    /**
     * @brief 初始化后处理系统
     *
     * @param sceneCamera 场景渲染相机
     * @param width 视口宽度
     * @param height 视口高度
     */
    void initialize(osg::Camera* sceneCamera, int width, int height);

    /**
     * @brief 调整大小
     */
    void resize(int width, int height);

    /**
     * @brief 获取后处理根节点
     */
    osg::Group* getPostProcessRoot() const { return _postProcessRoot.get(); }

    //=========================================================================
    // 效果管理
    //=========================================================================

    /**
     * @brief 启用/禁用 SSAO
     */
    void setSSAOEnabled(bool enabled);
    bool isSSAOEnabled() const;

    /**
     * @brief 获取 SSAO 效果
     */
    SSAOEffect* getSSAO() const { return _ssao.get(); }

    /**
     * @brief 启用/禁用 Bloom
     */
    void setBloomEnabled(bool enabled);
    bool isBloomEnabled() const;

    /**
     * @brief 获取 Bloom 效果
     */
    BloomEffect* getBloom() const { return _bloom.get(); }

    /**
     * @brief 检查是否有任何后处理效果启用
     */
    bool hasEnabledEffects() const;

    //=========================================================================
    // 纹理访问
    //=========================================================================

    /**
     * @brief 获取场景颜色纹理
     */
    osg::Texture2D* getSceneColorTexture() const { return _sceneColorTexture.get(); }

    /**
     * @brief 获取场景深度纹理
     */
    osg::Texture2D* getSceneDepthTexture() const { return _sceneDepthTexture.get(); }

    /**
     * @brief 获取最终输出纹理
     */
    osg::Texture2D* getFinalTexture() const { return _finalTexture.get(); }

private:
    void setupSceneRTT(osg::Camera* sceneCamera, int width, int height);
    void rebuildPipeline();

    osg::ref_ptr<osg::Group> _postProcessRoot;

    // 场景 RTT
    osg::ref_ptr<osg::Camera> _sceneRTTCamera;
    osg::ref_ptr<osg::Texture2D> _sceneColorTexture;
    osg::ref_ptr<osg::Texture2D> _sceneDepthTexture;

    // 效果
    std::unique_ptr<SSAOEffect> _ssao;
    std::unique_ptr<BloomEffect> _bloom;

    // 最终输出
    osg::ref_ptr<osg::Texture2D> _finalTexture;
    osg::ref_ptr<osg::Camera> _finalCamera;

    int _width{0};
    int _height{0};
    bool _initialized{false};
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_POSTPROCESS_H
