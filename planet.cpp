// =============================================================================
// planet.cpp
// -----------------------------------------------------------------------------
// Implements the Planet class defined in planet.h.
//
// This file is responsible for:
//   - Constructing planet objects with all orbital and visual parameters
//   - Advancing orbital and self-rotation angles each frame (update)
//   - Managing child moon hierarchy (addMoon)
//   - Configuring Saturn's ring geometry (setupRings)
//   - Handling user-driven size scaling with safe clamping (scaleSize)
//   - Computing the planet's current Cartesian position on its ellipse
//
// GRAPHICS CONCEPT — Time-Based Animation:
//   All angular updates use the form:
//       angle += speed * dt
//   where dt is the elapsed time in seconds (already scaled by timeScale).
//   This ensures animation speed is independent of frame rate — the planet
//   moves the same angular distance per real second whether the app runs at
//   30fps or 120fps.
// =============================================================================

#include "planet.h"
#include "math_utils.h"   // for degreesToRadians(), DEG_TO_RAD

#include <cmath>          // std::cos, std::sin
#include <algorithm>      // std::clamp (C++17)

// =============================================================================
// Constructor
// =============================================================================
// Initialises every field. The starting orbitAngle and selfRotationAngle are
// set to 0 so planets begin at the rightmost point of their ellipse.
// hasRings is false by default — only Saturn calls setupRings() after this.
// =============================================================================
Planet::Planet(const std::string& name,
               int         id,
               float       size,
               float       orbitRadiusX,
               float       orbitRadiusY,
               float       orbitSpeed,
               float       selfRotSpeed,
               float       r,
               float       g,
               float       b)
    : name(name)
    , id(id)
    , size(size)
    , baseSize(size)            // remember original for clamping / reset
    , orbitRadiusX(orbitRadiusX)
    , orbitRadiusY(orbitRadiusY)
    , orbitSpeed(orbitSpeed)
    , orbitAngle(0.0f)          // start at angle 0 (rightmost ellipse point)
    , selfRotationSpeed(selfRotSpeed)
    , selfRotationAngle(0.0f)
    , hasRings(false)
{
    // Store RGB colour components
    color[0] = r;
    color[1] = g;
    color[2] = b;

    // Zero-initialise ring params — they are only meaningful after setupRings()
    rings.innerRadius   = 0.0f;
    rings.outerRadius   = 0.0f;
    rings.color[0]      = 0.0f;
    rings.color[1]      = 0.0f;
    rings.color[2]      = 0.0f;
    rings.color[3]      = 1.0f;
}

// =============================================================================
// update(dt)
// =============================================================================
// Advances the orbital angle and self-rotation angle by one time step.
//
// GRAPHICS CONCEPT — Parametric Angle Accumulation:
//   We store the angle as a continuously growing float rather than wrapping
//   it to [0, 360). This avoids a discontinuity that could cause a one-frame
//   jump if the wrap happens mid-frame. The trigonometric functions (cos/sin)
//   used in getOrbitX/Y are periodic anyway, so large angle values are safe.
//
// dt is negative when time is reversed (timeScale < 0) and zero when paused.
// Recursion into moons ensures their angles advance in the same time step,
// keeping the hierarchy perfectly synchronised.
// =============================================================================
void Planet::update(float dt)
{
    // Advance orbital position along the ellipse
    orbitAngle += orbitSpeed * dt;

    // Advance axial spin
    selfRotationAngle += selfRotationSpeed * dt;

    // Recursively update all child moons.
    // Because moons are stored by value inside this planet, their update()
    // advances their angle relative to this planet's frame — exactly matching
    // how the renderer will later draw them inside this planet's push/pop block.
    for (Planet& moon : moons) {
        moon.update(dt);
    }
}

// =============================================================================
// addMoon(moon)
// =============================================================================
// Pushes a copy of the moon Planet into this planet's children vector.
//
// Using push_back (value semantics) means the moon is fully owned by this
// planet — simple and safe with no manual memory management needed.
// =============================================================================
void Planet::addMoon(const Planet& moon)
{
    moons.push_back(moon);
}

// =============================================================================
// setupRings(innerR, outerR, r, g, b, a)
// =============================================================================
// Configures the RingParams struct and sets hasRings = true.
//
// GRAPHICS CONCEPT — Rings in the Parent Frame:
//   Because the renderer draws rings inside Saturn's own glPushMatrix() block
//   (after applying Saturn's self-rotation), the ring disk automatically
//   rotates with the planet. No extra transformation is needed here — the
//   coordinate frame does the work.
//
// The alpha channel (a) allows semi-transparent rings when blending is on.
// =============================================================================
void Planet::setupRings(float innerR, float outerR,
                        float r, float g, float b, float a)
{
    hasRings            = true;
    rings.innerRadius   = innerR;
    rings.outerRadius   = outerR;
    rings.color[0]      = r;
    rings.color[1]      = g;
    rings.color[2]      = b;
    rings.color[3]      = a;
}

// =============================================================================
// scaleSize(factor)
// =============================================================================
// Multiplies the planet's current size by factor, then clamps to a safe range.
//
// Clamp range: [0.2 * baseSize,  5.0 * baseSize]
//   - Lower bound prevents the planet from shrinking to an invisible point.
//   - Upper bound prevents it from growing so large it occludes the whole scene.
//
// Called by controls.cpp in response to '[' (factor < 1) and ']' (factor > 1).
// =============================================================================
void Planet::scaleSize(float factor)
{
    size = size * factor;

    // Clamp to sensible bounds relative to the original size
    float minSize = 0.2f * baseSize;
    float maxSize = 5.0f * baseSize;

    if (size < minSize) size = minSize;
    if (size > maxSize) size = maxSize;
}

// =============================================================================
// resetSize()
// =============================================================================
// Restores size to the value it had at construction.
// This is called when the user presses 'T' (reset speed) or we add a dedicated
// reset key — giving the user a clear way to undo accidental scaling.
// =============================================================================
void Planet::resetSize()
{
    size = baseSize;
}

// =============================================================================
// getOrbitX()
// =============================================================================
// Returns the current x-coordinate of this planet on its elliptical orbit.
//
// PARAMETRIC ELLIPSE:
//   x(theta) = orbitRadiusX * cos(theta)
//
// The angle (orbitAngle) is stored in DEGREES, so we convert to radians here
// using the utility function from math_utils.h before passing to std::cos.
//
// This value is used by the renderer when it needs the planet's world-space
// x-position without re-entering the full OpenGL matrix stack
// (e.g. for drawing name labels or a selection highlight ring).
// =============================================================================
float Planet::getOrbitX() const
{
    return orbitRadiusX * std::cos(degreesToRadians(orbitAngle));
}

// =============================================================================
// getOrbitY()
// =============================================================================
// Returns the current y-coordinate of this planet on its elliptical orbit.
//
// PARAMETRIC ELLIPSE:
//   y(theta) = orbitRadiusY * sin(theta)
//
// Combined with getOrbitX(), this gives the full 2D world-space position.
// =============================================================================
float Planet::getOrbitY() const
{
    return orbitRadiusY * std::sin(degreesToRadians(orbitAngle));
}