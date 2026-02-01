// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseNaviCube.h"
#include "OsgVerseViewer.h"
#include <Gui/View3D/IViewer3D.h>
#include <Base/Console.h>

#include <QDateTime>
#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

#include <cmath>
#include <numbers>
#include <algorithm>

#ifdef FC_OS_WIN32
#include <windows.h>
#endif
#ifdef FC_OS_MACOSX
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace OsgVerseGui {

//===========================================================================
// Constructor / Destructor
//===========================================================================

OsgVerseNaviCube::OsgVerseNaviCube(OsgVerseViewer* viewer)
    : _viewer(viewer)
    , _animationTimer(new QTimer(this))
{
    // Set default labels
    _labels = {"FRONT", "TOP", "RIGHT", "REAR", "BOTTOM", "LEFT"};

    // Initialize colors
    updateColors();

    // Setup animation timer
    _animationTimer->setInterval(16);  // ~60fps
    connect(_animationTimer, &QTimer::timeout,
            this, &OsgVerseNaviCube::updateCameraAnimation);
}

OsgVerseNaviCube::~OsgVerseNaviCube()
{
    if (_pickingFramebuffer) {
        delete _pickingFramebuffer;
        _pickingFramebuffer = nullptr;
    }

    // Delete textures
    for (auto& pair : _labelTextures) {
        if (pair.second.texture) {
            delete pair.second.texture;
            pair.second.texture = nullptr;
        }
    }
}

//===========================================================================
// Core Interface
//===========================================================================

void OsgVerseNaviCube::setEnabled(bool enabled)
{
    if (_enabled != enabled) {
        _enabled = enabled;
        scheduleRedraw();
    }
}

bool OsgVerseNaviCube::isEnabled() const
{
    return _enabled;
}

void OsgVerseNaviCube::draw()
{
    if (!_enabled || !_viewer) {
        return;
    }

    if (!_prepared) {
        prepare();
        _prepared = true;
        // Don't return - continue to draw on first frame
    }

    ensurePickingFramebuffer();
    handleResize();

    drawNaviCube(false, _hovering ? 1.0f : _inactiveOpacity);
}

bool OsgVerseNaviCube::handleMouseEvent(QMouseEvent* event)
{
    if (!_enabled || !_prepared) {
        return false;
    }

    // Ensure we have current viewport dimensions
    handleResize();

    QPoint localPos = translateToLocal(event->pos());
    short x = static_cast<short>(localPos.x());
    short y = static_cast<short>(localPos.y());

    // Check if mouse is in NaviCube area
    float logicalSize = static_cast<float>(_cubeSize);
    bool inNaviCubeArea = std::abs(x) <= logicalSize / 2 && std::abs(y) <= logicalSize / 2;

    switch (event->type()) {
        case QEvent::MouseButtonPress:
            if (event->button() == Qt::LeftButton) {
                return mousePressed(x, y);
            }
            break;

        case QEvent::MouseButtonRelease:
            if (event->button() == Qt::LeftButton) {
                return mouseReleased(x, y);
            }
            break;

        case QEvent::MouseMove:
            return mouseMoved(x, y);

        case QEvent::MouseButtonDblClick:
            // Handle double click - consume the event if in NaviCube area
            // to prevent mouse stickiness
            if (inNaviCubeArea && event->button() == Qt::LeftButton) {
                // Reset mouse state
                _mouseDown = false;
                _dragging = false;
                _mightDrag = false;
                return true;  // Consume the event
            }
            break;

        default:
            break;
    }

    return false;
}

void OsgVerseNaviCube::resize(int width, int height)
{
    if (_viewWidth != width || _viewHeight != height) {
        _viewWidth = width;
        _viewHeight = height;
    }
}

//===========================================================================
// Configuration Interface
//===========================================================================

void OsgVerseNaviCube::setCorner(Corner corner)
{
    _corner = corner;
    switch (corner) {
        case CornerTopLeft:
            _relPosX = 0.0f;
            _relPosY = 1.0f;
            break;
        case CornerTopRight:
            _relPosX = 1.0f;
            _relPosY = 1.0f;
            break;
        case CornerBottomLeft:
            _relPosX = 0.0f;
            _relPosY = 0.0f;
            break;
        case CornerBottomRight:
            _relPosX = 1.0f;
            _relPosY = 0.0f;
            break;
    }
    scheduleRedraw();
}

OsgVerseNaviCube::Corner OsgVerseNaviCube::getCorner() const
{
    return _corner;
}

void OsgVerseNaviCube::setSize(int size)
{
    if (_cubeSize != size) {
        _cubeSize = size;
        _prepared = false;
        _viewWidth = 0;
        scheduleRedraw();
    }
}

int OsgVerseNaviCube::getSize() const
{
    return _cubeSize;
}

void OsgVerseNaviCube::setLabels(const std::vector<std::string>& labels)
{
    if (labels.size() >= 6) {
        _labelTextures[Front].label = labels[0];
        _labelTextures[Top].label = labels[1];
        _labelTextures[Right].label = labels[2];
        _labelTextures[Rear].label = labels[3];
        _labelTextures[Bottom].label = labels[4];
        _labelTextures[Left].label = labels[5];
        _prepared = false;
        scheduleRedraw();
    }
}

void OsgVerseNaviCube::setBaseColor(const QColor& color)
{
    _baseColor = color;
    scheduleRedraw();
}

QColor OsgVerseNaviCube::getBaseColor() const
{
    return _baseColor;
}

void OsgVerseNaviCube::setHighlightColor(const QColor& color)
{
    _hiliteColor = color;
    scheduleRedraw();
}

QColor OsgVerseNaviCube::getHighlightColor() const
{
    return _hiliteColor;
}

void OsgVerseNaviCube::setEmphasisColor(const QColor& color)
{
    _emphasisColor = color;
    _prepared = false;
    scheduleRedraw();
}

QColor OsgVerseNaviCube::getEmphasisColor() const
{
    return _emphasisColor;
}

void OsgVerseNaviCube::setChamfer(float chamfer)
{
    _chamfer = std::clamp(chamfer, 0.05f, 0.18f);
    _prepared = false;
    scheduleRedraw();
}

float OsgVerseNaviCube::getChamfer() const
{
    return _chamfer;
}

void OsgVerseNaviCube::setRotateToNearest(bool enable)
{
    _rotateToNearest = enable;
}

bool OsgVerseNaviCube::getRotateToNearest() const
{
    return _rotateToNearest;
}

void OsgVerseNaviCube::setFontZoom(float zoom)
{
    _fontZoom = zoom;
    _prepared = false;
    scheduleRedraw();
}

void OsgVerseNaviCube::setInactiveOpacity(float opacity)
{
    _inactiveOpacity = std::clamp(opacity, 0.0f, 1.0f);
    scheduleRedraw();
}

void OsgVerseNaviCube::setDraggable(bool draggable)
{
    _draggable = draggable;
}

bool OsgVerseNaviCube::isDraggable() const
{
    return _draggable;
}

void OsgVerseNaviCube::updateColors()
{
    // Default axis colors (RGB XYZ convention)
    _xColor = osg::Vec4f(1.0f, 0.2f, 0.2f, 1.0f);  // Red for X
    _yColor = osg::Vec4f(0.2f, 0.8f, 0.2f, 1.0f);  // Green for Y
    _zColor = osg::Vec4f(0.2f, 0.2f, 1.0f, 1.0f);  // Blue for Z
}

//===========================================================================
// Geometry Creation
//===========================================================================

void OsgVerseNaviCube::prepare()
{
    // Get device pixel ratio
    if (_viewer) {
        QWidget* widget = _viewer->getWidget();
        if (widget) {
            _devicePixelRatio = widget->devicePixelRatioF();
            _viewWidth = widget->width();
            _viewHeight = widget->height();
        }
    }

    createCubeFaceTextures();

    constexpr float pi = std::numbers::pi_v<float>;
    constexpr float pi1_2 = pi / 2;

    osg::Vec3f x(1, 0, 0);
    osg::Vec3f y(0, 1, 0);
    osg::Vec3f z(0, 0, 1);

    // Create the main faces
    addCubeFace(x, z, ShapeMain, Top);
    addCubeFace(x, -y, ShapeMain, Front);
    addCubeFace(-y, -x, ShapeMain, Left);
    addCubeFace(-x, y, ShapeMain, Rear);
    addCubeFace(y, x, ShapeMain, Right);
    addCubeFace(x, -z, ShapeMain, Bottom);

    // Create corner faces
    addCubeFace(-x - y, x - y + z, ShapeCorner, FrontTopRight, pi);
    addCubeFace(-x + y, -x - y + z, ShapeCorner, FrontTopLeft, pi);
    addCubeFace(x + y, x - y - z, ShapeCorner, FrontBottomRight);
    addCubeFace(x - y, -x - y - z, ShapeCorner, FrontBottomLeft);
    addCubeFace(x - y, x + y + z, ShapeCorner, RearTopRight, pi);
    addCubeFace(x + y, -x + y + z, ShapeCorner, RearTopLeft, pi);
    addCubeFace(-x + y, x + y - z, ShapeCorner, RearBottomRight);
    addCubeFace(-x - y, -x + y - z, ShapeCorner, RearBottomLeft);

    // Create edge faces
    addCubeFace(x, z - y, ShapeEdge, FrontTop);
    addCubeFace(x, -z - y, ShapeEdge, FrontBottom);
    addCubeFace(x, y - z, ShapeEdge, RearBottom, pi);
    addCubeFace(x, y + z, ShapeEdge, RearTop, pi);
    addCubeFace(z, x + y, ShapeEdge, RearRight, pi1_2);
    addCubeFace(z, x - y, ShapeEdge, FrontRight, pi1_2);
    addCubeFace(z, -x - y, ShapeEdge, FrontLeft, pi1_2);
    addCubeFace(z, y - x, ShapeEdge, RearLeft, pi1_2);
    addCubeFace(y, z - x, ShapeEdge, TopLeft, pi);
    addCubeFace(y, x + z, ShapeEdge, TopRight);
    addCubeFace(y, x - z, ShapeEdge, BottomRight);
    addCubeFace(y, -z - x, ShapeEdge, BottomLeft, pi);

    // Create the flat buttons
    // ArrowNorth/South: tilt up/down (rotate around X axis)
    // ArrowEast/West: turn left/right (rotate around Z axis for yaw)
    // ArrowLeft/Right: roll counter-clockwise/clockwise (rotate around Y axis for roll)
    addButtonFace(ArrowNorth, osg::Vec3f(1, 0, 0));    // Swapped: was (-1, 0, 0)
    addButtonFace(ArrowSouth, osg::Vec3f(-1, 0, 0));   // Swapped: was (1, 0, 0)
    addButtonFace(ArrowEast, osg::Vec3f(0, 0, -1));    // Restored
    addButtonFace(ArrowWest, osg::Vec3f(0, 0, 1));     // Restored
    addButtonFace(ArrowLeft, osg::Vec3f(0, 1, 0));     // Swapped with Right
    addButtonFace(ArrowRight, osg::Vec3f(0, -1, 0));   // Swapped with Left
    addButtonFace(DotBackside, osg::Vec3f(0, 1, 0));
    addButtonFace(ViewMenu);

    ensurePickingFramebuffer();
}

void OsgVerseNaviCube::addCubeFace(const osg::Vec3f& x, const osg::Vec3f& z,
                                    ShapeType shapeType, PickId pickId, float rotZ)
{
    Face& face = _faces[pickId];
    face.vertices.clear();
    face.type = shapeType;

    osg::Vec3f y = x ^ (-z);  // Cross product

    // Create normalized vectors
    osg::Vec3f xN = x; xN.normalize();
    osg::Vec3f yN = y; yN.normalize();
    osg::Vec3f zN = z; zN.normalize();

    // Store camera vectors directly (matching Coin3D's convention)
    // In Coin3D, the matrix R has columns xN, yN, zN
    // R transforms from camera space to world space:
    // - Camera X (right) -> World xN
    // - Camera Y (up) -> World yN
    // - Camera Z (back) -> World zN
    // So camera forward (-Z) -> World -zN
    face.forward = -zN;  // Camera forward direction in world space
    face.up = yN;        // Camera up direction in world space

    // Apply rotZ rotation around the forward axis if needed
    if (std::abs(rotZ) > 0.001f) {
        osg::Matrixd rotMatrix = osg::Matrixd::rotate(rotZ, osg::Vec3d(face.forward.x(), face.forward.y(), face.forward.z()));
        osg::Vec3d upD(face.up.x(), face.up.y(), face.up.z());
        upD = rotMatrix.preMult(upD);
        face.up = osg::Vec3f(upD.x(), upD.y(), upD.z());
        face.up.normalize();
    }

    // Also store quaternion for compatibility with getNearestOrientation
    osg::Matrix rotMatrix(
        xN.x(), xN.y(), xN.z(), 0,
        yN.x(), yN.y(), yN.z(), 0,
        zN.x(), zN.y(), zN.z(), 0,
        0, 0, 0, 1
    );
    osg::Quat baseRotation;
    baseRotation.set(rotMatrix);
    osg::Quat zRotation(rotZ, osg::Vec3f(0, 0, 1));
    face.rotation = (baseRotation * zRotation).inverse();

    if (shapeType == ShapeCorner) {
        auto xC = x * _chamfer;
        auto yC = y * _chamfer;
        auto zC = z * (1.0f - 2.0f * _chamfer);

        face.vertices.reserve(6);
        face.vertices.push_back(zC - xC * 2.0f);
        face.vertices.push_back(zC - xC - yC);
        face.vertices.push_back(zC + xC - yC);
        face.vertices.push_back(zC + xC * 2.0f);
        face.vertices.push_back(zC + xC + yC);
        face.vertices.push_back(zC - xC + yC);
    }
    else if (shapeType == ShapeEdge) {
        auto x4 = x * (1.0f - _chamfer * 4.0f);
        auto yE = y * _chamfer;
        auto zE = z * (1.0f - _chamfer);

        face.vertices.reserve(4);
        face.vertices.push_back(zE - x4 - yE);
        face.vertices.push_back(zE + x4 - yE);
        face.vertices.push_back(zE + x4 + yE);
        face.vertices.push_back(zE - x4 + yE);
    }
    else if (shapeType == ShapeMain) {
        auto x2 = x * (1.0f - _chamfer * 2.0f);
        auto y2 = y * (1.0f - _chamfer * 2.0f);
        auto x4 = x * (1.0f - _chamfer * 4.0f);
        auto y4 = y * (1.0f - _chamfer * 4.0f);

        // Main face is an octagon
        face.vertices.reserve(8);
        face.vertices.push_back(z - x2 - y4);
        face.vertices.push_back(z - x4 - y2);
        face.vertices.push_back(z + x4 - y2);
        face.vertices.push_back(z + x2 - y4);
        face.vertices.push_back(z + x2 + y4);
        face.vertices.push_back(z + x4 + y2);
        face.vertices.push_back(z - x4 + y2);
        face.vertices.push_back(z - x2 + y4);

        // Store label vertices
        LabelTexture& label = _labelTextures[pickId];
        label.vertices.clear();
        label.vertices.push_back(z - x2 - y2);
        label.vertices.push_back(z + x2 - y2);
        label.vertices.push_back(z + x2 + y2);
        label.vertices.push_back(z - x2 + y2);
    }
}

void OsgVerseNaviCube::addButtonFace(PickId pickId, const osg::Vec3f& direction)
{
    Face& face = _faces[pickId];
    face.vertices.clear();
    face.type = ShapeButton;

    float scale = 0.005f;
    float offx = 0.5f;
    float offy = 0.5f;
    std::vector<float> pointData;

    switch (pickId) {
        case ArrowRight:
        case ArrowLeft: {
            pointData = {66.6f,  -66.6f,
                         58.3f,  -74.0f, 49.2f,  -80.3f, 39.4f,
                         -85.5f, 29.0f,  -89.5f, 25.3f,  -78.1f,
                         34.3f,  -74.3f, 42.8f,  -69.9f, 50.8f,
                         -64.4f, 58.1f,  -58.1f, 53.8f,  -53.8f,
                         74.7f,  -46.8f, 70.7f,  -70.4f};
            break;
        }
        case ArrowWest:
        case ArrowNorth:
        case ArrowSouth:
        case ArrowEast: {
            pointData = {100.f, 0.f, 80.f, -18.f, 80.f, 18.f};
            break;
        }
        case ViewMenu: {
            offx = 0.84f;
            offy = 0.84f;
            pointData = {0.f, 0.f,
                         15.f, -6.f, 0.f, -12.f, -15.f, -6.f, 0.f, 0.f,
                         -15.f, -6.f, -15.f, 12.f, 0.f, 18.f, 0.f, 0.f,
                         0.f, 18.f, 15.f, 12.f, 15.f, -6.f};
            break;
        }
        case DotBackside: {
            int steps = 16;
            for (int i = 0; i < steps; i++) {
                float angle = 2.0f * std::numbers::pi_v<float> * (static_cast<float>(i) + 0.5f) / static_cast<float>(steps);
                pointData.push_back(10.f * std::cos(angle) + 87.f);
                pointData.push_back(10.f * std::sin(angle) - 87.f);
            }
            break;
        }
        default:
            break;
    }

    int count = static_cast<int>(pointData.size()) / 2;
    face.vertices.reserve(count);

    for (int i = 0; i < count; i++) {
        float px = pointData[i * 2] * scale + offx;
        float py = pointData[i * 2 + 1] * scale + offy;

        if (pickId == ArrowNorth || pickId == ArrowWest || pickId == ArrowLeft) {
            px = 1.0f - px;
        }

        if (pickId == ArrowSouth || pickId == ArrowNorth) {
            face.vertices.push_back(osg::Vec3f(py, px, 0.0f));
        } else {
            face.vertices.push_back(osg::Vec3f(px, py, 0.0f));
        }
    }

    // Store rotation based on direction
    // Matches Coin3D: m_Faces[pickId].rotation = SbRotation(direction, 1).inverse();
    // SbRotation(axis, angle) creates rotation around axis by angle radians
    // .inverse() negates the angle, so final angle is -1 radian
    if (direction.length() > 0.001f) {
        osg::Vec3f axis = direction;
        axis.normalize();
        // Create rotation: angle = -1 radian (because of inverse)
        face.rotation = osg::Quat(-1.0, axis);
        face.direction = axis;
    }
}

// Helper to compute vertical balance for text centering
static int imageVerticalBalance(const QImage& p, int sizeHint)
{
    if (sizeHint < 0) {
        return 0;
    }

    int h = p.height();
    int startRow = (h - sizeHint) / 2;
    bool done = false;
    int x, bottom, top;
    for (top = startRow; top < h; top++) {
        for (x = 0; x < p.width(); x++) {
            if (qAlpha(p.pixel(x, top))) {
                done = true;
                break;
            }
        }
        if (done) {
            break;
        }
    }
    for (bottom = startRow; bottom < h; bottom++) {
        for (x = 0; x < p.width(); x++) {
            if (qAlpha(p.pixel(x, h - 1 - bottom))) {
                return (bottom - top) / 2;
            }
        }
    }
    return 0;
}

void OsgVerseNaviCube::createCubeFaceTextures()
{
    int texSize = 192;  // Works well for the max cube size 1024
    QFont font;
    font.fromString(QStringLiteral("Arial"));
    font.setStyleHint(QFont::SansSerif);
    font.setPointSizeF(texSize);
    QFontMetrics fm(font);

    qreal minFontSize = texSize;
    qreal maxFontSize = 0.0;

    std::vector<PickId> mains = {Front, Top, Right, Rear, Bottom, Left};
    std::vector<std::string> defaultLabels = {"FRONT", "TOP", "RIGHT", "REAR", "BOTTOM", "LEFT"};

    // Initialize labels if not set
    for (size_t i = 0; i < mains.size(); ++i) {
        if (_labelTextures[mains[i]].label.empty()) {
            _labelTextures[mains[i]].label = defaultLabels[i];
        }
    }

    // Calculate font sizes
    for (PickId pickId : mains) {
        auto t = QString::fromUtf8(_labelTextures[pickId].label.c_str());
        QRect br = fm.boundingRect(t);
        float scale = static_cast<float>(texSize) / std::max(br.width(), br.height());
        _labelTextures[pickId].fontSize = texSize * scale;
        minFontSize = std::min(minFontSize, _labelTextures[pickId].fontSize);
        maxFontSize = std::max(maxFontSize, _labelTextures[pickId].fontSize);
    }

    if (_fontZoom > 0.0) {
        maxFontSize = minFontSize + (maxFontSize - minFontSize) * _fontZoom;
    } else {
        maxFontSize = minFontSize * std::pow(2.0, _fontZoom);
    }

    // Create textures for each face
    for (PickId pickId : mains) {
        QImage image(texSize, texSize, QImage::Format_ARGB32);
        image.fill(qRgba(255, 255, 255, 0));

        if (_labelTextures[pickId].fontSize > 0.5) {
            // 5% margin looks nice and prevents some artifacts
            font.setPointSizeF(std::min(_labelTextures[pickId].fontSize, maxFontSize) * 0.9);
            QPainter paint;
            paint.begin(&image);
            paint.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
            paint.setPen(Qt::white);
            QString text = QString::fromUtf8(_labelTextures[pickId].label.c_str());
            paint.setFont(font);
            paint.drawText(QRect(0, 0, texSize, texSize), Qt::AlignCenter, text);
            int offset = imageVerticalBalance(image, font.pointSize());
            image.fill(qRgba(255, 255, 255, 0));
            paint.drawText(QRect(0, offset, texSize, texSize), Qt::AlignCenter, text);
            paint.end();
        }

        // Delete old texture if exists
        if (_labelTextures[pickId].texture) {
            delete _labelTextures[pickId].texture;
        }

        _labelTextures[pickId].texture = new QOpenGLTexture(image.mirrored());
        _labelTextures[pickId].texture->setMaximumAnisotropy(4.0);
        _labelTextures[pickId].texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        _labelTextures[pickId].texture->setMagnificationFilter(QOpenGLTexture::Linear);
        _labelTextures[pickId].texture->generateMipMaps();
    }
}

//===========================================================================
// Drawing
//===========================================================================

void OsgVerseNaviCube::drawNaviCube(bool pickMode, float opacity)
{
    if (!_viewer) {
        return;
    }

    // Store GL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // Set viewport for NaviCube (must be after glPushAttrib)
    if (!pickMode) {
        // Ensure we have valid dimensions
        if (_viewWidth <= 0 || _viewHeight <= 0) {
            glPopAttrib();
            return;
        }

        float physicalSize = _cubeSize * _devicePixelRatio;

        // Calculate physical viewport dimensions
        float physicalViewWidth = _viewWidth * _devicePixelRatio;
        float physicalViewHeight = _viewHeight * _devicePixelRatio;

        // Calculate NaviCube center position in physical coordinates
        // Use margin-based positioning for reliability
        float margin = physicalSize * 0.55f;  // Margin from edge

        // Calculate available area for positioning (in physical pixels)
        float availableWidth = physicalViewWidth - 2 * margin;
        float availableHeight = physicalViewHeight - 2 * margin;

        // Ensure available area is at least 1 pixel
        availableWidth = std::max(1.0f, availableWidth);
        availableHeight = std::max(1.0f, availableHeight);

        // Calculate center position based on relative position
        float centerX = margin + _relPosX * availableWidth;
        float centerY = margin + _relPosY * availableHeight;

        // Calculate viewport position (bottom-left corner)
        int posX = static_cast<int>(centerX - physicalSize / 2);
        int posY = static_cast<int>(centerY - physicalSize / 2);

        glViewport(posX, posY, static_cast<int>(physicalSize), static_cast<int>(physicalSize));
    }

    // Configure
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthRange(0.f, 1.f);
    glClearDepth(1.f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_LIGHTING);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (pickMode) {
        glDisable(GL_BLEND);
        glShadeModel(GL_FLAT);
        glDisable(GL_DITHER);
        glDisable(GL_POLYGON_SMOOTH);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glEnable(GL_BLEND);
        glShadeModel(GL_SMOOTH);
    }

    // Mimic 3D view projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    const float NEARVAL = 0.1f;
    const float FARVAL = 10.1f;

    // Use orthographic projection (like SoOrthographicCamera)
    glOrtho(-2.1, 2.1, -2.1, 2.1, NEARVAL, FARVAL);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Get camera orientation and create view matrix
    auto params = _viewer->getCamera();
    osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
    osg::Vec3d center(params.target.x, params.target.y, params.target.z);
    osg::Vec3d up(params.upVector.x, params.upVector.y, params.upVector.z);

    // Calculate view direction
    osg::Vec3d viewDir = center - eye;
    viewDir.normalize();

    // Calculate right vector
    osg::Vec3d right = viewDir ^ up;
    right.normalize();

    // Recalculate up to ensure orthogonality
    up = right ^ viewDir;
    up.normalize();

    // Create view rotation matrix (inverse of camera orientation)
    float mx[16] = {
        static_cast<float>(right.x()), static_cast<float>(up.x()), static_cast<float>(-viewDir.x()), 0,
        static_cast<float>(right.y()), static_cast<float>(up.y()), static_cast<float>(-viewDir.y()), 0,
        static_cast<float>(right.z()), static_cast<float>(up.z()), static_cast<float>(-viewDir.z()), 0,
        0, 0, -5.1f, 1
    };
    glLoadMatrixf(mx);

    glEnableClientState(GL_VERTEX_ARRAY);
    QColor& cb = _emphasisColor;

    // Draw coordinate system
    if (!pickMode && _showCS) {
        glLineWidth(_borderWidth * 2.f);
        glPointSize(_borderWidth * 2.f);
        float a = -1.1f;
        float b = -1.05f;
        float c = 0.5f;

        float pointData[] = {
            b, a, a,  // X1
            c, a, a,  // X2
            a, b, a,  // Y1
            a, c, a,  // Y2
            a, a, b,  // Z1
            a, a, c,  // Z2
            a, a, a   // 0
        };
        glVertexPointer(3, GL_FLOAT, 0, pointData);

        glColor4f(_xColor.r(), _xColor.g(), _xColor.b(), opacity);
        glDrawArrays(GL_LINES, 0, 2);
        glDrawArrays(GL_POINTS, 0, 2);

        glColor4f(_yColor.r(), _yColor.g(), _yColor.b(), opacity);
        glDrawArrays(GL_LINES, 2, 2);
        glDrawArrays(GL_POINTS, 2, 2);

        glColor4f(_zColor.r(), _zColor.g(), _zColor.b(), opacity);
        glDrawArrays(GL_LINES, 4, 2);
        glDrawArrays(GL_POINTS, 4, 2);
    }

    // Cube faces
    for (const auto& pair : _faces) {
        const Face& f = pair.second;
        if (f.type == ShapeButton) {
            continue;
        }
        PickId pickId = pair.first;
        if (pickMode) {
            // Use glColor4ub with alpha=255 for picking detection
            glColor4ub(static_cast<GLubyte>(pickId), 0, 0, 255);
        } else {
            QColor& c = (_hiliteId == pickId) ? _hiliteColor : _baseColor;
            glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF() * opacity);
        }
        glVertexPointer(3, GL_FLOAT, 0, f.vertices.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, f.vertices.size());
    }

    if (!pickMode) {
        // Cube borders
        glLineWidth(_borderWidth);
        for (const auto& pair : _faces) {
            const Face& f = pair.second;
            if (f.type == ShapeButton) {
                continue;
            }
            glColor4f(cb.redF(), cb.greenF(), cb.blueF(), cb.alphaF() * opacity);
            glVertexPointer(3, GL_FLOAT, 0, f.vertices.data());
            glDrawArrays(GL_LINE_LOOP, 0, f.vertices.size());
        }

        // Label textures
        glDisable(GL_POLYGON_OFFSET_FILL);  // Make sure labels are on top
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        float texCoords[] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
        QColor& c = _emphasisColor;
        glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF() * opacity);

        for (const auto& pair : _labelTextures) {
            const LabelTexture& lt = pair.second;
            if (!lt.texture || lt.vertices.empty()) {
                continue;
            }
            glVertexPointer(3, GL_FLOAT, 0, lt.vertices.data());
            glBindTexture(GL_TEXTURE_2D, lt.texture->textureId());
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_POLYGON_OFFSET_FILL);
    }

    // Draw the flat buttons
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (const auto& pair : _faces) {
        const Face& f = pair.second;
        if (f.type != ShapeButton) {
            continue;
        }
        PickId pickId = pair.first;
        if (pickMode) {
            // Use glColor4ub with alpha=255 for picking detection
            glColor4ub(static_cast<GLubyte>(pickId), 0, 0, 255);
        } else {
            QColor& c = (_hiliteId == pickId) ? _hiliteColor : _baseColor;
            glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF() * opacity);
        }
        glVertexPointer(3, GL_FLOAT, 0, f.vertices.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, f.vertices.size());
        if (!pickMode) {
            glColor4f(cb.redF(), cb.greenF(), cb.blueF(), cb.alphaF() * opacity);
            glDrawArrays(GL_LINE_LOOP, 0, f.vertices.size());
        }
    }

    // Restore original state
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

//===========================================================================
// Picking
//===========================================================================

OsgVerseNaviCube::PickId OsgVerseNaviCube::pickFace(int x, int y)
{
    float physicalSize = _cubeSize * _devicePixelRatio;
    float logicalSize = static_cast<float>(_cubeSize);
    GLubyte pixels[4] = {0};

    ensurePickingFramebuffer();

    // Use logical size for boundary check (x, y are in logical coordinates from translateToLocal)
    if (_pickingFramebuffer && std::abs(x) <= logicalSize / 2 && std::abs(y) <= logicalSize / 2) {
        QWidget* widget = _viewer->getWidget();
        if (auto* glWidget = qobject_cast<QOpenGLWidget*>(widget)) {
            glWidget->makeCurrent();
        }

        _pickingFramebuffer->bind();

        glViewport(0, 0, static_cast<int>(physicalSize * 2), static_cast<int>(physicalSize * 2));

        drawNaviCube(true, 1.f);

        glFinish();
        // Convert logical coordinates to physical coordinates for pixel reading
        // x, y are in logical coordinates relative to NaviCube center
        // FBO has NaviCube centered, so we need to scale and offset
        int physicalX = static_cast<int>(x * _devicePixelRatio * 2 + physicalSize);
        int physicalY = static_cast<int>(y * _devicePixelRatio * 2 + physicalSize);
        glReadPixels(
            physicalX,
            physicalY,
            1,
            1,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            &pixels
        );

        _pickingFramebuffer->release();

        if (auto* glWidget = qobject_cast<QOpenGLWidget*>(widget)) {
            glWidget->doneCurrent();
        }
    }

    return pixels[3] == 255 ? static_cast<PickId>(pixels[0]) : None;
}

void OsgVerseNaviCube::ensurePickingFramebuffer()
{
    float physicalSize = _cubeSize * _devicePixelRatio;
    int fbSize = static_cast<int>(physicalSize * 2);

    if (!_pickingFramebuffer || !_pickingFramebuffer->isValid() ||
        _pickingFramebuffer->width() != fbSize) {
        if (_pickingFramebuffer) {
            delete _pickingFramebuffer;
            _pickingFramebuffer = nullptr;
        }
        _pickingFramebuffer = new QOpenGLFramebufferObject(
            fbSize, fbSize, QOpenGLFramebufferObject::CombinedDepthStencil);
    }
}

void OsgVerseNaviCube::drawForPicking()
{
    drawNaviCube(true, 1.0f);
}

//===========================================================================
// View Control
//===========================================================================

// Helper: Get camera orientation quaternion from view vectors
// Returns world-to-camera rotation (like Coin3D's cam->orientation)
static osg::Quat getCameraQuatFromParams(const Gui::View3D::CameraParams& params)
{
    osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
    osg::Vec3d center(params.target.x, params.target.y, params.target.z);
    osg::Vec3d up(params.upVector.x, params.upVector.y, params.upVector.z);

    osg::Vec3d viewDir = center - eye;
    viewDir.normalize();

    osg::Vec3d right = viewDir ^ up;
    right.normalize();

    up = right ^ viewDir;
    up.normalize();

    // Create rotation matrix: camera-to-world transformation
    // Camera axes in world space: right (X), up (Y), -viewDir (Z/back)
    // OSG uses row vectors, so rows are the basis vectors
    osg::Matrix camToWorld(
        right.x(), right.y(), right.z(), 0,
        up.x(), up.y(), up.z(), 0,
        -viewDir.x(), -viewDir.y(), -viewDir.z(), 0,
        0, 0, 0, 1
    );

    osg::Quat camToWorldQuat;
    camToWorldQuat.set(camToWorld);

    // Return world-to-camera (inverse) to match Coin3D's cam->orientation convention
    return camToWorldQuat.inverse();
}

// Helper: Set camera params from quaternion orientation
// The quaternion represents the world-to-camera rotation (like Coin3D's cam->orientation)
// To get camera vectors in world space, we need to use the inverse (camera-to-world)
static void setCameraParamsFromQuat(Gui::View3D::CameraParams& params, const osg::Quat& quat)
{
    osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
    osg::Vec3d center(params.target.x, params.target.y, params.target.z);
    double distance = (eye - center).length();

    // Ensure minimum distance to avoid numerical issues
    if (distance < 0.1) {
        distance = 10.0;  // Default reasonable distance
    }

    // quat is world-to-camera, so we need inverse (camera-to-world) to transform camera vectors to world
    osg::Quat camToWorld = quat.inverse();

    // Default forward is -Z (0, 0, -1) in OpenGL convention
    // Transform from camera space to world space
    osg::Vec3d forward = camToWorld * osg::Vec3d(0, 0, -1);
    forward.normalize();

    osg::Vec3d newUp = camToWorld * osg::Vec3d(0, 1, 0);
    newUp.normalize();

    osg::Vec3d newEye = center - forward * distance;

    params.position.x = newEye.x();
    params.position.y = newEye.y();
    params.position.z = newEye.z();
    params.upVector.x = newUp.x();
    params.upVector.y = newUp.y();
    params.upVector.z = newUp.z();
}

void OsgVerseNaviCube::setCameraOrientation(PickId face)
{
    if (face == None || face >= PickIdCount) {
        return;
    }

    auto it = _faces.find(face);
    if (it == _faces.end()) {
        return;
    }

    const Face& f = it->second;

    if (f.type == ShapeMain || f.type == ShapeEdge || f.type == ShapeCorner) {
        // Use stored forward and up vectors directly
        if (!_viewer) return;

        auto params = _viewer->getCamera();
        osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
        osg::Vec3d center(params.target.x, params.target.y, params.target.z);

        // Calculate distance (preserve it)
        double distance = (eye - center).length();
        if (distance < 0.1) {
            distance = 10.0;  // Default reasonable distance
        }

        // Use the stored forward and up vectors
        osg::Vec3d forward(f.forward.x(), f.forward.y(), f.forward.z());
        osg::Vec3d up(f.up.x(), f.up.y(), f.up.z());

        // Calculate new eye position: eye = center - forward * distance
        osg::Vec3d newEye = center - forward * distance;

        // Update camera parameters
        params.position.x = newEye.x();
        params.position.y = newEye.y();
        params.position.z = newEye.z();
        params.upVector.x = up.x();
        params.upVector.y = up.y();
        params.upVector.z = up.z();

        _viewer->setCamera(params);
        return;
    } else if (f.type == ShapeButton) {
        // Handle button rotations by transforming world axes to camera-relative axes
        // This makes arrow buttons behave like mouse drag (camera-relative rotation)

        if (!_viewer) return;

        // Get rotation step angle
        long step = std::clamp(static_cast<long>(_naviStepByTurn), 4L, 36L);
        float rotStepAngle = (2.0f * std::numbers::pi_v<float>) / static_cast<float>(step);

        // Determine the rotation angle
        double angle;
        if (face == DotBackside) {
            angle = std::numbers::pi_v<double>;  // 180 degrees
        } else {
            angle = static_cast<double>(rotStepAngle);
        }

        // Get current camera parameters
        auto params = _viewer->getCamera();
        osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
        osg::Vec3d center(params.target.x, params.target.y, params.target.z);
        osg::Vec3d up(params.upVector.x, params.upVector.y, params.upVector.z);

        // Calculate camera coordinate system
        osg::Vec3d viewDir = center - eye;  // Forward direction (from eye to center)
        viewDir.normalize();
        
        osg::Vec3d right = viewDir ^ up;  // Right direction
        right.normalize();
        
        osg::Vec3d camUp = right ^ viewDir;  // Recalculate up to ensure orthogonality
        camUp.normalize();

        // Transform world axis to camera space
        // Arrow directions are defined in world space, we need to interpret them relative to camera
        osg::Vec3d worldAxis(f.direction.x(), f.direction.y(), f.direction.z());
        worldAxis.normalize();

        // Map world axis to camera axis
        // X-axis (1,0,0) -> right axis (for pitch/tilt up-down)
        // Y-axis (0,1,0) -> viewDir axis (for roll left-right)
        // Z-axis (0,0,1) -> camUp axis (for yaw east-west)
        osg::Vec3d cameraAxis = right * worldAxis.x() + viewDir * worldAxis.y() + camUp * worldAxis.z();
        cameraAxis.normalize();

        // Calculate distance (preserve it)
        double distance = (eye - center).length();
        if (distance < 0.1) {
            distance = 10.0;  // Default reasonable distance
        }

        // Create rotation matrix around the camera-relative axis
        osg::Matrixd rotMatrix = osg::Matrixd::rotate(angle, cameraAxis);

        // Apply rotation to the view direction and up vector
        osg::Vec3d newViewDir = rotMatrix.preMult(viewDir);
        newViewDir.normalize();
        osg::Vec3d newUp = rotMatrix.preMult(camUp);
        newUp.normalize();

        // Calculate new eye position
        osg::Vec3d newEye = center - newViewDir * distance;

        // Start animation instead of directly setting camera
        startCameraAnimation(newEye, newUp, center);
        return;
    }
}

osg::Quat OsgVerseNaviCube::getFaceRotation(PickId face) const
{
    auto it = _faces.find(face);
    if (it != _faces.end()) {
        return it->second.rotation;
    }
    return osg::Quat();
}

osg::Quat OsgVerseNaviCube::getNearestOrientation(PickId pickId) const
{
    auto it = _faces.find(pickId);
    if (it == _faces.end()) {
        return osg::Quat();
    }

    const Face& face = it->second;
    osg::Quat standardOrientation = face.rotation;

    // Get current camera orientation
    auto params = _viewer->getCamera();
    osg::Quat cameraOrientation = getCameraQuatFromParams(params);

    // Get camera Z axis
    osg::Vec3f cameraZ = cameraOrientation * osg::Vec3f(0, 0, 1);

    // Get standard Z axis for this face
    osg::Vec3f standardZ = standardOrientation * osg::Vec3f(0, 0, 1);

    // Cleanup near-zero values
    for (int i = 0; i < 3; i++) {
        if (std::abs(standardZ[i]) < 1e-6f) {
            standardZ[i] = 0.0f;
        }
    }
    standardZ.normalize();

    // Calculate intermediate orientation
    osg::Quat alignRotation;
    alignRotation.makeRotate(cameraZ, standardZ);
    osg::Quat intermediateOrientation = cameraOrientation * alignRotation;

    // Find angle between intermediate and standard
    osg::Quat rotation = intermediateOrientation.inverse() * standardOrientation;
    osg::Vec3d axis;
    double angle;
    rotation.getRotate(angle, axis);

    // Align axis with standardZ
    if (osg::Vec3f(axis.x(), axis.y(), axis.z()) * standardZ < 0) {
        axis = -axis;
        angle = -angle;
    }

    // Make angle positive
    constexpr float pi = std::numbers::pi_v<float>;
    while (angle < 0) {
        angle += 2.0 * pi;
    }

    // Small value for orientation priority control
    float f = 0.00001f;

    // Snap to nearest standard orientation
    if (face.type == ShapeCorner) {
        // 6 possible orientations for corners
        if (angle <= (pi / 6 + f)) {
            angle = 0;
        } else if (angle <= (pi / 2 + f)) {
            angle = pi / 3;
        } else if (angle < (5 * pi / 6 - f)) {
            angle = 2 * pi / 3;
        } else if (angle <= (pi + pi / 6 + f)) {
            angle = pi;
        } else if (angle < (pi + pi / 2 - f)) {
            angle = pi + pi / 3;
        } else if (angle < (pi + 5 * pi / 6 - f)) {
            angle = pi + 2 * pi / 3;
        } else {
            angle = 0;
        }
    } else {
        // 4 possible orientations for main and edge faces
        if (angle <= (pi / 4 + f)) {
            angle = 0;
        } else if (angle <= (3 * pi / 4 + f)) {
            angle = pi / 2;
        } else if (angle < (pi + pi / 4 - f)) {
            angle = pi;
        } else if (angle < (pi + 3 * pi / 4 - f)) {
            angle = pi + pi / 2;
        } else {
            angle = 0;
        }
    }

    // Create final rotation
    osg::Quat snapRotation(angle, osg::Vec3d(standardZ.x(), standardZ.y(), standardZ.z()));
    return standardOrientation * snapRotation.inverse();
}

//===========================================================================
// Event Handling
//===========================================================================

bool OsgVerseNaviCube::mousePressed(int x, int y)
{
    _mouseDown = true;
    _mightDrag = inDragZone(x, y);

    PickId pick = pickFace(x, y);
    setHighlight(pick);

    return pick != None;
}

bool OsgVerseNaviCube::mouseReleased(int x, int y)
{
    setHighlight(None);
    _mouseDown = false;

    if (_dragging) {
        _dragging = false;
        return true;
    }

    PickId pickId = pickFace(x, y);

    if (pickId != None) {
        // Handle view menu
        if (pickId == ViewMenu) {
            // TODO: Show context menu
            return true;
        }

        // Set camera orientation
        setCameraOrientation(pickId);
        return true;
    }

    return false;
}

bool OsgVerseNaviCube::mouseMoved(int x, int y)
{
    // Use logical size for hover detection (consistent with Qt events)
    float logicalSize = static_cast<float>(_cubeSize);
    bool hovering = std::abs(x) <= logicalSize / 2 && std::abs(y) <= logicalSize / 2;

    if (hovering != _hovering) {
        _hovering = hovering;
        scheduleRedraw();
    }

    if (!_dragging) {
        setHighlight(pickFace(x, y));
    }

    if (_mouseDown && _draggable) {
        if (_mightDrag && !_dragging) {
            _dragging = true;
            setHighlight(None);
        }

        if (_dragging && (std::abs(x) > 0 || std::abs(y) > 0)) {
            // Calculate available area for positioning
            float logicalSize = static_cast<float>(_cubeSize);
            float margin = logicalSize * 0.55f;
            float availableWidth = std::max(1.0f, _viewWidth - 2 * margin);
            float availableHeight = std::max(1.0f, _viewHeight - 2 * margin);

            float newX = _relPosX + static_cast<float>(x) / availableWidth;
            float newY = _relPosY + static_cast<float>(y) / availableHeight;
            _relPosX = std::clamp(newX, 0.0f, 1.0f);
            _relPosY = std::clamp(newY, 0.0f, 1.0f);
            scheduleRedraw();
            return true;
        }
    }

    return false;
}

bool OsgVerseNaviCube::inDragZone(int x, int y) const
{
    // Use logical size (x, y are in logical coordinates from translateToLocal)
    float logicalSize = static_cast<float>(_cubeSize);
    int limit = static_cast<int>(logicalSize / 4);
    return std::abs(x) < limit && std::abs(y) < limit;
}

QPoint OsgVerseNaviCube::translateToLocal(const QPoint& screenPos) const
{
    // Calculate NaviCube center position in logical coordinates
    // This must match the calculation in drawNaviCube()
    float logicalSize = static_cast<float>(_cubeSize);
    float margin = logicalSize * 0.55f;  // Margin from edge

    // Calculate available area for positioning (in logical pixels)
    float availableWidth = _viewWidth - 2 * margin;
    float availableHeight = _viewHeight - 2 * margin;

    // Ensure available area is at least 1 pixel
    availableWidth = std::max(1.0f, availableWidth);
    availableHeight = std::max(1.0f, availableHeight);

    // Calculate center position based on relative position
    float centerX = margin + _relPosX * availableWidth;
    float centerY = margin + _relPosY * availableHeight;

    // Convert from Qt coordinate system (Y down) to OpenGL (Y up)
    int localX = screenPos.x() - static_cast<int>(centerX);
    int localY = (_viewHeight - screenPos.y()) - static_cast<int>(centerY);

    return QPoint(localX, localY);
}

void OsgVerseNaviCube::setHighlight(PickId id)
{
    if (_hiliteId != id) {
        _hiliteId = id;
        scheduleRedraw();
    }
}

void OsgVerseNaviCube::handleResize()
{
    // Get device pixel ratio from widget
    _devicePixelRatio = 1.0f;
    if (_viewer) {
        QWidget* widget = _viewer->getWidget();
        if (widget) {
            _devicePixelRatio = widget->devicePixelRatioF();

            int newWidth = widget->width();
            int newHeight = widget->height();

            if (newWidth != _viewWidth || newHeight != _viewHeight) {
                _viewWidth = newWidth;
                _viewHeight = newHeight;
            }
        }
    }

    if (_viewWidth <= 0 || _viewHeight <= 0) {
        return;
    }

    // Use logical size for position calculations (consistent with Qt events)
    float logicalSize = static_cast<float>(_cubeSize);

    // Calculate position area (margin from edges) using logical coordinates
    _posAreaBaseX = std::min(static_cast<int>(logicalSize * 0.55f), _viewWidth / 2);
    _posAreaBaseY = std::min(static_cast<int>(logicalSize * 0.55f), _viewHeight / 2);
    _posAreaSizeX = _viewWidth - 2 * _posAreaBaseX;
    _posAreaSizeY = _viewHeight - 2 * _posAreaBaseY;
}

void OsgVerseNaviCube::scheduleRedraw()
{
    if (_viewer) {
        QWidget* widget = _viewer->getWidget();
        if (widget) {
            widget->update();
        }
    }
}

void OsgVerseNaviCube::updateFaceColors()
{
    // Colors are updated in drawNaviCube() directly based on highlight state
}

//===========================================================================
// Camera Animation
//===========================================================================

void OsgVerseNaviCube::startCameraAnimation(const osg::Vec3d& targetEye,
                                             const osg::Vec3d& targetUp,
                                             const osg::Vec3d& center)
{
    if (!_viewer) return;

    // If animation is already running, stop it
    if (_cameraAnimation.active) {
        stopCameraAnimation();
    }

    // Get current camera state
    auto params = _viewer->getCamera();
    _cameraAnimation.startEye = osg::Vec3d(
        params.position.x, params.position.y, params.position.z
    );
    _cameraAnimation.startUp = osg::Vec3d(
        params.upVector.x, params.upVector.y, params.upVector.z
    );

    // Set target state
    _cameraAnimation.targetEye = targetEye;
    _cameraAnimation.targetUp = targetUp;
    _cameraAnimation.center = center;

    // Set timing
    _cameraAnimation.startTime = QDateTime::currentMSecsSinceEpoch();
    _cameraAnimation.duration = DEFAULT_ANIMATION_DURATION;
    _cameraAnimation.active = true;

    // Start timer
    _animationTimer->start();
}

void OsgVerseNaviCube::stopCameraAnimation()
{
    _cameraAnimation.active = false;
    _animationTimer->stop();
}

void OsgVerseNaviCube::updateCameraAnimation()
{
    if (!_cameraAnimation.active || !_viewer) {
        _animationTimer->stop();
        return;
    }

    // Calculate progress
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    double elapsed = currentTime - _cameraAnimation.startTime;
    double t = elapsed / _cameraAnimation.duration;

    if (t >= 1.0) {
        // Animation complete, set final position
        t = 1.0;
        _cameraAnimation.active = false;
        _animationTimer->stop();
    }

    // Apply easing function
    double easedT = easeInOutCubic(t);

    // Interpolate
    osg::Vec3d currentEye = lerp(
        _cameraAnimation.startEye,
        _cameraAnimation.targetEye,
        easedT
    );

    osg::Vec3d currentUp = slerp(
        _cameraAnimation.startUp,
        _cameraAnimation.targetUp,
        easedT
    );

    // Update camera
    auto params = _viewer->getCamera();
    params.position.x = currentEye.x();
    params.position.y = currentEye.y();
    params.position.z = currentEye.z();
    params.upVector.x = currentUp.x();
    params.upVector.y = currentUp.y();
    params.upVector.z = currentUp.z();

    _viewer->setCamera(params);

    // Trigger redraw
    scheduleRedraw();
}

double OsgVerseNaviCube::easeInOutCubic(double t)
{
    if (t < 0.5) {
        return 4.0 * t * t * t;
    } else {
        double f = 2.0 * t - 2.0;
        return 0.5 * f * f * f + 1.0;
    }
}

osg::Vec3d OsgVerseNaviCube::lerp(const osg::Vec3d& a, const osg::Vec3d& b, double t)
{
    return a * (1.0 - t) + b * t;
}

osg::Vec3d OsgVerseNaviCube::slerp(const osg::Vec3d& a, const osg::Vec3d& b, double t)
{
    // Normalize vectors
    osg::Vec3d v0 = a;
    osg::Vec3d v1 = b;
    v0.normalize();
    v1.normalize();

    // Calculate dot product
    double dot = v0 * v1;

    // If vectors are nearly parallel, use linear interpolation
    if (dot > 0.9995) {
        return lerp(a, b, t);
    }

    // Clamp dot to [-1, 1]
    dot = std::clamp(dot, -1.0, 1.0);

    // Calculate angle
    double theta = std::acos(dot) * t;

    // Calculate orthogonal vector
    osg::Vec3d v2 = v1 - v0 * dot;
    v2.normalize();

    // Spherical interpolation
    return v0 * std::cos(theta) + v2 * std::sin(theta);
}

}