// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#include "OsgVersePostProcess.h"

#include <Base/Console.h>

#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/StateSet>
#include <osg/TexEnv>
#include <osg/Image>
#include <osg/GL>

#include <cmath>
#include <random>

// Define GL constants if not available
#ifndef GL_RGB16F
#define GL_RGB16F 0x881B
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif

namespace OsgVerseGui {

//=============================================================================
// PostProcessEffect 基类实现
//=============================================================================

PostProcessEffect::PostProcessEffect(const std::string& name)
    : _name(name)
{
}

PostProcessEffect::~PostProcessEffect()
{
}

void PostProcessEffect::setEnabled(bool enabled)
{
    _enabled = enabled;
    if (_camera) {
        _camera->setNodeMask(enabled ? ~0u : 0u);
    }
}

void PostProcessEffect::setIntensity(float intensity)
{
    _intensity = std::max(0.0f, std::min(1.0f, intensity));
}

osg::ref_ptr<osg::Geode> PostProcessEffect::createScreenQuad()
{
    osg::ref_ptr<osg::Geometry> quad = new osg::Geometry();

    // 全屏四边形顶点 (NDC 坐标)
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(4);
    (*vertices)[0] = osg::Vec3(-1.0f, -1.0f, 0.0f);
    (*vertices)[1] = osg::Vec3( 1.0f, -1.0f, 0.0f);
    (*vertices)[2] = osg::Vec3( 1.0f,  1.0f, 0.0f);
    (*vertices)[3] = osg::Vec3(-1.0f,  1.0f, 0.0f);
    quad->setVertexArray(vertices.get());

    // 纹理坐标
    osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array(4);
    (*texcoords)[0] = osg::Vec2(0.0f, 0.0f);
    (*texcoords)[1] = osg::Vec2(1.0f, 0.0f);
    (*texcoords)[2] = osg::Vec2(1.0f, 1.0f);
    (*texcoords)[3] = osg::Vec2(0.0f, 1.0f);
    quad->setTexCoordArray(0, texcoords.get());

    // 三角形索引
    osg::ref_ptr<osg::DrawElementsUShort> indices =
        new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES, 6);
    (*indices)[0] = 0; (*indices)[1] = 1; (*indices)[2] = 2;
    (*indices)[3] = 0; (*indices)[4] = 2; (*indices)[5] = 3;
    quad->addPrimitiveSet(indices.get());

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(quad.get());

    return geode;
}

osg::ref_ptr<osg::Camera> PostProcessEffect::createRTTCamera(
    int width, int height, osg::Texture2D* outputTexture)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera();

    camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT);
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setViewMatrix(osg::Matrix::identity());
    camera->setProjectionMatrix(osg::Matrix::ortho2D(-1, 1, -1, 1));
    camera->setViewport(0, 0, width, height);

    camera->setRenderOrder(osg::Camera::PRE_RENDER);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    if (outputTexture) {
        camera->attach(osg::Camera::COLOR_BUFFER, outputTexture);
    }

    return camera;
}

osg::ref_ptr<osg::Program> PostProcessEffect::createProgram(
    const std::string& vertSource, const std::string& fragSource)
{
    osg::ref_ptr<osg::Program> program = new osg::Program();

    osg::ref_ptr<osg::Shader> vertShader =
        new osg::Shader(osg::Shader::VERTEX, vertSource);
    osg::ref_ptr<osg::Shader> fragShader =
        new osg::Shader(osg::Shader::FRAGMENT, fragSource);

    program->addShader(vertShader.get());
    program->addShader(fragShader.get());

    return program;
}

std::string PostProcessEffect::getSimpleVertexShader()
{
    return R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

out vec2 vTexCoord;

void main() {
    vTexCoord = texCoord;
    gl_Position = vec4(position, 1.0);
}
)";
}

//=============================================================================
// SSAOEffect 实现
//=============================================================================

SSAOEffect::SSAOEffect()
    : PostProcessEffect("SSAO")
{
    generateSampleKernel();
}

SSAOEffect::~SSAOEffect()
{
}

osg::ref_ptr<osg::Texture2D> SSAOEffect::setup(
    int width, int height, osg::Texture2D* inputTexture)
{
    _width = width;
    _height = height;

    // 生成噪声纹理
    generateNoiseTexture();

    // 创建 SSAO 输出纹理
    _ssaoTexture = new osg::Texture2D();
    _ssaoTexture->setTextureSize(width, height);
    _ssaoTexture->setInternalFormat(GL_R8);
    _ssaoTexture->setSourceFormat(GL_RED);
    _ssaoTexture->setSourceType(GL_UNSIGNED_BYTE);
    _ssaoTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _ssaoTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    _ssaoTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    _ssaoTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    // 创建 SSAO pass
    createSSAOPass(width, height, inputTexture);

    // 如果启用模糊，创建模糊 pass
    if (_blurEnabled) {
        _outputTexture = new osg::Texture2D();
        _outputTexture->setTextureSize(width, height);
        _outputTexture->setInternalFormat(GL_R8);
        _outputTexture->setSourceFormat(GL_RED);
        _outputTexture->setSourceType(GL_UNSIGNED_BYTE);
        _outputTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        _outputTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

        createBlurPass(width, height);
    }
    else {
        _outputTexture = _ssaoTexture;
    }

    Base::Console().log("SSAOEffect: Setup complete (%dx%d)\n", width, height);

    return _outputTexture;
}

void SSAOEffect::resize(int width, int height)
{
    if (_width == width && _height == height) {
        return;
    }

    _width = width;
    _height = height;

    if (_ssaoTexture) {
        _ssaoTexture->setTextureSize(width, height);
        _ssaoTexture->dirtyTextureObject();
    }

    if (_outputTexture && _outputTexture != _ssaoTexture) {
        _outputTexture->setTextureSize(width, height);
        _outputTexture->dirtyTextureObject();
    }

    if (_camera) {
        _camera->setViewport(0, 0, width, height);
    }

    if (_blurCamera) {
        _blurCamera->setViewport(0, 0, width, height);
    }
}

void SSAOEffect::setRadius(float radius)
{
    _radius = std::max(0.01f, radius);
    if (_radiusUniform) {
        _radiusUniform->set(_radius);
    }
}

void SSAOEffect::setSampleCount(int count)
{
    _sampleCount = std::max(4, std::min(64, count));
    generateSampleKernel();
}

void SSAOEffect::setBias(float bias)
{
    _bias = std::max(0.0f, bias);
    if (_biasUniform) {
        _biasUniform->set(_bias);
    }
}

void SSAOEffect::setBlurEnabled(bool enabled)
{
    _blurEnabled = enabled;
}

void SSAOEffect::setDepthTexture(osg::Texture2D* depthTexture)
{
    _depthTexture = depthTexture;
}

void SSAOEffect::setNormalTexture(osg::Texture2D* normalTexture)
{
    _normalTexture = normalTexture;
}

void SSAOEffect::generateSampleKernel()
{
    _sampleKernel.clear();
    _sampleKernel.reserve(_sampleCount);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < _sampleCount; ++i) {
        // 半球内随机采样
        osg::Vec3 sample(
            dist(gen) * 2.0f - 1.0f,
            dist(gen) * 2.0f - 1.0f,
            dist(gen)  // 只在半球上方
        );
        sample.normalize();

        // 随机长度，靠近中心的采样更多
        float scale = static_cast<float>(i) / static_cast<float>(_sampleCount);
        scale = 0.1f + scale * scale * 0.9f;  // lerp(0.1, 1.0, scale^2)
        sample *= scale;

        _sampleKernel.push_back(sample);
    }
}

void SSAOEffect::generateNoiseTexture()
{
    const int noiseSize = 4;  // 4x4 噪声纹理

    osg::ref_ptr<osg::Image> noiseImage = new osg::Image();
    noiseImage->allocateImage(noiseSize, noiseSize, 1, GL_RGB, GL_FLOAT);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float* data = reinterpret_cast<float*>(noiseImage->data());
    for (int i = 0; i < noiseSize * noiseSize; ++i) {
        // 随机旋转向量 (绕 Z 轴)
        data[i * 3 + 0] = dist(gen) * 2.0f - 1.0f;  // x
        data[i * 3 + 1] = dist(gen) * 2.0f - 1.0f;  // y
        data[i * 3 + 2] = 0.0f;                      // z
    }

    _noiseTexture = new osg::Texture2D();
    _noiseTexture->setImage(noiseImage.get());
    _noiseTexture->setInternalFormat(GL_RGB16F);
    _noiseTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _noiseTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _noiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _noiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
}

void SSAOEffect::createSSAOPass(int width, int height, osg::Texture2D* inputTexture)
{
    _camera = createRTTCamera(width, height, _ssaoTexture.get());
    _camera->setName("SSAO_Pass");

    // 创建全屏四边形
    osg::ref_ptr<osg::Geode> quad = createScreenQuad();
    _camera->addChild(quad.get());

    // 创建着色器程序
    _program = createProgram(getSimpleVertexShader(), getSSAOFragmentShader());

    // 设置 StateSet
    osg::StateSet* ss = quad->getOrCreateStateSet();
    ss->setAttributeAndModes(_program.get(), osg::StateAttribute::ON);

    // 绑定纹理
    if (_depthTexture) {
        ss->setTextureAttributeAndModes(0, _depthTexture.get(), osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("depthTexture", 0));
    }

    if (_noiseTexture) {
        ss->setTextureAttributeAndModes(1, _noiseTexture.get(), osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("noiseTexture", 1));
    }

    // 设置 uniform
    _radiusUniform = new osg::Uniform("radius", _radius);
    _biasUniform = new osg::Uniform("bias", _bias);
    _intensityUniform = new osg::Uniform("intensity", _intensity);

    ss->addUniform(_radiusUniform.get());
    ss->addUniform(_biasUniform.get());
    ss->addUniform(_intensityUniform.get());
    ss->addUniform(new osg::Uniform("screenSize", osg::Vec2(float(width), float(height))));

    // 添加采样核心 uniform
    osg::ref_ptr<osg::Uniform> samplesUniform =
        new osg::Uniform(osg::Uniform::FLOAT_VEC3, "samples", _sampleCount);
    for (int i = 0; i < _sampleCount; ++i) {
        samplesUniform->setElement(i, _sampleKernel[i]);
    }
    ss->addUniform(samplesUniform.get());
    ss->addUniform(new osg::Uniform("sampleCount", _sampleCount));

    // 禁用深度测试
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
}

void SSAOEffect::createBlurPass(int width, int height)
{
    _blurCamera = createRTTCamera(width, height, _outputTexture.get());
    _blurCamera->setName("SSAO_Blur");

    osg::ref_ptr<osg::Geode> quad = createScreenQuad();
    _blurCamera->addChild(quad.get());

    _blurProgram = createProgram(getSimpleVertexShader(), getBlurFragmentShader());

    osg::StateSet* ss = quad->getOrCreateStateSet();
    ss->setAttributeAndModes(_blurProgram.get(), osg::StateAttribute::ON);

    ss->setTextureAttributeAndModes(0, _ssaoTexture.get(), osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("ssaoTexture", 0));
    ss->addUniform(new osg::Uniform("texelSize", osg::Vec2(1.0f / width, 1.0f / height)));

    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
}

std::string SSAOEffect::getSSAOFragmentShader()
{
    return R"(
#version 330 core
in vec2 vTexCoord;
out float fragColor;

uniform sampler2D depthTexture;
uniform sampler2D noiseTexture;

uniform vec2 screenSize;
uniform float radius;
uniform float bias;
uniform float intensity;
uniform int sampleCount;
uniform vec3 samples[64];

// 从深度重建视图空间位置
vec3 reconstructPosition(vec2 uv, float depth) {
    // 简化的位置重建 (假设透视投影)
    float z = depth * 2.0 - 1.0;
    vec3 pos;
    pos.xy = uv * 2.0 - 1.0;
    pos.z = z;
    return pos;
}

void main() {
    float depth = texture(depthTexture, vTexCoord).r;

    // 如果是背景，跳过 SSAO
    if (depth >= 1.0) {
        fragColor = 1.0;
        return;
    }

    vec3 fragPos = reconstructPosition(vTexCoord, depth);

    // 获取噪声向量
    vec2 noiseScale = screenSize / 4.0;
    vec3 randomVec = texture(noiseTexture, vTexCoord * noiseScale).xyz;

    // 创建 TBN 矩阵 (简化版，假设法线朝向 +Z)
    vec3 normal = vec3(0.0, 0.0, 1.0);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // 采样
    float occlusion = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        vec3 sampleDir = TBN * samples[i];
        vec3 samplePos = fragPos + sampleDir * radius;

        // 投影到屏幕空间
        vec2 sampleUV = samplePos.xy * 0.5 + 0.5;

        // 获取采样点深度
        float sampleDepth = texture(depthTexture, sampleUV).r;

        // 范围检查
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(depth - sampleDepth));

        // 遮蔽检查
        occlusion += (sampleDepth >= depth + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(sampleCount)) * intensity;
    fragColor = occlusion;
}
)";
}

std::string SSAOEffect::getBlurFragmentShader()
{
    return R"(
#version 330 core
in vec2 vTexCoord;
out float fragColor;

uniform sampler2D ssaoTexture;
uniform vec2 texelSize;

void main() {
    float result = 0.0;

    // 4x4 box blur
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoTexture, vTexCoord + offset).r;
        }
    }

    fragColor = result / 16.0;
}
)";
}

//=============================================================================
// BloomEffect 实现
//=============================================================================

BloomEffect::BloomEffect()
    : PostProcessEffect("Bloom")
{
}

BloomEffect::~BloomEffect()
{
}

osg::ref_ptr<osg::Texture2D> BloomEffect::setup(
    int width, int height, osg::Texture2D* inputTexture)
{
    _width = width;
    _height = height;

    // 创建亮度提取纹理
    _brightTexture = new osg::Texture2D();
    _brightTexture->setTextureSize(width / 2, height / 2);
    _brightTexture->setInternalFormat(GL_RGB16F);
    _brightTexture->setSourceFormat(GL_RGB);
    _brightTexture->setSourceType(GL_FLOAT);
    _brightTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _brightTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    _brightTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    _brightTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    // 创建最终输出纹理
    _outputTexture = new osg::Texture2D();
    _outputTexture->setTextureSize(width, height);
    _outputTexture->setInternalFormat(GL_RGB8);
    _outputTexture->setSourceFormat(GL_RGB);
    _outputTexture->setSourceType(GL_UNSIGNED_BYTE);
    _outputTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _outputTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    // 创建各个 pass
    createBrightPassCamera(width, height, inputTexture);
    createBlurPassCameras(width, height);
    createCompositeCamera(width, height, inputTexture);

    Base::Console().log("BloomEffect: Setup complete (%dx%d, %d mip levels)\n",
                        width, height, _mipLevels);

    return _outputTexture;
}

void BloomEffect::resize(int width, int height)
{
    if (_width == width && _height == height) {
        return;
    }

    _width = width;
    _height = height;

    if (_brightTexture) {
        _brightTexture->setTextureSize(width / 2, height / 2);
        _brightTexture->dirtyTextureObject();
    }

    if (_outputTexture) {
        _outputTexture->setTextureSize(width, height);
        _outputTexture->dirtyTextureObject();
    }

    // 更新模糊纹理大小
    int w = width / 2;
    int h = height / 2;
    for (size_t i = 0; i < _blurTextures.size(); ++i) {
        _blurTextures[i]->setTextureSize(w, h);
        _blurTextures[i]->dirtyTextureObject();
        if (_blurCameras[i]) {
            _blurCameras[i]->setViewport(0, 0, w, h);
        }
        w /= 2;
        h /= 2;
    }

    if (_brightPassCamera) {
        _brightPassCamera->setViewport(0, 0, width / 2, height / 2);
    }

    if (_compositeCamera) {
        _compositeCamera->setViewport(0, 0, width, height);
    }
}

void BloomEffect::setThreshold(float threshold)
{
    _threshold = std::max(0.0f, threshold);
    if (_thresholdUniform) {
        _thresholdUniform->set(_threshold);
    }
}

void BloomEffect::setBlurStrength(float strength)
{
    _blurStrength = std::max(0.0f, strength);
}

void BloomEffect::setMipLevels(int levels)
{
    _mipLevels = std::max(1, std::min(8, levels));
}

void BloomEffect::createBrightPassCamera(int width, int height, osg::Texture2D* inputTexture)
{
    _brightPassCamera = createRTTCamera(width / 2, height / 2, _brightTexture.get());
    _brightPassCamera->setName("Bloom_BrightPass");

    osg::ref_ptr<osg::Geode> quad = createScreenQuad();
    _brightPassCamera->addChild(quad.get());

    osg::ref_ptr<osg::Program> program = createProgram(
        getSimpleVertexShader(),  // 复用简单的顶点着色器
        getBrightPassFragmentShader()
    );

    osg::StateSet* ss = quad->getOrCreateStateSet();
    ss->setAttributeAndModes(program.get(), osg::StateAttribute::ON);

    ss->setTextureAttributeAndModes(0, inputTexture, osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("sceneTexture", 0));

    _thresholdUniform = new osg::Uniform("threshold", _threshold);
    ss->addUniform(_thresholdUniform.get());

    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
}

void BloomEffect::createBlurPassCameras(int width, int height)
{
    _blurCameras.clear();
    _blurTextures.clear();

    int w = width / 2;
    int h = height / 2;

    osg::Texture2D* prevTexture = _brightTexture.get();

    for (int i = 0; i < _mipLevels; ++i) {
        w = std::max(1, w / 2);
        h = std::max(1, h / 2);

        // 创建模糊输出纹理
        osg::ref_ptr<osg::Texture2D> blurTex = new osg::Texture2D();
        blurTex->setTextureSize(w, h);
        blurTex->setInternalFormat(GL_RGB16F);
        blurTex->setSourceFormat(GL_RGB);
        blurTex->setSourceType(GL_FLOAT);
        blurTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        blurTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        blurTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        blurTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        _blurTextures.push_back(blurTex);

        // 创建模糊相机
        osg::ref_ptr<osg::Camera> blurCam = createRTTCamera(w, h, blurTex.get());
        blurCam->setName("Bloom_Blur_" + std::to_string(i));

        osg::ref_ptr<osg::Geode> quad = createScreenQuad();
        blurCam->addChild(quad.get());

        osg::ref_ptr<osg::Program> program = createProgram(
            getSimpleVertexShader(),
            getBlurFragmentShader()
        );

        osg::StateSet* ss = quad->getOrCreateStateSet();
        ss->setAttributeAndModes(program.get(), osg::StateAttribute::ON);
        ss->setTextureAttributeAndModes(0, prevTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("inputTexture", 0));
        ss->addUniform(new osg::Uniform("texelSize", osg::Vec2(1.0f / w, 1.0f / h)));
        ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

        _blurCameras.push_back(blurCam);
        prevTexture = blurTex.get();
    }
}

void BloomEffect::createCompositeCamera(int width, int height, osg::Texture2D* inputTexture)
{
    _compositeCamera = createRTTCamera(width, height, _outputTexture.get());
    _compositeCamera->setName("Bloom_Composite");
    _camera = _compositeCamera;  // 主相机

    osg::ref_ptr<osg::Geode> quad = createScreenQuad();
    _compositeCamera->addChild(quad.get());

    osg::ref_ptr<osg::Program> program = createProgram(
        getSimpleVertexShader(),
        getCompositeFragmentShader()
    );

    osg::StateSet* ss = quad->getOrCreateStateSet();
    ss->setAttributeAndModes(program.get(), osg::StateAttribute::ON);

    // 原始场景纹理
    ss->setTextureAttributeAndModes(0, inputTexture, osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("sceneTexture", 0));

    // 模糊后的高光纹理 (使用最后一级)
    if (!_blurTextures.empty()) {
        ss->setTextureAttributeAndModes(1, _blurTextures.back().get(), osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("bloomTexture", 1));
    }

    _bloomIntensityUniform = new osg::Uniform("bloomIntensity", _intensity);
    ss->addUniform(_bloomIntensityUniform.get());

    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
}

std::string BloomEffect::getBrightPassFragmentShader()
{
    return R"(
#version 330 core
in vec2 vTexCoord;
out vec3 fragColor;

uniform sampler2D sceneTexture;
uniform float threshold;

void main() {
    vec3 color = texture(sceneTexture, vTexCoord).rgb;

    // 计算亮度
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // 阈值过滤
    if (brightness > threshold) {
        fragColor = color;
    } else {
        fragColor = vec3(0.0);
    }
}
)";
}

std::string BloomEffect::getBlurFragmentShader()
{
    return R"(
#version 330 core
in vec2 vTexCoord;
out vec3 fragColor;

uniform sampler2D inputTexture;
uniform vec2 texelSize;

// 9-tap 高斯模糊权重
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec3 result = texture(inputTexture, vTexCoord).rgb * weights[0];

    // 水平和垂直模糊
    for (int i = 1; i < 5; ++i) {
        result += texture(inputTexture, vTexCoord + vec2(texelSize.x * i, 0.0)).rgb * weights[i];
        result += texture(inputTexture, vTexCoord - vec2(texelSize.x * i, 0.0)).rgb * weights[i];
        result += texture(inputTexture, vTexCoord + vec2(0.0, texelSize.y * i)).rgb * weights[i];
        result += texture(inputTexture, vTexCoord - vec2(0.0, texelSize.y * i)).rgb * weights[i];
    }

    fragColor = result;
}
)";
}

std::string BloomEffect::getCompositeFragmentShader()
{
    return R"(
#version 330 core
in vec2 vTexCoord;
out vec3 fragColor;

uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform float bloomIntensity;

void main() {
    vec3 scene = texture(sceneTexture, vTexCoord).rgb;
    vec3 bloom = texture(bloomTexture, vTexCoord).rgb;

    // 叠加泛光
    fragColor = scene + bloom * bloomIntensity;
}
)";
}

//=============================================================================
// PostProcessManager 实现
//=============================================================================

PostProcessManager::PostProcessManager()
{
    _postProcessRoot = new osg::Group();
    _postProcessRoot->setName("PostProcessRoot");
}

PostProcessManager::~PostProcessManager()
{
}

void PostProcessManager::initialize(osg::Camera* sceneCamera, int width, int height)
{
    _width = width;
    _height = height;

    // 设置场景 RTT
    setupSceneRTT(sceneCamera, width, height);

    // 创建效果
    _ssao = std::make_unique<SSAOEffect>();
    _bloom = std::make_unique<BloomEffect>();

    // 默认禁用
    _ssao->setEnabled(false);
    _bloom->setEnabled(false);

    _initialized = true;

    Base::Console().log("PostProcessManager: Initialized (%dx%d)\n", width, height);
}

void PostProcessManager::resize(int width, int height)
{
    if (_width == width && _height == height) {
        return;
    }

    _width = width;
    _height = height;

    if (_sceneColorTexture) {
        _sceneColorTexture->setTextureSize(width, height);
        _sceneColorTexture->dirtyTextureObject();
    }

    if (_sceneDepthTexture) {
        _sceneDepthTexture->setTextureSize(width, height);
        _sceneDepthTexture->dirtyTextureObject();
    }

    if (_ssao) {
        _ssao->resize(width, height);
    }

    if (_bloom) {
        _bloom->resize(width, height);
    }

    if (_sceneRTTCamera) {
        _sceneRTTCamera->setViewport(0, 0, width, height);
    }
}

void PostProcessManager::setSSAOEnabled(bool enabled)
{
    if (_ssao) {
        _ssao->setEnabled(enabled);
        if (enabled && _ssao->getOutputTexture() == nullptr) {
            _ssao->setDepthTexture(_sceneDepthTexture.get());
            _ssao->setup(_width, _height, _sceneColorTexture.get());
        }
        rebuildPipeline();
    }
}

bool PostProcessManager::isSSAOEnabled() const
{
    return _ssao && _ssao->isEnabled();
}

void PostProcessManager::setBloomEnabled(bool enabled)
{
    if (_bloom) {
        _bloom->setEnabled(enabled);
        if (enabled && _bloom->getOutputTexture() == nullptr) {
            _bloom->setup(_width, _height, _sceneColorTexture.get());
        }
        rebuildPipeline();
    }
}

bool PostProcessManager::isBloomEnabled() const
{
    return _bloom && _bloom->isEnabled();
}

bool PostProcessManager::hasEnabledEffects() const
{
    return isSSAOEnabled() || isBloomEnabled();
}

void PostProcessManager::setupSceneRTT(osg::Camera* sceneCamera, int width, int height)
{
    // 创建颜色纹理
    _sceneColorTexture = new osg::Texture2D();
    _sceneColorTexture->setTextureSize(width, height);
    _sceneColorTexture->setInternalFormat(GL_RGB8);
    _sceneColorTexture->setSourceFormat(GL_RGB);
    _sceneColorTexture->setSourceType(GL_UNSIGNED_BYTE);
    _sceneColorTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _sceneColorTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    _sceneColorTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    _sceneColorTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    // 创建深度纹理
    _sceneDepthTexture = new osg::Texture2D();
    _sceneDepthTexture->setTextureSize(width, height);
    _sceneDepthTexture->setInternalFormat(GL_DEPTH_COMPONENT24);
    _sceneDepthTexture->setSourceFormat(GL_DEPTH_COMPONENT);
    _sceneDepthTexture->setSourceType(GL_FLOAT);
    _sceneDepthTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _sceneDepthTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _sceneDepthTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    _sceneDepthTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    // 注意：实际的场景 RTT 设置需要在 OsgVerseViewer 中完成
    // 这里只是创建纹理，相机设置需要与 viewer 的渲染相机集成
}

void PostProcessManager::rebuildPipeline()
{
    // 清除现有子节点
    _postProcessRoot->removeChildren(0, _postProcessRoot->getNumChildren());

    // 按顺序添加启用的效果相机
    if (_ssao && _ssao->isEnabled()) {
        if (_ssao->getCamera()) {
            _postProcessRoot->addChild(_ssao->getCamera());
        }
    }

    if (_bloom && _bloom->isEnabled()) {
        // 添加所有 bloom pass 相机
        if (auto* bloom = dynamic_cast<BloomEffect*>(_bloom.get())) {
            if (bloom->getCamera()) {
                _postProcessRoot->addChild(bloom->getCamera());
            }
        }
    }
}

} // namespace OsgVerseGui
