// =============================================================================
// math_utils.cpp
// -----------------------------------------------------------------------------
// Implements every function and struct method declared in math_utils.h.
//
// Sections:
//   1. Vec2 — struct method implementations
//   2. Angle Utilities — degree/radian conversion, normalisation
//   3. Ellipse Utilities — parametric ellipse position and tangent
//   4. Interpolation & Clamping — lerp, smoothstep, clampf
//   5. OpenGL Draw Helpers — circle, sphere, ellipse, annulus primitives
//   6. Starfield — generation and rendering of background stars
//
// OpenGL headers are included here (and only here among the utility files)
// because the draw helpers emit raw GL vertex calls. Every other file that
// needs GL should include it directly; we do not re-export GL through this
// header.
//
// GRAPHICS CONCEPT — Immediate Mode OpenGL:
//   This project uses OpenGL's legacy "immediate mode" API (glBegin/glEnd),
//   which is appropriate for a university graphics course where the goal is
//   to understand transformation matrices and geometric construction rather
//   than modern GPU pipeline management.
// =============================================================================

#include "math_utils.h"

// OpenGL / GLUT headers (platform-aware)
#ifdef __APPLE__
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
#endif

#include <cmath>      // std::cos, std::sin, std::sqrt, std::fmod
#include <cstdlib>    // std::rand, std::srand, RAND_MAX
#include <ctime>      // (not used directly; srand seeded with fixed value)

// =============================================================================
// SECTION 1 — Vec2 Method Implementations
// =============================================================================

// -----------------------------------------------------------------------------
// Default constructor — zero vector
// -----------------------------------------------------------------------------
Vec2::Vec2()
    : x(0.0f), y(0.0f)
{}

// -----------------------------------------------------------------------------
// Component constructor
// -----------------------------------------------------------------------------
Vec2::Vec2(float x, float y)
    : x(x), y(y)
{}

// -----------------------------------------------------------------------------
// operator+ — vector addition
// Returns a new Vec2 whose components are the sum of both vectors.
// -----------------------------------------------------------------------------
Vec2 Vec2::operator+(const Vec2& other) const
{
    return Vec2(x + other.x, y + other.y);
}

// -----------------------------------------------------------------------------
// operator- — vector subtraction
// Returns a new Vec2 whose components are the difference of both vectors.
// -----------------------------------------------------------------------------
Vec2 Vec2::operator-(const Vec2& other) const
{
    return Vec2(x - other.x, y - other.y);
}

// -----------------------------------------------------------------------------
// operator* — scalar multiplication
// Scales both components by the given scalar value.
// -----------------------------------------------------------------------------
Vec2 Vec2::operator*(float scalar) const
{
    return Vec2(x * scalar, y * scalar);
}

// -----------------------------------------------------------------------------
// operator+= — in-place addition
// -----------------------------------------------------------------------------
Vec2& Vec2::operator+=(const Vec2& other)
{
    x += other.x;
    y += other.y;
    return *this;
}

// -----------------------------------------------------------------------------
// operator-= — in-place subtraction
// -----------------------------------------------------------------------------
Vec2& Vec2::operator-=(const Vec2& other)
{
    x -= other.x;
    y -= other.y;
    return *this;
}

// -----------------------------------------------------------------------------
// length()
// Returns the Euclidean magnitude: sqrt(x² + y²)
// Used by normalized() and any code that needs the actual distance.
// -----------------------------------------------------------------------------
float Vec2::length() const
{
    return std::sqrt(x * x + y * y);
}

// -----------------------------------------------------------------------------
// normalized()
// Returns a unit vector (length == 1) in the same direction.
//
// SAFETY: If the vector has zero length (or near-zero to avoid division by a
// very small number), we return the zero vector rather than NaN/Inf.
// Threshold of 1e-6 is well below any meaningful world-space distance.
// -----------------------------------------------------------------------------
Vec2 Vec2::normalized() const
{
    float len = length();
    if (len < 1e-6f) {
        return Vec2(0.0f, 0.0f);    // safe fallback — avoid division by zero
    }
    return Vec2(x / len, y / len);
}

// -----------------------------------------------------------------------------
// dot(other)
// Returns the scalar dot product: x*other.x + y*other.y
//
// GRAPHICS USE: dot(a, b) = |a||b|cos(theta)
// When both vectors are unit length, dot product gives cos of the angle
// between them — useful for checking if two directions are aligned.
// -----------------------------------------------------------------------------
float Vec2::dot(const Vec2& other) const
{
    return x * other.x + y * other.y;
}

// -----------------------------------------------------------------------------
// perpendicular()
// Returns a vector rotated 90° counter-clockwise: (-y, x)
//
// This is the 2D analogue of the cross product's direction.
// Useful for computing orbit tangents or constructing a local coordinate frame.
// -----------------------------------------------------------------------------
Vec2 Vec2::perpendicular() const
{
    return Vec2(-y, x);
}

// =============================================================================
// SECTION 2 — Angle Utilities
// =============================================================================

// -----------------------------------------------------------------------------
// degreesToRadians(degrees)
// Converts an angle in degrees to radians.
//
// Formula: radians = degrees * (PI / 180)
//
// This is called in planet.cpp's getOrbitX/Y and throughout renderer.cpp
// before passing angles to std::cos / std::sin, which operate in radians.
// -----------------------------------------------------------------------------
float degreesToRadians(float degrees)
{
    return degrees * DEG_TO_RAD;
}

// -----------------------------------------------------------------------------
// radiansToDegrees(radians)
// Converts an angle in radians to degrees.
//
// Formula: degrees = radians * (180 / PI)
//
// Useful when computing an angle via atan2 (which returns radians) and
// storing it as degrees for display or OpenGL rotation calls.
// -----------------------------------------------------------------------------
float radiansToDegrees(float radians)
{
    return radians * RAD_TO_DEG;
}

// -----------------------------------------------------------------------------
// normalizeAngle(degrees)
// Wraps an angle to the range [0, 360).
//
// std::fmod handles the wrap, but fmod(-10, 360) returns -10 in C++
// (it preserves sign), so we add 360 and fmod again to handle negatives.
//
// Example:  normalizeAngle(-10)  → 350
//           normalizeAngle(370)  → 10
//           normalizeAngle(360)  → 0
// -----------------------------------------------------------------------------
float normalizeAngle(float degrees)
{
    float result = std::fmod(degrees, 360.0f);
    if (result < 0.0f) {
        result += 360.0f;
    }
    return result;
}

// =============================================================================
// SECTION 3 — Ellipse Utilities
// =============================================================================

// -----------------------------------------------------------------------------
// ellipsePoint(radiusX, radiusY, angleDeg)
// Returns the (x, y) position on the ellipse at the given angle.
//
// PARAMETRIC ELLIPSE EQUATIONS:
//   x(θ) = radiusX * cos(θ)
//   y(θ) = radiusY * sin(θ)
//
// This maps the parameter θ (in degrees) to a 2D point on the ellipse whose
// semi-major axis is radiusX (along x) and semi-minor axis is radiusY (along y).
//
// Setting radiusX == radiusY gives a circle.
// This is the same formula used in planet.cpp getOrbitX() / getOrbitY().
// -----------------------------------------------------------------------------
Vec2 ellipsePoint(float radiusX, float radiusY, float angleDeg)
{
    float rad = degreesToRadians(angleDeg);
    return Vec2(radiusX * std::cos(rad),
                radiusY * std::sin(rad));
}

// -----------------------------------------------------------------------------
// ellipseTangent(radiusX, radiusY, angleDeg)
// Returns the unit tangent vector to the ellipse at the given angle.
//
// The tangent is the derivative of the position with respect to θ:
//   dx/dθ = -radiusX * sin(θ)
//   dy/dθ =  radiusY * cos(θ)
//
// We normalise this to get a unit direction vector.
// The tangent points in the direction of increasing angle (CCW travel).
//
// Use case: orienting a velocity arrow or an arrow on the orbit path.
// -----------------------------------------------------------------------------
Vec2 ellipseTangent(float radiusX, float radiusY, float angleDeg)
{
    float rad = degreesToRadians(angleDeg);
    Vec2 tangent(-radiusX * std::sin(rad),
                  radiusY * std::cos(rad));
    return tangent.normalized();
}

// =============================================================================
// SECTION 4 — Interpolation & Clamping
// =============================================================================

// -----------------------------------------------------------------------------
// lerp(a, b, t)
// Linear interpolation from a to b by factor t in [0, 1].
//
// GRAPHICS CONCEPT — Keyframe Animation:
//   Given two keyframe values (a at t=0, b at t=1), lerp gives the
//   intermediate value at any fractional time t. It is the mathematical
//   foundation of all tweening and smooth transitions in real-time graphics.
//
// Formula: a + t * (b - a)
//   equivalent to: (1 - t) * a + t * b
// -----------------------------------------------------------------------------
float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

// -----------------------------------------------------------------------------
// smoothstep(a, b, t)
// Smooth ease-in / ease-out interpolation between a and b.
//
// Maps t through the S-curve polynomial:  3t² - 2t³
// This produces zero first-derivative at t=0 and t=1 (starts and ends
// with zero velocity), giving a much more natural feel than linear motion.
//
// GRAPHICS CONCEPT:
//   smoothstep is a built-in function in GLSL (the OpenGL shader language).
//   Understanding it here builds intuition for shader programming later.
//
// Steps:
//   1. Clamp t to [0, 1]
//   2. Apply the smooth curve: t = 3t² - 2t³
//   3. Lerp a to b using the smoothed t
// -----------------------------------------------------------------------------
float smoothstep(float a, float b, float t)
{
    // Clamp t to valid interpolation range
    t = clampf(t, 0.0f, 1.0f);

    // Apply the smoothing polynomial (Ken Perlin's formulation)
    t = t * t * (3.0f - 2.0f * t);

    return lerp(a, b, t);
}

// -----------------------------------------------------------------------------
// clampf(value, minVal, maxVal)
// Clamps a float to the inclusive range [minVal, maxVal].
//
// Used in scaleSize() clamping and smoothstep's t normalisation.
// A manual implementation is provided for compatibility with C++11/14
// (std::clamp is C++17 only).
// -----------------------------------------------------------------------------
float clampf(float value, float minVal, float maxVal)
{
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// =============================================================================
// SECTION 5 — OpenGL Draw Helpers
// =============================================================================
//
// GRAPHICS CONCEPT — Immediate Mode Rendering:
//   In legacy OpenGL, geometry is submitted between glBegin() and glEnd()
//   calls. Each glVertex2f() sends one vertex directly to the GPU.
//   While inefficient for modern hardware, this model makes the relationship
//   between mathematical coordinates and screen positions explicit — ideal
//   for learning the fundamentals.
//
// All helpers below draw at the CURRENT OpenGL origin.
// The caller is responsible for positioning via glTranslatef / glPushMatrix.

// -----------------------------------------------------------------------------
// drawCircle(radius, segments)
// Draws a wireframe circle using GL_LINE_LOOP.
//
// Iterates over `segments` evenly-spaced angles from 0 to 2π, emitting one
// vertex per angle. GL_LINE_LOOP automatically closes the last edge back to
// the first vertex.
//
// Formula for vertex i:
//   angle = (i / segments) * 2π
//   x = radius * cos(angle)
//   y = radius * sin(angle)
// -----------------------------------------------------------------------------
void drawCircle(float radius, int segments)
{
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; ++i) {
            float angle = (static_cast<float>(i) / static_cast<float>(segments))
                          * 2.0f * PI;
            glVertex2f(radius * std::cos(angle),
                       radius * std::sin(angle));
        }
    glEnd();
}

// -----------------------------------------------------------------------------
// drawFilledCircle(radius, segments)
// Draws a solid filled circle using GL_TRIANGLE_FAN.
//
// GRAPHICS CONCEPT — Triangle Fan:
//   GL_TRIANGLE_FAN starts with one central vertex, then forms triangles by
//   connecting each successive pair of perimeter vertices back to the centre.
//   For N perimeter vertices this produces N triangles filling the disk.
//
//   Vertex layout:
//     v0 = centre (0, 0)
//     v1..vN = perimeter, evenly spaced
//     vN+1 = repeat of v1 to close the fan
// -----------------------------------------------------------------------------
void drawFilledCircle(float radius, int segments)
{
    glBegin(GL_TRIANGLE_FAN);
        // Fan centre at the current origin
        glVertex2f(0.0f, 0.0f);

        // Perimeter vertices — go one step past 2π to close the fan cleanly
        for (int i = 0; i <= segments; ++i) {
            float angle = (static_cast<float>(i) / static_cast<float>(segments))
                          * 2.0f * PI;
            glVertex2f(radius * std::cos(angle),
                       radius * std::sin(angle));
        }
    glEnd();
}

// -----------------------------------------------------------------------------
// drawSphere(radius, segments)
// Draws a pseudo-3D sphere as a set of stacked latitude ellipses.
//
// GRAPHICS CONCEPT — Pseudo-3D in 2D:
//   A true 3D sphere requires a 3D projection, depth buffer, and lighting.
//   Here we simulate the appearance using a series of GL_LINE_LOOP ellipses:
//     - The equator is a full circle (radiusX = radiusY = radius)
//     - Latitude bands are compressed ellipses at various y-offsets
//     - The compression factor simulates foreshortening of the sphere's surface
//
//   This gives a recognisable globe appearance with very simple maths.
//
// Number of latitude bands = segments / 4 (e.g. 16 bands for segments=64).
// The vertical prime meridian circle adds a 3D cross-section feel.
// -----------------------------------------------------------------------------
void drawSphere(float radius, int segments)
{
    int latBands = segments / 4;    // number of latitude ellipses

    // Draw latitude bands (horizontal ellipses at increasing y-offsets)
    for (int i = 0; i <= latBands; ++i) {

        // t goes from -1 to +1 across the bands (poles to equator to poles)
        float t = -1.0f + (2.0f * static_cast<float>(i)
                           / static_cast<float>(latBands));

        // y-offset of this band on the sphere surface
        float yOffset    = radius * t;

        // The horizontal radius shrinks toward the poles (cos of latitude angle)
        // This is the key foreshortening that makes it look spherical
        float latAngle   = std::asin(clampf(t, -1.0f, 1.0f));
        float bandRadius = radius * std::cos(latAngle);

        // Draw this latitude ellipse — flat horizontal circle at yOffset
        glBegin(GL_LINE_LOOP);
            for (int j = 0; j < segments; ++j) {
                float angle = (static_cast<float>(j) /
                               static_cast<float>(segments)) * 2.0f * PI;
                glVertex2f(bandRadius * std::cos(angle),
                           yOffset + bandRadius * 0.1f * std::sin(angle));
                // The 0.1f vertical factor compresses the ellipse to suggest
                // the curvature of the sphere when viewed at a slight angle
            }
        glEnd();
    }

    // Draw a vertical meridian circle (the prime meridian cross-section)
    // This gives the sphere its characteristic globe appearance
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; ++i) {
            float angle = (static_cast<float>(i) /
                           static_cast<float>(segments)) * 2.0f * PI;
            glVertex2f(radius * std::cos(angle) * 0.3f,   // narrow vertical oval
                       radius * std::sin(angle));
        }
    glEnd();

    // Draw the equator as a full circle for clarity
    drawCircle(radius, segments);
}

// -----------------------------------------------------------------------------
// drawEllipse(radiusX, radiusY, segments)
// Draws a wireframe ellipse using GL_LINE_LOOP.
//
// The standard parametric ellipse:
//   x(θ) = radiusX * cos(θ)
//   y(θ) = radiusY * sin(θ)
//
// Primary use: rendering planet orbit paths in renderer.cpp.
// Each planet's orbit path is one call to drawEllipse with its own
// orbitRadiusX and orbitRadiusY.
// -----------------------------------------------------------------------------
void drawEllipse(float radiusX, float radiusY, int segments)
{
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; ++i) {
            float angle = (static_cast<float>(i) /
                           static_cast<float>(segments)) * 2.0f * PI;
            glVertex2f(radiusX * std::cos(angle),
                       radiusY * std::sin(angle));
        }
    glEnd();
}

// -----------------------------------------------------------------------------
// drawFilledAnnulus(innerRadius, outerRadius, segments)
// Draws a filled ring (annulus) between two radii using GL_TRIANGLE_STRIP.
//
// GRAPHICS CONCEPT — Triangle Strip for an Annulus:
//   We alternate between placing a vertex on the OUTER ring and a vertex on
//   the INNER ring at the same angle. GL_TRIANGLE_STRIP connects each new
//   pair of vertices to the previous pair, forming a ribbon of triangles
//   that fills the annular region between the two circles.
//
//   Vertex pattern (O = outer, I = inner):
//     O0, I0, O1, I1, O2, I2, ... ON, IN, O0, I0  (close the strip)
//
//   This produces 2*segments triangles from 2*(segments+1) vertices —
//   very efficient compared to a triangle fan approach.
//
// Used for: Saturn's ring system in renderer.cpp.
// The alpha channel of the current glColor4f is used for transparency.
// -----------------------------------------------------------------------------
void drawFilledAnnulus(float innerRadius, float outerRadius, int segments)
{
    glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= segments; ++i) {
            float angle = (static_cast<float>(i) /
                           static_cast<float>(segments)) * 2.0f * PI;

            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            // Outer ring vertex
            glVertex2f(outerRadius * cosA, outerRadius * sinA * 0.3f);

            // Inner ring vertex at the same angle
            // The 0.3f vertical scale flattens the rings to look like a
            // disk viewed at an angle — the classic Saturn ring appearance
            glVertex2f(innerRadius * cosA, innerRadius * sinA * 0.3f);
        }
    glEnd();
}

// =============================================================================
// SECTION 6 — Starfield
// =============================================================================

// -----------------------------------------------------------------------------
// generateStars(count, spread)
// Pre-generates a fixed set of star positions and brightness values.
//
// IMPLEMENTATION DETAILS:
//   - Uses a fixed seed (42) for deterministic output — same star field
//     every run, regardless of machine or timing.
//   - Positions are uniformly distributed in a square [-spread, +spread]
//     on both axes.
//   - Brightness is in [0.5, 1.0] — a minimum of 0.5 ensures all stars
//     are visible while still showing variation.
//   - Stars at the very centre (within 20 units) are rejected to avoid
//     placing stars directly on top of the sun.
//
// Called ONCE in main.cpp at startup.
// The returned vector is stored and passed to drawStars() each frame.
// -----------------------------------------------------------------------------
std::vector<StarPoint> generateStars(int count, float spread)
{
    // Fixed seed — deterministic starfield, looks the same every run
    std::srand(42);

    std::vector<StarPoint> stars;
    stars.reserve(count);

    int attempts = 0;
    const int maxAttempts = count * 10;   // safety cap on the rejection loop

    while (static_cast<int>(stars.size()) < count && attempts < maxAttempts) {

        ++attempts;

        StarPoint s;

        // Random position in [-spread, +spread] on each axis
        s.x = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f * spread
              - spread;
        s.y = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f * spread
              - spread;

        // Reject stars too close to the centre (sun area)
        float distFromCentre = std::sqrt(s.x * s.x + s.y * s.y);
        if (distFromCentre < 20.0f) continue;

        // Random brightness in [0.5, 1.0]
        s.brightness = 0.5f + (static_cast<float>(std::rand()) / RAND_MAX)
                              * 0.5f;

        stars.push_back(s);
    }

    return stars;
}

// -----------------------------------------------------------------------------
// drawStars(stars)
// Renders all pre-generated stars as GL_POINTS.
//
// GRAPHICS CONCEPT — Drawing Order Matters:
//   Stars must be drawn BEFORE any planet transformation matrices are applied.
//   If drawn inside a glPushMatrix/glPopMatrix block, the stars would
//   transform with the solar system — moving as the camera zooms or pans.
//   Drawing them first in world space ensures they remain a fixed backdrop.
//
// Each star is drawn as a single pixel (GL_POINTS with default point size).
// The brightness value is used as all three RGB components, giving a
// white-to-grey range of star colours.
// -----------------------------------------------------------------------------
void drawStars(const std::vector<StarPoint>& stars)
{
    // Slightly increase point size for better visibility on high-DPI displays
    glPointSize(1.5f);

    glBegin(GL_POINTS);
        for (const StarPoint& s : stars) {
            // Grey star: R = G = B = brightness, fully opaque
            glColor3f(s.brightness, s.brightness, s.brightness);
            glVertex2f(s.x, s.y);
        }
    glEnd();

    // Restore default point size
    glPointSize(1.0f);
}