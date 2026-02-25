/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                           *
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

#ifndef GUI_RENDER_INTERACTIONTYPES_H
#define GUI_RENDER_INTERACTIONTYPES_H

#include <vector>
#include <string>
#include <cstdint>

#include <FCGlobal.h>
#include "RenderTypes.h"

namespace Gui {

// Forward declarations
class ViewProvider;

namespace Render {

//=============================================================================
// Input Events - Backend-agnostic event representation
//=============================================================================

/**
 * @brief Mouse button identifiers
 */
enum class MouseButton : uint8_t {
    None = 0,
    Left = 1,
    Middle = 2,
    Right = 3,
    Button4 = 4,
    Button5 = 5
};

/**
 * @brief Keyboard modifier flags
 */
enum class KeyModifier : uint8_t {
    None  = 0x00,
    Shift = 0x01,
    Ctrl  = 0x02,
    Alt   = 0x04,
    Meta  = 0x08  // Windows/Command key
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline KeyModifier operator&(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool hasModifier(KeyModifier mods, KeyModifier test) {
    return (static_cast<uint8_t>(mods) & static_cast<uint8_t>(test)) != 0;
}

/**
 * @brief Input event type
 */
enum class InputEventType : uint8_t {
    Unknown = 0,
    MousePress,
    MouseRelease,
    MouseMove,
    MouseDoubleClick,
    MouseWheel,
    KeyPress,
    KeyRelease,
    TouchBegin,
    TouchUpdate,
    TouchEnd
};

/**
 * @brief Backend-agnostic input event
 *
 * This structure represents mouse, keyboard, and touch events
 * in a way that is independent of the underlying graphics backend.
 */
struct GuiExport InputEvent {
    InputEventType type{InputEventType::Unknown};

    //-------------------------------------------------------------------------
    // Mouse data
    //-------------------------------------------------------------------------

    /// Screen position in pixels (origin at top-left)
    int screenX{0};
    int screenY{0};

    /// Normalized position [0,1] (origin at bottom-left, like OpenGL)
    float normalizedX{0.0f};
    float normalizedY{0.0f};

    /// Mouse button (for press/release events)
    MouseButton button{MouseButton::None};

    /// Pressed buttons bitmask
    uint8_t buttons{0};

    /// Wheel delta (positive = up/forward)
    float wheelDelta{0.0f};

    //-------------------------------------------------------------------------
    // Keyboard data
    //-------------------------------------------------------------------------

    /// Key code (Qt::Key compatible)
    int key{0};

    /// Character representation (if printable)
    char character{0};

    /// Modifier keys
    KeyModifier modifiers{KeyModifier::None};

    //-------------------------------------------------------------------------
    // Touch data
    //-------------------------------------------------------------------------

    /// Touch point ID (for multi-touch)
    int touchId{0};

    /// Touch pressure [0,1]
    float pressure{0.0f};

    //-------------------------------------------------------------------------
    // Timing
    //-------------------------------------------------------------------------

    /// Timestamp in milliseconds
    double timestamp{0.0};

    //-------------------------------------------------------------------------
    // Helper methods
    //-------------------------------------------------------------------------

    bool isMouseEvent() const {
        return type == InputEventType::MousePress ||
               type == InputEventType::MouseRelease ||
               type == InputEventType::MouseMove ||
               type == InputEventType::MouseDoubleClick ||
               type == InputEventType::MouseWheel;
    }

    bool isKeyEvent() const {
        return type == InputEventType::KeyPress ||
               type == InputEventType::KeyRelease;
    }

    bool isTouchEvent() const {
        return type == InputEventType::TouchBegin ||
               type == InputEventType::TouchUpdate ||
               type == InputEventType::TouchEnd;
    }

    bool hasShift() const { return hasModifier(modifiers, KeyModifier::Shift); }
    bool hasCtrl() const { return hasModifier(modifiers, KeyModifier::Ctrl); }
    bool hasAlt() const { return hasModifier(modifiers, KeyModifier::Alt); }
    bool hasMeta() const { return hasModifier(modifiers, KeyModifier::Meta); }

    bool isLeftButton() const { return button == MouseButton::Left; }
    bool isMiddleButton() const { return button == MouseButton::Middle; }
    bool isRightButton() const { return button == MouseButton::Right; }
};

//=============================================================================
// Pick Results - Backend-agnostic picking information
//=============================================================================

/**
 * @brief Pick type specifying what kind of geometry was hit
 */
enum class PickType : uint8_t {
    None = 0,
    Face,
    Edge,
    Vertex,
    Point
};

/**
 * @brief Result of a single pick operation
 *
 * Contains information about what was hit by a pick ray.
 */
struct GuiExport PickResult {
    /// Whether something was hit
    bool hit{false};

    /// Type of geometry hit
    PickType type{PickType::None};

    /// World-space intersection point
    Vector3 point{0, 0, 0};

    /// Surface normal at intersection (if applicable)
    Vector3 normal{0, 0, 1};

    /// Distance from camera to intersection
    float distance{0.0f};

    /// ViewProvider that owns the hit geometry
    ViewProvider* viewProvider{nullptr};

    /// Sub-element name (e.g., "Face1", "Edge5", "Vertex3")
    std::string elementName;

    /// Face/triangle index (if face hit)
    int faceIndex{-1};

    /// Edge index (if edge hit)
    int edgeIndex{-1};

    /// Vertex index (if vertex hit)
    int vertexIndex{-1};

    /// Is this the closest hit?
    bool isClosest{false};

    //-------------------------------------------------------------------------
    // Helper methods
    //-------------------------------------------------------------------------

    bool isFace() const { return type == PickType::Face; }
    bool isEdge() const { return type == PickType::Edge; }
    bool isVertex() const { return type == PickType::Vertex; }

    /// Compare by distance (for sorting)
    bool operator<(const PickResult& other) const {
        return distance < other.distance;
    }
};

/**
 * @brief Collection of pick results from a pick operation
 */
struct GuiExport PickResults {
    /// All hits, sorted by distance (closest first)
    std::vector<PickResult> hits;

    /// Screen position of the pick
    int screenX{0};
    int screenY{0};

    /// Ray origin in world space
    Vector3 rayOrigin{0, 0, 0};

    /// Ray direction in world space (normalized)
    Vector3 rayDirection{0, 0, -1};

    //-------------------------------------------------------------------------
    // Helper methods
    //-------------------------------------------------------------------------

    bool hasHit() const { return !hits.empty(); }

    size_t hitCount() const { return hits.size(); }

    const PickResult& closest() const {
        static PickResult empty;
        return hits.empty() ? empty : hits.front();
    }

    /// Get hit by ViewProvider
    const PickResult* getHitByViewProvider(ViewProvider* vp) const {
        for (const auto& hit : hits) {
            if (hit.viewProvider == vp) {
                return &hit;
            }
        }
        return nullptr;
    }

    /// Get all hits for a specific ViewProvider
    std::vector<const PickResult*> getHitsByViewProvider(ViewProvider* vp) const {
        std::vector<const PickResult*> result;
        for (const auto& hit : hits) {
            if (hit.viewProvider == vp) {
                result.push_back(&hit);
            }
        }
        return result;
    }
};

//=============================================================================
// Ray definition for pick operations
//=============================================================================

/**
 * @brief Ray for pick operations
 */
struct GuiExport PickRay {
    Vector3 origin{0, 0, 0};
    Vector3 direction{0, 0, -1};

    /// Near clip distance
    float nearDistance{0.0f};

    /// Far clip distance
    float farDistance{1e10f};

    /// Get point along ray at parameter t
    Vector3 getPoint(float t) const {
        return Vector3{
            origin.x + direction.x * t,
            origin.y + direction.y * t,
            origin.z + direction.z * t
        };
    }
};

//=============================================================================
// Pick options
//=============================================================================

/**
 * @brief Options for pick operations
 */
struct GuiExport PickOptions {
    /// Pick only the closest object
    bool pickClosestOnly{true};

    /// Maximum number of hits to return
    int maxHits{100};

    /// Pick faces
    bool pickFaces{true};

    /// Pick edges
    bool pickEdges{true};

    /// Pick vertices
    bool pickVertices{true};

    /// Radius for edge/vertex picking (in pixels)
    float pickRadius{5.0f};

    /// Only pick from visible objects
    bool pickVisibleOnly{true};

    /// Only pick from selectable objects
    bool pickSelectableOnly{true};

    /// Specific ViewProviders to pick from (empty = all)
    std::vector<ViewProvider*> filterViewProviders;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_INTERACTIONTYPES_H
