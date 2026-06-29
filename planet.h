// =============================================================================
// planet.h
// -----------------------------------------------------------------------------
// Defines the Planet class — the core data model of the Solar System Simulator.
//
// Every celestial body in this simulation (sun, planet, or moon) is represented
// as a Planet object. The class encapsulates:
//   - Visual properties  (color, size)
//   - Orbital parameters (elliptical orbit axes, speed, current angle)
//   - Self-rotation      (spin around own axis)
//   - Hierarchy          (a planet can own a list of child moons)
//   - Special features   (Saturn's rings flag)
//
// DESIGN NOTE:
//   We use a recursive structure — a Planet holds a vector<Planet> for its
//   moons. This mirrors the hierarchical matrix stack in OpenGL:
//   the renderer recurses into children while still inside the parent's
//   glPushMatrix() / glPopMatrix() block, so child positions are naturally
//   expressed relative to their parent. This is the heart of hierarchical
//   modeling in computer graphics.
// =============================================================================

#ifndef PLANET_H
#define PLANET_H

#include <string>
#include <vector>

// =============================================================================
// RingParams
// -----------------------------------------------------------------------------
// Holds parameters for Saturn's ring system.
// Rings are drawn as a flat annulus (filled region between two radii)
// inside Saturn's own coordinate frame, so they rotate with the planet
// automatically — no extra transformation math required.
// =============================================================================
struct RingParams {
    float innerRadius;   // inner edge of the ring disk
    float outerRadius;   // outer edge of the ring disk
    float color[4];      // RGBA — alpha < 1 gives a semi-transparent look
};

// =============================================================================
// Planet
// -----------------------------------------------------------------------------
// Represents any celestial body: sun, planet, or moon.
//
// Orbital motion uses a PARAMETRIC ELLIPSE:
//   x(t) = orbitRadiusX * cos(orbitAngle)
//   y(t) = orbitRadiusY * sin(orbitAngle)
//
// orbitAngle advances each frame by (orbitSpeed * dt), where dt is scaled
// by the global timeScale — giving smooth, frame-rate-independent animation.
//
// For the Sun, orbitRadiusX = orbitRadiusY = 0 (it sits at the world origin).
// =============================================================================
class Planet {
public:

    // -------------------------------------------------------------------------
    // Identity
    // -------------------------------------------------------------------------
    std::string name;       // display name (e.g. "Earth", "Moon")
    int         id;         // unique index 0–7 for planets; -1 for sun/moons

    // -------------------------------------------------------------------------
    // Visual Properties
    // -------------------------------------------------------------------------
    float size;             // rendering radius (in world units)
    float baseSize;         // original size — used when resetting scale
    float color[3];         // RGB components, each in [0.0, 1.0]

    // -------------------------------------------------------------------------
    // Orbital Parameters  (elliptical orbit)
    // -------------------------------------------------------------------------
    // The orbit is an ellipse with semi-major axis orbitRadiusX (horizontal)
    // and semi-minor axis orbitRadiusY (vertical). Setting both equal gives
    // a circle; making them differ gives a realistic elliptical path.
    float orbitRadiusX;     // semi-major axis (x-direction extent)
    float orbitRadiusY;     // semi-minor axis (y-direction extent)
    float orbitSpeed;       // orbital angular speed in degrees per second
    float orbitAngle;       // current orbital angle in degrees (simulation state)

    // -------------------------------------------------------------------------
    // Self-Rotation (axial spin)
    // -------------------------------------------------------------------------
    float selfRotationSpeed;  // axial spin speed in degrees per second
    float selfRotationAngle;  // current spin angle in degrees (simulation state)

    // -------------------------------------------------------------------------
    // Saturn's Rings
    // -------------------------------------------------------------------------
    bool      hasRings;     // true only for Saturn
    RingParams rings;       // ring geometry and color (used if hasRings == true)

    // -------------------------------------------------------------------------
    // Hierarchy — Child Bodies (Moons)
    // -------------------------------------------------------------------------
    // A planet owns its moons directly. The renderer will recurse into this
    // vector while still inside the parent's OpenGL matrix push/pop block,
    // so each moon's position is automatically relative to its parent planet.
    std::vector<Planet> moons;

    // =========================================================================
    // Constructor
    // =========================================================================
    // Constructs a planet with all orbital and visual parameters.
    //
    // Parameters:
    //   name          - display name
    //   id            - selection index (0–7 for planets, -1 otherwise)
    //   size          - rendering radius
    //   orbitRadiusX  - semi-major axis of elliptical orbit
    //   orbitRadiusY  - semi-minor axis of elliptical orbit
    //   orbitSpeed    - orbital speed in degrees/second
    //   selfRotSpeed  - axial spin speed in degrees/second
    //   r, g, b       - RGB color
    // =========================================================================
    Planet(const std::string& name,
           int         id,
           float       size,
           float       orbitRadiusX,
           float       orbitRadiusY,
           float       orbitSpeed,
           float       selfRotSpeed,
           float       r,
           float       g,
           float       b);

    // =========================================================================
    // update(dt)
    // =========================================================================
    // Advances the planet's orbital angle and self-rotation angle by one
    // time step. Also recursively updates all child moons.
    //
    // Parameters:
    //   dt - delta time in seconds, already multiplied by timeScale.
    //        Negative dt causes reverse motion; zero dt pauses.
    //
    // This function is called once per frame from the GLUT timer callback.
    // =========================================================================
    void update(float dt);

    // =========================================================================
    // addMoon(moon)
    // =========================================================================
    // Attaches a moon to this planet. The moon becomes a child in the
    // hierarchy and will be rendered in this planet's coordinate frame.
    // =========================================================================
    void addMoon(const Planet& moon);

    // =========================================================================
    // setupRings(innerR, outerR, r, g, b, a)
    // =========================================================================
    // Configures ring geometry and color for Saturn.
    // Sets hasRings = true.
    //
    // Parameters:
    //   innerR - inner ring radius
    //   outerR - outer ring radius
    //   r,g,b  - ring color
    //   a      - alpha (0 = fully transparent, 1 = fully opaque)
    // =========================================================================
    void setupRings(float innerR, float outerR,
                    float r, float g, float b, float a);

    // =========================================================================
    // scaleSize(factor)
    // =========================================================================
    // Multiplies the current size by factor.
    // Clamps to [0.2 * baseSize, 5.0 * baseSize] so the planet cannot be
    // scaled to zero or an unreasonably large value.
    //
    // Called by controls.cpp in response to '[' and ']' key presses.
    // =========================================================================
    void scaleSize(float factor);

    // =========================================================================
    // resetSize()
    // =========================================================================
    // Restores size to its original baseSize value.
    // =========================================================================
    void resetSize();

    // =========================================================================
    // getOrbitX() / getOrbitY()
    // =========================================================================
    // Returns the planet's current Cartesian position on its elliptical orbit.
    //
    // Uses the parametric ellipse equations:
    //   x = orbitRadiusX * cos(orbitAngle)
    //   y = orbitRadiusY * sin(orbitAngle)
    //
    // Used by renderer.cpp when it needs the world-space position
    // (e.g. for drawing a label or a selection indicator).
    // =========================================================================
    float getOrbitX() const;
    float getOrbitY() const;
};

#endif // PLANET_H