// =============================================================================
// math_utils.h
// -----------------------------------------------------------------------------
// Mathematical utilities for the Hierarchical Solar System Simulator.
//
// This header sits at the bottom of the dependency tree — it depends on
// nothing else in the project. Every other file that needs math includes this.
//
// Provides:
//   - Constants          (PI, DEG_TO_RAD, RAD_TO_DEG)
//   - Vec2 struct        (2D vector with common operations)
//   - Angle utilities    (degreesToRadians, radiansToDegrees, normalizeAngle)
//   - Ellipse utilities  (ellipsePoint, ellipseTangent)
//   - Interpolation      (lerp, smoothstep, clampf)
//   - OpenGL draw helpers(drawCircle, drawFilledCircle, drawSphere,
//                         drawEllipse, drawFilledAnnulus, drawStarfield)
//
// DESIGN NOTE — Why put draw helpers in math_utils?
//   These helpers are pure geometric functions: they take a radius or axis
//   length and emit GL_LINE_LOOP / GL_TRIANGLE_FAN vertices. They have no
//   knowledge of planet state, simulation time, or scene structure. Putting
//   them here keeps renderer.cpp focused on scene logic while this file owns
//   the low-level primitive construction.
// =============================================================================

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>    // std::cos, std::sin, std::sqrt, std::fmod
#include <vector>   // std::vector (for star positions)

// =============================================================================
// Constants
// =============================================================================

// Pi to sufficient precision for all angular calculations in this project
const float PI          = 3.14159265358979323846f;

// Multiply degrees by this to get radians:  radians = degrees * DEG_TO_RAD
const float DEG_TO_RAD  = PI / 180.0f;

// Multiply radians by this to get degrees:  degrees = radians * RAD_TO_DEG
const float RAD_TO_DEG  = 180.0f / PI;

// Number of line segments used to approximate a circle or ellipse.
// Higher = smoother curves, lower = faster rendering.
// 64 is a good balance for this simulation.
const int   CIRCLE_SEGMENTS = 64;

// Number of background stars in the starfield
const int   STAR_COUNT      = 300;

// =============================================================================
// Vec2
// -----------------------------------------------------------------------------
// A minimal 2D vector type used for orbit positions, tangent directions,
// and any 2D geometric computation in the project.
//
// Kept intentionally simple — only the operations actually used are defined.
// =============================================================================
struct Vec2 {

    float x, y;

    // -------------------------------------------------------------------------
    // Constructors
    // -------------------------------------------------------------------------
    Vec2();                          // zero vector (0, 0)
    Vec2(float x, float y);          // initialise with components

    // -------------------------------------------------------------------------
    // Arithmetic operators
    // -------------------------------------------------------------------------
    Vec2  operator+(const Vec2& other) const;   // vector addition
    Vec2  operator-(const Vec2& other) const;   // vector subtraction
    Vec2  operator*(float scalar)      const;   // scalar multiplication
    Vec2& operator+=(const Vec2& other);        // in-place addition
    Vec2& operator-=(const Vec2& other);        // in-place subtraction

    // -------------------------------------------------------------------------
    // Vector operations
    // -------------------------------------------------------------------------

    // Returns the Euclidean length: sqrt(x*x + y*y)
    float length() const;

    // Returns a unit vector in the same direction (length = 1).
    // Returns zero vector if this vector has zero length (safe).
    Vec2  normalized() const;

    // Dot product: x*other.x + y*other.y
    // Used for projections and angle calculations.
    float dot(const Vec2& other) const;

    // Returns a vector perpendicular to this one (rotated 90° CCW).
    // Useful for drawing orbit tangents or normal directions.
    Vec2  perpendicular() const;
};

// =============================================================================
// Angle Utilities
// =============================================================================

// Convert degrees to radians.
// Used throughout the project wherever OpenGL angles (stored in degrees)
// need to be passed to std::cos or std::sin.
float degreesToRadians(float degrees);

// Convert radians to degrees.
float radiansToDegrees(float radians);

// Normalise an angle to the range [0, 360).
// Handles negative angles correctly (e.g. -10 → 350).
float normalizeAngle(float degrees);

// =============================================================================
// Ellipse Utilities
// =============================================================================

// Returns the Cartesian (x, y) position on an ellipse at a given angle.
//
// Parameters:
//   radiusX - semi-major axis (horizontal extent)
//   radiusY - semi-minor axis (vertical extent)
//   angleDeg - angle in degrees (0 = rightmost point of ellipse)
//
// Formula:
//   x = radiusX * cos(angleDeg)
//   y = radiusY * sin(angleDeg)
//
// This is the parametric ellipse equation used by both the orbit path
// renderer and planet.cpp's getOrbitX() / getOrbitY().
Vec2 ellipsePoint(float radiusX, float radiusY, float angleDeg);

// Returns the unit tangent vector to the ellipse at a given angle.
// This is the derivative of the position with respect to angle, normalised.
//
// Useful for orienting arrows or velocity indicators along the orbit.
//
// Formula (before normalisation):
//   dx/dtheta = -radiusX * sin(theta)
//   dy/dtheta =  radiusY * cos(theta)
Vec2 ellipseTangent(float radiusX, float radiusY, float angleDeg);

// =============================================================================
// Interpolation & Clamping
// =============================================================================

// Linear interpolation between a and b by factor t in [0, 1].
//   t = 0 → returns a
//   t = 1 → returns b
//
// GRAPHICS CONCEPT — Keyframe Interpolation:
//   lerp is the building block of all smooth animation. When transitioning
//   between two states over time, compute t = elapsed / duration and use
//   lerp to get the intermediate value.
float lerp(float a, float b, float t);

// Smooth-step interpolation: like lerp but with ease-in / ease-out.
// Maps t in [0,1] to a smooth S-curve: 3t² - 2t³
// Produces more natural-looking motion than linear interpolation.
float smoothstep(float a, float b, float t);

// Clamps value to [minVal, maxVal].
float clampf(float value, float minVal, float maxVal);

// =============================================================================
// OpenGL Geometric Draw Helpers
// =============================================================================
// These functions emit raw OpenGL vertex calls. They must be called from
// within an active OpenGL rendering context (after glBegin / before glEnd,
// or they call glBegin/glEnd themselves — see individual documentation).
//
// They have NO knowledge of planet state or simulation time.
// They only understand geometry: radii, positions, segment counts.

// -----------------------------------------------------------------------------
// drawCircle(radius, segments)
// -----------------------------------------------------------------------------
// Draws a wireframe circle of the given radius centred at the current
// OpenGL origin, using GL_LINE_LOOP.
//
// The circle is approximated by `segments` straight line segments.
// Uses the current glColor state for colour.
//
// Typical use: orbit path rings, selection highlight.
void drawCircle(float radius, int segments = CIRCLE_SEGMENTS);

// -----------------------------------------------------------------------------
// drawFilledCircle(radius, segments)
// -----------------------------------------------------------------------------
// Draws a solid filled circle using GL_TRIANGLE_FAN.
// The first vertex is placed at the origin (fan centre),
// followed by `segments + 1` perimeter vertices.
//
// Typical use: drawing planets and the sun.
void drawFilledCircle(float radius, int segments = CIRCLE_SEGMENTS);

// -----------------------------------------------------------------------------
// drawSphere(radius, segments)
// -----------------------------------------------------------------------------
// Draws a pseudo-3D sphere by rendering multiple stacked ellipses
// (latitude bands) using GL_LINE_LOOP.
//
// This gives a wireframe globe appearance. The number of latitude bands
// equals segments / 4, which keeps it visually clean without excessive
// overdraw.
//
// GRAPHICS CONCEPT — Pseudo-3D in a 2D Renderer:
//   True 3D spheres require depth testing and lighting. Here we fake the
//   appearance by drawing compressed horizontal ellipses at varying
//   y-offsets, simulating the look of a sphere viewed from a slight angle.
void drawSphere(float radius, int segments = CIRCLE_SEGMENTS);

// -----------------------------------------------------------------------------
// drawEllipse(radiusX, radiusY, segments)
// -----------------------------------------------------------------------------
// Draws a wireframe ellipse using GL_LINE_LOOP.
//
// Used for:
//   1. Orbit path rendering  (each planet's orbital ellipse)
//   2. Equator lines on the pseudo-3D sphere
//
// Parameters:
//   radiusX  - horizontal semi-axis
//   radiusY  - vertical semi-axis
//   segments - number of line segments (default CIRCLE_SEGMENTS)
void drawEllipse(float radiusX, float radiusY, int segments = CIRCLE_SEGMENTS);

// -----------------------------------------------------------------------------
// drawFilledAnnulus(innerRadius, outerRadius, segments)
// -----------------------------------------------------------------------------
// Draws a filled ring (annulus) between innerRadius and outerRadius,
// using a GL_TRIANGLE_STRIP that alternates between inner and outer vertices.
//
// Used exclusively for Saturn's rings.
// The current glColor4f state is used — set alpha < 1 for transparency.
//
// GRAPHICS CONCEPT — Triangle Strip Topology:
//   GL_TRIANGLE_STRIP shares edges between successive triangles.
//   For N segments, we emit 2*(N+1) vertices forming 2*N triangles,
//   filling the annular region efficiently.
void drawFilledAnnulus(float innerRadius, float outerRadius,
                       int segments = CIRCLE_SEGMENTS);

// =============================================================================
// Star Field
// =============================================================================

// Struct holding the pre-generated position and brightness of one star.
// Stars are generated once at startup and stored — they never move.
struct StarPoint {
    float x, y;           // world-space position
    float brightness;     // in [0.5, 1.0] — gives varied star intensities
};

// generateStars(count, spread)
// -----------------------------------------------------------------------------
// Generates `count` stars at random positions within a square of half-width
// `spread`, each with a random brightness in [0.5, 1.0].
//
// Call this ONCE at startup (in main.cpp) and pass the result to drawStars().
// Using a fixed seed (srand(42)) makes the star field deterministic —
// it looks the same every time the program runs.
//
// Parameters:
//   count  - number of stars to generate (default STAR_COUNT)
//   spread - half-width of the square region (default 300.0)
std::vector<StarPoint> generateStars(int count  = STAR_COUNT,
                                     float spread = 300.0f);

// drawStars(stars)
// -----------------------------------------------------------------------------
// Renders all stars as GL_POINTS using their stored brightness as the grey
// colour value. Must be called before any planet transformations so the stars
// appear behind the solar system.
//
// Stars do NOT transform with any planet matrix — they are drawn at their
// fixed world-space coordinates and represent the infinite, fixed background.
void drawStars(const std::vector<StarPoint>& stars);

#endif // MATH_UTILS_H