// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_NAVICUBE_H
#define OSGVERSEGUI_NAVICUBE_H

#include <osg/Quat>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <QColor>
#include <QPoint>

#include <map>
#include <vector>
#include <string>

class QMouseEvent;
class QOpenGLFramebufferObject;
class QOpenGLTexture;

namespace OsgVerseGui {

class OsgVerseViewer;

/**
 * @brief NaviCube for OsgVerse backend
 *
 * Provides a 3D navigation cube overlay for quickly orienting the camera
 * to standard views. The cube supports:
 * - 6 main faces (Front, Back, Top, Bottom, Left, Right)
 * - 12 edge faces
 * - 8 corner faces
 * - Directional arrows for rotation
 * - View menu button
 *
 * Uses color-based picking for face selection.
 * This implementation mirrors the Coin3D NaviCube for visual consistency.
 */
class OsgVerseNaviCube {
public:
    /**
     * @brief Corner position for the NaviCube
     */
    enum Corner {
        CornerTopLeft = 0,
        CornerTopRight = 1,
        CornerBottomLeft = 2,
        CornerBottomRight = 3
    };

    /**
     * @brief Face/element identifiers for picking
     */
    enum PickId {
        None = 0,
        Front,
        Top,
        Right,
        Rear,
        Bottom,
        Left,
        FrontTop,
        FrontBottom,
        FrontRight,
        FrontLeft,
        RearTop,
        RearBottom,
        RearRight,
        RearLeft,
        TopRight,
        TopLeft,
        BottomRight,
        BottomLeft,
        FrontTopRight,
        FrontTopLeft,
        FrontBottomRight,
        FrontBottomLeft,
        RearTopRight,
        RearTopLeft,
        RearBottomRight,
        RearBottomLeft,
        ArrowNorth,
        ArrowSouth,
        ArrowEast,
        ArrowWest,
        ArrowRight,
        ArrowLeft,
        DotBackside,
        ViewMenu,
        PickIdCount
    };

    /**
     * @brief Shape type for geometry generation
     */
    enum ShapeType {
        ShapeNone = 0,
        ShapeMain,
        ShapeEdge,
        ShapeCorner,
        ShapeButton
    };

    /**
     * @brief Constructor
     * @param viewer The parent OsgVerse viewer
     */
    explicit OsgVerseNaviCube(OsgVerseViewer* viewer);

    /**
     * @brief Destructor
     */
    ~OsgVerseNaviCube();

    //-----------------------------------------------------------------------
    // Core interface
    //-----------------------------------------------------------------------

    /**
     * @brief Enable or disable the NaviCube
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if NaviCube is enabled
     */
    bool isEnabled() const;

    /**
     * @brief Draw the NaviCube
     *
     * Called during rendering to draw the navigation cube overlay.
     */
    void draw();

    /**
     * @brief Handle mouse events
     * @param event The Qt mouse event
     * @return true if the event was handled by NaviCube
     */
    bool handleMouseEvent(QMouseEvent* event);

    /**
     * @brief Handle viewport resize
     * @param width New viewport width
     * @param height New viewport height
     */
    void resize(int width, int height);

    //-----------------------------------------------------------------------
    // Configuration interface
    //-----------------------------------------------------------------------

    /**
     * @brief Set the corner position
     */
    void setCorner(Corner corner);

    /**
     * @brief Get current corner position
     */
    Corner getCorner() const;

    /**
     * @brief Set the NaviCube size in pixels
     * @param size Size in pixels (default: 132)
     */
    void setSize(int size);

    /**
     * @brief Get the NaviCube size
     */
    int getSize() const;

    /**
     * @brief Set face labels
     * @param labels Vector of 6 strings: Front, Top, Right, Rear, Bottom, Left
     */
    void setLabels(const std::vector<std::string>& labels);

    /**
     * @brief Set the base color of the cube faces
     */
    void setBaseColor(const QColor& color);

    /**
     * @brief Get the base color
     */
    QColor getBaseColor() const;

    /**
     * @brief Set the highlight color for hovered faces
     */
    void setHighlightColor(const QColor& color);

    /**
     * @brief Get the highlight color
     */
    QColor getHighlightColor() const;

    /**
     * @brief Set the border/emphasis color
     */
    void setEmphasisColor(const QColor& color);

    /**
     * @brief Get the emphasis color
     */
    QColor getEmphasisColor() const;

    /**
     * @brief Set the chamfer amount (edge beveling)
     * @param chamfer Value between 0.05 and 0.18
     */
    void setChamfer(float chamfer);

    /**
     * @brief Get the chamfer amount
     */
    float getChamfer() const;

    /**
     * @brief Set whether to rotate to nearest standard view
     */
    void setRotateToNearest(bool enable);

    /**
     * @brief Get rotate to nearest setting
     */
    bool getRotateToNearest() const;

    /**
     * @brief Set the font zoom factor
     */
    void setFontZoom(float zoom);

    /**
     * @brief Set the inactive opacity
     */
    void setInactiveOpacity(float opacity);

    /**
     * @brief Set whether the cube is draggable
     */
    void setDraggable(bool draggable);

    /**
     * @brief Check if draggable
     */
    bool isDraggable() const;

    /**
     * @brief Update colors from ViewParams
     */
    void updateColors();

private:
    //-----------------------------------------------------------------------
    // Geometry creation
    //-----------------------------------------------------------------------

    /**
     * @brief Prepare all geometry and textures
     */
    void prepare();

    /**
     * @brief Create label textures for main faces
     */
    void createCubeFaceTextures();

    /**
     * @brief Add a cube face (main, edge, or corner)
     */
    void addCubeFace(const osg::Vec3f& x, const osg::Vec3f& z,
                     ShapeType shapeType, PickId pickId, float rotZ = 0.0f);

    /**
     * @brief Add a button face (arrows, menu)
     */
    void addButtonFace(PickId pickId, const osg::Vec3f& direction = osg::Vec3f(0, 0, 0));

    //-----------------------------------------------------------------------
    // Drawing
    //-----------------------------------------------------------------------

    /**
     * @brief Draw the NaviCube using OpenGL
     * @param pickMode If true, draw with color-coded faces for picking
     * @param opacity Opacity level for rendering
     */
    void drawNaviCube(bool pickMode, float opacity);

    //-----------------------------------------------------------------------
    // Picking
    //-----------------------------------------------------------------------

    /**
     * @brief Pick face at screen coordinates
     * @param x Screen X coordinate (relative to NaviCube center)
     * @param y Screen Y coordinate (relative to NaviCube center)
     * @return PickId of the face under cursor, or None
     */
    PickId pickFace(int x, int y);

    /**
     * @brief Ensure the picking framebuffer is valid
     */
    void ensurePickingFramebuffer();

    /**
     * @brief Draw NaviCube for picking (color-coded)
     */
    void drawForPicking();

    //-----------------------------------------------------------------------
    // View control
    //-----------------------------------------------------------------------

    /**
     * @brief Set camera orientation based on picked face
     */
    void setCameraOrientation(PickId face);

    /**
     * @brief Get rotation quaternion for a face
     */
    osg::Quat getFaceRotation(PickId face) const;

    /**
     * @brief Get nearest standard orientation
     */
    osg::Quat getNearestOrientation(PickId pickId) const;

    //-----------------------------------------------------------------------
    // Event handling helpers
    //-----------------------------------------------------------------------

    /**
     * @brief Handle mouse press
     */
    bool mousePressed(int x, int y);

    /**
     * @brief Handle mouse release
     */
    bool mouseReleased(int x, int y);

    /**
     * @brief Handle mouse move
     */
    bool mouseMoved(int x, int y);

    /**
     * @brief Check if position is in drag zone
     */
    bool inDragZone(int x, int y) const;

    /**
     * @brief Translate screen position to NaviCube-relative coordinates
     */
    QPoint translateToLocal(const QPoint& screenPos) const;

    /**
     * @brief Update highlight state
     */
    void setHighlight(PickId id);

    /**
     * @brief Handle resize for position calculation
     */
    void handleResize();

    /**
     * @brief Request a redraw
     */
    void scheduleRedraw();

    /**
     * @brief Update face colors based on highlight state
     */
    void updateFaceColors();

private:
    //-----------------------------------------------------------------------
    // Internal data structures
    //-----------------------------------------------------------------------

    struct Face {
        ShapeType type = ShapeNone;
        std::vector<osg::Vec3f> vertices;
        osg::Quat rotation;
        osg::Vec3f direction;  // For buttons: the rotation axis
        osg::Vec3f forward;    // Camera forward direction in world space
        osg::Vec3f up;         // Camera up direction in world space
    };

    struct LabelTexture {
        std::vector<osg::Vec3f> vertices;
        qreal fontSize = 0.0;
        QOpenGLTexture* texture = nullptr;
        std::string label;
    };

    //-----------------------------------------------------------------------
    // Member variables
    //-----------------------------------------------------------------------

    OsgVerseViewer* _viewer;
    bool _enabled{true};
    bool _prepared{false};

    // Picking
    QOpenGLFramebufferObject* _pickingFramebuffer{nullptr};

    // Geometry data
    std::map<PickId, Face> _faces;
    std::map<PickId, LabelTexture> _labelTextures;

    // Configuration
    int _cubeSize{132};
    Corner _corner{CornerTopRight};
    QColor _baseColor{226, 232, 239};
    QColor _hiliteColor{170, 226, 255};
    QColor _emphasisColor{51, 51, 51};
    float _chamfer{0.12f};
    float _fontZoom{0.3f};
    float _inactiveOpacity{0.5f};
    float _borderWidth{1.1f};
    bool _rotateToNearest{true};
    int _naviStepByTurn{8};
    bool _draggable{false};
    bool _showCS{true};

    // Axis colors
    osg::Vec4f _xColor{1.0f, 0.2f, 0.2f, 1.0f};
    osg::Vec4f _yColor{0.2f, 0.8f, 0.2f, 1.0f};
    osg::Vec4f _zColor{0.2f, 0.2f, 1.0f, 1.0f};

    // Labels
    std::vector<std::string> _labels{"FRONT", "TOP", "RIGHT", "REAR", "BOTTOM", "LEFT"};

    // State
    PickId _hiliteId{None};
    bool _hovering{false};
    bool _mouseDown{false};
    bool _dragging{false};
    bool _mightDrag{false};

    // Position
    float _relPosX{1.0f};  // 0.0 = left, 1.0 = right
    float _relPosY{1.0f};  // 0.0 = bottom, 1.0 = top
    int _posAreaBaseX{0};
    int _posAreaBaseY{0};
    int _posAreaSizeX{0};
    int _posAreaSizeY{0};
    int _viewWidth{0};
    int _viewHeight{0};
    float _devicePixelRatio{1.0f};
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_NAVICUBE_H
