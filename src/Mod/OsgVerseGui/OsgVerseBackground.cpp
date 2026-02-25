// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#include "OsgVerseBackground.h"

#include <osg/StateSet>
#include <osg/Depth>
#include <osg/BlendFunc>
#include <osg/Vec4>
#include <osg/Shader>

namespace OsgVerseGui {

// Vertex shader for background quad
static const char* backgroundVertexShader = R"(
#version 120

varying vec2 texCoord;

void main()
{
    texCoord = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
}
)";

// Fragment shader for gradient background
static const char* backgroundFragmentShader = R"(
#version 120

varying vec2 texCoord;

uniform int gradientType;       // 0=None/Solid, 1=Linear, 2=Radial, 3=Corner
uniform vec4 topColor;          // Top color (linear) or center color (radial)
uniform vec4 bottomColor;       // Bottom color (linear) or edge color (radial)
uniform vec4 topLeftColor;      // Top-left corner (corner gradient)
uniform vec4 topRightColor;     // Top-right corner (corner gradient)
uniform vec4 bottomLeftColor;   // Bottom-left corner (corner gradient)
uniform vec4 bottomRightColor;  // Bottom-right corner (corner gradient)
uniform vec4 solidColor;        // Solid background color
uniform float midPoint;         // Gradient midpoint (0.0-1.0)

void main()
{
    vec4 color;

    if (gradientType == 0) {
        // Solid color
        color = solidColor;
    }
    else if (gradientType == 1) {
        // Linear gradient (top to bottom)
        float t = texCoord.y;

        // Apply midpoint adjustment for non-linear interpolation
        if (t < midPoint) {
            t = t / midPoint * 0.5;
        } else {
            t = 0.5 + (t - midPoint) / (1.0 - midPoint) * 0.5;
        }

        color = mix(bottomColor, topColor, t);
    }
    else if (gradientType == 2) {
        // Radial gradient (center to edge)
        vec2 center = vec2(0.5, 0.5);
        float dist = length(texCoord - center) * 2.0; // Normalize to 0-1 range
        dist = clamp(dist, 0.0, 1.0);

        // Apply midpoint adjustment
        if (dist < midPoint) {
            dist = dist / midPoint * 0.5;
        } else {
            dist = 0.5 + (dist - midPoint) / (1.0 - midPoint) * 0.5;
        }

        color = mix(topColor, bottomColor, dist); // topColor is center, bottomColor is edge
    }
    else if (gradientType == 3) {
        // Corner gradient (bilinear interpolation)
        vec4 topMix = mix(topLeftColor, topRightColor, texCoord.x);
        vec4 bottomMix = mix(bottomLeftColor, bottomRightColor, texCoord.x);
        color = mix(bottomMix, topMix, texCoord.y);
    }
    else {
        color = solidColor;
    }

    gl_FragColor = color;
}
)";

OsgVerseBackground::OsgVerseBackground()
    : _bgColor(0.2f, 0.2f, 0.3f)
{
    _gradient.type = Gui::View3D::BackgroundGradientType::None;
    createBackground();
    createShaders();
    updateUniforms();
}

OsgVerseBackground::~OsgVerseBackground()
{
}

void OsgVerseBackground::createBackground()
{
    // Create background camera
    _camera = new osg::Camera;
    _camera->setName("BackgroundCamera");

    // Set up camera to render as part of the scene graph (NESTED_RENDER)
    // This ensures it renders to the main frame buffer
    _camera->setClearMask(0); // Don't clear - we're drawing the background
    _camera->setRenderOrder(osg::Camera::NESTED_RENDER, -100); // Render first within the scene
    _camera->setAllowEventFocus(false);

    // Use absolute reference frame (not affected by parent transforms)
    _camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);

    // Set up orthographic projection for full-screen quad
    _camera->setProjectionMatrix(osg::Matrix::ortho2D(-1.0, 1.0, -1.0, 1.0));
    _camera->setViewMatrix(osg::Matrix::identity());

    // Create full-screen quad geometry
    _quad = new osg::Geometry;
    _quad->setName("BackgroundQuad");

    // Vertices for full-screen quad (NDC coordinates)
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(-1.0f, -1.0f, 0.0f)); // Bottom-left
    vertices->push_back(osg::Vec3( 1.0f, -1.0f, 0.0f)); // Bottom-right
    vertices->push_back(osg::Vec3( 1.0f,  1.0f, 0.0f)); // Top-right
    vertices->push_back(osg::Vec3(-1.0f,  1.0f, 0.0f)); // Top-left
    _quad->setVertexArray(vertices);

    // Texture coordinates (0-1 range for shader)
    osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array;
    texCoords->push_back(osg::Vec2(0.0f, 0.0f)); // Bottom-left
    texCoords->push_back(osg::Vec2(1.0f, 0.0f)); // Bottom-right
    texCoords->push_back(osg::Vec2(1.0f, 1.0f)); // Top-right
    texCoords->push_back(osg::Vec2(0.0f, 1.0f)); // Top-left
    _quad->setTexCoordArray(0, texCoords);

    // Draw as quad
    _quad->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

    // Create geode to hold the quad
    _geode = new osg::Geode;
    _geode->setName("BackgroundGeode");
    _geode->addDrawable(_quad);

    // Set up state set
    osg::StateSet* stateSet = _geode->getOrCreateStateSet();

    // Disable depth testing and writing (background is always behind everything)
    osg::ref_ptr<osg::Depth> depth = new osg::Depth;
    depth->setFunction(osg::Depth::ALWAYS);
    depth->setWriteMask(false);
    stateSet->setAttributeAndModes(depth, osg::StateAttribute::ON);

    // Disable lighting
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    // Disable culling for background (it should always be visible)
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    // Set render bin to ensure background renders first
    stateSet->setRenderBinDetails(-1, "RenderBin");

    // Add geode to camera
    _camera->addChild(_geode);

    // Disable culling on the camera itself
    _camera->setCullingActive(false);
}

void OsgVerseBackground::createShaders()
{
    _program = new osg::Program;
    _program->setName("BackgroundGradientProgram");

    // Create and attach vertex shader
    osg::ref_ptr<osg::Shader> vertShader = new osg::Shader(osg::Shader::VERTEX, backgroundVertexShader);
    vertShader->setName("BackgroundVertexShader");
    _program->addShader(vertShader);

    // Create and attach fragment shader
    osg::ref_ptr<osg::Shader> fragShader = new osg::Shader(osg::Shader::FRAGMENT, backgroundFragmentShader);
    fragShader->setName("BackgroundFragmentShader");
    _program->addShader(fragShader);

    // Create uniforms
    _gradientType = new osg::Uniform("gradientType", 0);
    _topColor = new osg::Uniform("topColor", osg::Vec4(0.4f, 0.4f, 0.6f, 1.0f));
    _bottomColor = new osg::Uniform("bottomColor", osg::Vec4(0.1f, 0.1f, 0.2f, 1.0f));
    _topLeftColor = new osg::Uniform("topLeftColor", osg::Vec4(0.3f, 0.3f, 0.5f, 1.0f));
    _topRightColor = new osg::Uniform("topRightColor", osg::Vec4(0.4f, 0.4f, 0.6f, 1.0f));
    _bottomLeftColor = new osg::Uniform("bottomLeftColor", osg::Vec4(0.1f, 0.1f, 0.2f, 1.0f));
    _bottomRightColor = new osg::Uniform("bottomRightColor", osg::Vec4(0.2f, 0.2f, 0.3f, 1.0f));
    _solidColor = new osg::Uniform("solidColor", osg::Vec4(0.2f, 0.2f, 0.3f, 1.0f));
    _midPoint = new osg::Uniform("midPoint", 0.5f);

    // Apply shader and uniforms to geode
    osg::StateSet* stateSet = _geode->getOrCreateStateSet();
    stateSet->setAttributeAndModes(_program, osg::StateAttribute::ON);
    stateSet->addUniform(_gradientType);
    stateSet->addUniform(_topColor);
    stateSet->addUniform(_bottomColor);
    stateSet->addUniform(_topLeftColor);
    stateSet->addUniform(_topRightColor);
    stateSet->addUniform(_bottomLeftColor);
    stateSet->addUniform(_bottomRightColor);
    stateSet->addUniform(_solidColor);
    stateSet->addUniform(_midPoint);
}

void OsgVerseBackground::setGradient(const Gui::View3D::BackgroundGradient& gradient)
{
    _gradient = gradient;
    updateUniforms();
}

void OsgVerseBackground::setSolidColor(const Base::Color& color)
{
    _bgColor = color;
    if (_solidColor) {
        _solidColor->set(osg::Vec4(color.r, color.g, color.b, 1.0f));
    }
}

void OsgVerseBackground::updateUniforms()
{
    if (!_gradientType) return;

    // Set gradient type
    int type = static_cast<int>(_gradient.type);
    _gradientType->set(type);

    // Set colors based on gradient type
    switch (_gradient.type) {
        case Gui::View3D::BackgroundGradientType::None:
            _solidColor->set(osg::Vec4(_bgColor.r, _bgColor.g, _bgColor.b, 1.0f));
            break;

        case Gui::View3D::BackgroundGradientType::Linear:
            _topColor->set(osg::Vec4(_gradient.topColor.r, _gradient.topColor.g,
                                      _gradient.topColor.b, 1.0f));
            _bottomColor->set(osg::Vec4(_gradient.bottomColor.r, _gradient.bottomColor.g,
                                         _gradient.bottomColor.b, 1.0f));
            break;

        case Gui::View3D::BackgroundGradientType::Radial:
            _topColor->set(osg::Vec4(_gradient.topColor.r, _gradient.topColor.g,
                                      _gradient.topColor.b, 1.0f)); // Center color
            _bottomColor->set(osg::Vec4(_gradient.bottomColor.r, _gradient.bottomColor.g,
                                         _gradient.bottomColor.b, 1.0f)); // Edge color
            break;

        case Gui::View3D::BackgroundGradientType::Corner:
            _topLeftColor->set(osg::Vec4(_gradient.topLeftColor.r, _gradient.topLeftColor.g,
                                          _gradient.topLeftColor.b, 1.0f));
            _topRightColor->set(osg::Vec4(_gradient.topRightColor.r, _gradient.topRightColor.g,
                                           _gradient.topRightColor.b, 1.0f));
            _bottomLeftColor->set(osg::Vec4(_gradient.bottomLeftColor.r, _gradient.bottomLeftColor.g,
                                             _gradient.bottomLeftColor.b, 1.0f));
            _bottomRightColor->set(osg::Vec4(_gradient.bottomRightColor.r, _gradient.bottomRightColor.g,
                                              _gradient.bottomRightColor.b, 1.0f));
            break;
    }

    // Set midpoint
    _midPoint->set(_gradient.midPoint);
}

void OsgVerseBackground::resize(int width, int height)
{
    _width = width;
    _height = height;

    if (_camera) {
        _camera->setViewport(0, 0, width, height);
    }
}

void OsgVerseBackground::setEnabled(bool enabled)
{
    _enabled = enabled;
    if (_camera) {
        _camera->setNodeMask(enabled ? ~0u : 0u);
    }
}

} // namespace OsgVerseGui
