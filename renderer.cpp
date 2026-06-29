// =============================================================================
// renderer.cpp
// -----------------------------------------------------------------------------
// Owns every OpenGL draw call that produces the simulation's visual output.
//
// Rendering pipeline (called once per frame from main.cpp):
//   1. clearScene()          — clear colour and depth buffers
//   2. drawStarfield()       — fixed background stars (world space)
//   3. drawSolarSystem()     — orbit paths + sun + planet/moon hierarchy + HUD
//
// GRAPHICS CONCEPT — Separation of Read and Write:
//   This file READS planet state (angles, size, color) but NEVER modifies it.
//   All state mutation happens in controls.cpp (user input) and planet.cpp
//   (update logic). The renderer is a pure observer — given the same planet
//   state it will always produce the same frame.
// =============================================================================

#include "renderer.h"
#include "planet.h"
#include "math_utils.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
#endif

#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>   // std::fixed, std::setprecision

// =============================================================================
// Internal helpers — not exposed in renderer.h
// =============================================================================

// Draw a null-terminated string at (x, y) in the current projection space
// using GLUT's bitmap font. Used for HUD text and planet name labels.
static void drawText(float x, float y, const std::string& text,
                     void* font = GLUT_BITMAP_8_BY_13)
{
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

// Draw a larger heading string using a bigger GLUT font
static void drawTextLarge(float x, float y, const std::string& text)
{
    drawText(x, y, text, GLUT_BITMAP_HELVETICA_18);
}

// Draw a selection highlight ring around a planet at its current world position
// Pulsing effect is achieved by modulating the ring radius with a sine wave
static void drawSelectionHighlight(const Planet& planet, float time)
{
    float wx = planet.getOrbitX();
    float wy = planet.getOrbitY();

    // Pulsing ring: radius oscillates between size*1.8 and size*2.4
    float pulse  = 1.8f + 0.3f * std::sin(time * 4.0f);
    float radius = planet.size * pulse;

    glPushMatrix();
        glTranslatef(wx, wy, 0.0f);

        // Bright yellow selection ring
        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(1.5f);
        drawCircle(radius);
        glLineWidth(1.0f);
    glPopMatrix();
}

// =============================================================================
// clearScene()
// =============================================================================
// Clears the colour buffer to deep space black and resets the modelview matrix
// to the identity. Called at the very start of each frame.
//
// GRAPHICS CONCEPT — Double Buffering:
//   glutSwapBuffers() (called in main.cpp after rendering) swaps the back
//   buffer (where we draw) with the front buffer (what the screen shows).
//   glClear wipes the back buffer clean for the new frame.
// =============================================================================
void clearScene()
{
    // Deep space black background
    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset modelview to identity — fresh slate for this frame's transforms
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// =============================================================================
// setupCamera(zoom)
// =============================================================================
// NOTE: In this project, zoom is handled via glOrtho in updateProjection()
// (main.cpp), and rotation/pan are applied in display() before calling
// drawSolarSystem(). This function is intentionally a no-op — the camera
// transform is already on the modelview stack when drawing begins.
// It is kept in the interface for structural completeness.
// =============================================================================
void setupCamera(float /*zoom*/)
{
    // Camera transform applied in display() via glTranslatef + glRotatef
    // before drawSolarSystem() is called. Nothing to do here.
}

// =============================================================================
// drawStarfield(stars)
// =============================================================================
// Renders the pre-generated background star field.
//
// GRAPHICS CONCEPT — Draw Order / Painter's Algorithm:
//   Stars must be drawn with a clean identity matrix so they are NOT affected
//   by the zoom. They represent the infinite fixed background — they should
//   fill the screen evenly regardless of how much the user zooms in or out.
//   main.cpp calls this BEFORE setupCamera() so the zoom scale is not yet set.
// =============================================================================
void drawStarfield(const std::vector<StarPoint>& stars)
{
    drawStars(stars);
}

// =============================================================================
// drawOrbitPaths(planets)
// =============================================================================
// Draws the elliptical orbit path for every planet (and moon).
// Orbit paths are rendered faintly so they guide the eye without dominating.
//
// CRITICAL: This must be called AFTER setupCamera() so orbit paths are drawn
// in the same zoomed coordinate space as the planets themselves.
// If called before setupCamera(), orbits appear at a different scale than
// the planets and visually disconnect from them on zoom.
//
// GRAPHICS CONCEPT — Drawing Orbits in World Space:
//   Orbit paths are centred on the sun (world origin), so we draw them with
//   no translation — just the ellipse centred at (0,0). Each planet's
//   orbitRadiusX and orbitRadiusY define the ellipse shape.
//
// Moon orbits are drawn relative to their parent planet's CURRENT position,
// so we call getOrbitX()/getOrbitY() to find where the planet currently is
// and translate to that point before drawing the moon's orbit ellipse.
// =============================================================================
void drawOrbitPaths(const std::vector<Planet>& planets)
{
    // Orbit paths are drawn in the same world-space coordinate system as
    // the planets. Zoom is in the projection, so all world coordinates
    // scale uniformly — orbits and planets always stay aligned.
    for (const Planet& planet : planets) {

        if (planet.orbitRadiusX <= 0.0f) continue;   // skip the sun

        // Planet orbit ellipse — centred at world origin (sun)
        glColor4f(0.35f, 0.35f, 0.45f, 0.6f);
        glLineWidth(0.8f);
        drawEllipse(planet.orbitRadiusX, planet.orbitRadiusY);

        // Moon orbit ellipses — centred on the planet's current position.
        // We use the SAME getOrbitX/Y that drawPlanetHierarchy uses to place
        // the planet, so orbit circles are guaranteed to match planet positions.
        if (!planet.moons.empty()) {
            for (const Planet& moon : planet.moons) {
                glPushMatrix();
                    // Translate to planet centre (matches drawPlanetHierarchy)
                    glTranslatef(planet.getOrbitX(), planet.getOrbitY(), 0.0f);
                    // Draw moon orbit ellipse relative to planet centre
                    glColor4f(0.25f, 0.25f, 0.35f, 0.5f);
                    drawEllipse(moon.orbitRadiusX, moon.orbitRadiusY);
                glPopMatrix();
            }
        }
    }

    glLineWidth(1.0f);
}

// =============================================================================
// drawRings(planet)
// =============================================================================
// Draws Saturn's ring system as a filled annulus.
//
// GRAPHICS CONCEPT — Rings in the Planet's Own Frame:
//   This function is called INSIDE Saturn's glPushMatrix() block after the
//   self-rotation has been applied. That means the ring disk automatically
//   inherits Saturn's rotation — it spins with the planet for free.
//
// We enable alpha blending temporarily to render the semi-transparent rings,
// then disable it to avoid affecting subsequent draw calls.
// =============================================================================
static void drawRings(const Planet& planet)
{
    if (!planet.hasRings) return;

    const RingParams& r = planet.rings;

    // Enable alpha blending for transparent ring appearance
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set ring colour with alpha from ring params
    glColor4f(r.color[0], r.color[1], r.color[2], r.color[3]);

    // Draw the filled annulus ring disk
    drawFilledAnnulus(r.innerRadius, r.outerRadius);

    // Draw a wireframe outline on the rings for definition
    glColor4f(r.color[0] * 0.7f, r.color[1] * 0.7f, r.color[2] * 0.6f, 0.8f);
    drawEllipse(r.outerRadius, r.outerRadius * 0.3f);
    drawEllipse(r.innerRadius, r.innerRadius * 0.3f);

    glDisable(GL_BLEND);
}

// =============================================================================
// drawCelestialBody(planet)
// =============================================================================
// Draws a single celestial body (planet or moon) as a pseudo-3D sphere with
// its name label. Does NOT handle orbital positioning — that is handled by
// the caller via glPushMatrix / glTranslatef before calling this.
//
// This function assumes the current matrix is already centred on the body.
// It applies ONLY the self-rotation before drawing.
// =============================================================================
static void drawCelestialBody(const Planet& planet)
{
    glPushMatrix();

        // Apply axial self-rotation around the Z axis
        // GRAPHICS CONCEPT: This rotation is applied AFTER the orbital
        // translation, so it spins the planet around its own centre —
        // not around the sun. The matrix stack handles this automatically.
        glRotatef(planet.selfRotationAngle, 0.0f, 0.0f, 1.0f);

        // Draw rings BEHIND the planet body (drawn first = further back)
        if (planet.hasRings) {
            drawRings(planet);
        }

        // Set the planet's colour
        glColor3f(planet.color[0], planet.color[1], planet.color[2]);

        // Draw the planet as a filled circle (body)
        drawFilledCircle(planet.size);

        // Draw the sphere overlay (latitude/meridian lines) in a slightly
        // darker shade to give depth and a recognisable globe appearance
        glColor3f(planet.color[0] * 0.6f,
                  planet.color[1] * 0.6f,
                  planet.color[2] * 0.6f);
        drawSphere(planet.size);

        // Draw rings IN FRONT of the planet body for the front half
        if (planet.hasRings) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            const RingParams& r = planet.rings;
            glColor4f(r.color[0], r.color[1], r.color[2], r.color[3] * 0.5f);
            drawFilledAnnulus(r.innerRadius, planet.size * 1.05f);
            glDisable(GL_BLEND);
        }

    glPopMatrix();

    // Draw name label slightly above the planet
    // Label is drawn OUTSIDE the self-rotation push/pop so it stays upright
    glColor3f(0.85f, 0.85f, 0.85f);
    drawText(-static_cast<float>(planet.name.size()) * 2.0f,
             planet.size + 2.5f,
             planet.name,
             GLUT_BITMAP_8_BY_13);
}

// =============================================================================
// drawPlanetHierarchy(planet)
// =============================================================================
// Recursively draws a planet and all its moons using the OpenGL matrix stack.
//
// GRAPHICS CONCEPT — Hierarchical Transformation (THE CORE OF THIS PROJECT):
//
//   IMPORTANT FIX — Zoom Preservation:
//   We use glPushMatrix/glPopMatrix around each planet instead of calling
//   glLoadIdentity(). Calling glLoadIdentity() would wipe the zoom scale
//   set by setupCamera(), causing planets to render at world scale while
//   orbit paths render at zoom scale — making them visually disconnect.
//
//   By using push/pop we PRESERVE the zoom scale from the parent matrix
//   while still isolating each planet's own position transforms.
//
//   The transformation sequence for each planet:
//
//     glPushMatrix()                       ← save zoomed world origin
//       glTranslatef(orbitX, orbitY, 0)    ← move to ellipse position
//       drawCelestialBody(planet)          ← draw planet (self-rotation inside)
//       for each moon:
//           glPushMatrix()                 ← save planet's position
//             glRotatef(moon.orbit, Z)     ← rotate around planet
//             glTranslatef(moon.X, 0, 0)   ← move out from planet
//             drawCelestialBody(moon)      ← draw moon
//           glPopMatrix()                  ← restore to planet's position
//     glPopMatrix()                        ← restore to zoomed world origin
// =============================================================================
static void drawPlanetHierarchy(const Planet& planet)
{
    // =========================================================================
    // GRAPHICS CONCEPT — Hierarchical Transformation with glPushMatrix/glPopMatrix
    //
    // Zoom lives in the projection (glOrtho). The modelview is identity.
    // We place every body using glTranslatef to its parametric ellipse position:
    //   planet pos = (orbitRadiusX*cos(angle), orbitRadiusY*sin(angle))
    //   moon pos   = planet pos + (moonRadiusX*cos(moonAngle), moonRadiusY*sin(moonAngle))
    //
    // Moons are drawn INSIDE the planet's glPushMatrix block so their
    // getOrbitX/Y is added on top of the planet's translation automatically.
    // This is the core of hierarchical modeling — child coordinates are
    // expressed relative to the parent's current coordinate frame.
    // =========================================================================

    glPushMatrix();                                              // SAVE: world origin
        glTranslatef(planet.getOrbitX(), planet.getOrbitY(), 0.0f); // move to planet

        // Draw this planet (self-rotation handled inside drawCelestialBody)
        drawCelestialBody(planet);

        // Draw moons — still inside planet's pushed frame.
        // getOrbitX/Y of each moon gives its offset FROM the planet centre.
        for (const Planet& moon : planet.moons) {
            glPushMatrix();                                          // SAVE: planet pos
                glTranslatef(moon.getOrbitX(), moon.getOrbitY(), 0.0f); // moon offset
                drawCelestialBody(moon);
            glPopMatrix();                                           // RESTORE: planet pos
        }

    glPopMatrix();                                               // RESTORE: world origin
}

// =============================================================================
// drawSun(sun)
// =============================================================================
// Draws the sun at the world origin (0,0) with a glow effect.
// The sun does not orbit anything, so no orbital rotation is applied.
// It still self-rotates slowly for visual interest.
// =============================================================================
static void drawSun(const Planet& sun)
{
    glPushMatrix();
        // Self-rotation (slow axial spin for visual interest)
        glRotatef(sun.selfRotationAngle, 0.0f, 0.0f, 1.0f);

        // Glow layers — concentric circles with decreasing alpha
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(1.0f, 0.7f, 0.0f, 0.08f);
        drawFilledCircle(sun.size * 2.5f);

        glColor4f(1.0f, 0.75f, 0.0f, 0.12f);
        drawFilledCircle(sun.size * 2.0f);

        glColor4f(1.0f, 0.8f, 0.0f, 0.20f);
        drawFilledCircle(sun.size * 1.6f);

        glColor4f(1.0f, 0.85f, 0.0f, 0.35f);
        drawFilledCircle(sun.size * 1.3f);

        glDisable(GL_BLEND);

        // Sun body — bright yellow-orange filled circle
        glColor3f(sun.color[0], sun.color[1], sun.color[2]);
        drawFilledCircle(sun.size);

        // Surface detail lines (darker orange)
        glColor3f(0.9f, 0.55f, 0.0f);
        drawSphere(sun.size);

    glPopMatrix();

    // Sun label
    glColor3f(1.0f, 0.9f, 0.4f);
    drawText(-8.0f, sun.size + 3.0f, "Sun", GLUT_BITMAP_HELVETICA_18);
}

// =============================================================================
// drawSolarSystem(sun, planets, selectedIndex, zoom, time)
// =============================================================================
// Top-level scene draw function. Draws orbit paths, sun, all planets and
// their moons hierarchically, and the selection highlight if active.
//
// ZOOM FIX — Correct draw order:
//   1. setupCamera(zoom)    ← sets zoom scale on modelview ONCE
//   2. drawOrbitPaths()     ← drawn IN the zoomed space
//   3. drawSun()            ← drawn IN the zoomed space
//   4. drawPlanetHierarchy()← drawn IN the zoomed space (uses push/pop only)
//   5. selection highlight  ← drawn IN the zoomed space
//
//   All drawing happens inside the same zoom-scaled modelview frame.
//   Nothing calls glLoadIdentity() after setupCamera — only push/pop is used.
// =============================================================================
void drawSolarSystem(const Planet&              sun,
                     const std::vector<Planet>& planets,
                     int                        selectedIndex,
                     float                      /*zoom*/,
                     float                      time)
{
    // Zoom is applied via glOrtho in updateProjection() (main.cpp).
    // The modelview is already identity when this function is called.
    // Simply draw everything in world space — the projection handles zoom.

    drawOrbitPaths(planets);
    drawSun(sun);

    for (const Planet& planet : planets) {
        drawPlanetHierarchy(planet);
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(planets.size())) {
        drawSelectionHighlight(planets[selectedIndex], time);
    }
}

// =============================================================================
// drawHUD(simState, planets, windowW, windowH)
// =============================================================================
// Draws the 2D heads-up display overlay showing simulation status.
//
// GRAPHICS CONCEPT — HUD Projection Switch:
//   The main scene uses a world-space coordinate system (world units centred
//   at origin). The HUD needs to draw at fixed pixel positions regardless of
//   zoom. We achieve this by temporarily switching to an orthographic
//   pixel-space projection (0,0) to (windowW, windowH), drawing the text,
//   then restoring the original projection.
// =============================================================================
void drawHUD(const SimState&            simState,
             const std::vector<Planet>& planets,
             int                        windowW,
             int                        windowH)
{
    // --- Switch to pixel-space 2D projection ---
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity();
        // Map (0,0) bottom-left to (windowW, windowH) top-right
        glOrtho(0.0, windowW, 0.0, windowH, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
            glLoadIdentity();

            // Disable depth test so HUD always appears on top
            glDisable(GL_DEPTH_TEST);

            // ---------------------------------------------------------------
            // Title (top centre)
            // ---------------------------------------------------------------
            glColor3f(0.9f, 0.85f, 1.0f);
            drawTextLarge(windowW / 2.0f - 130.0f, windowH - 25.0f,
                          "Hierarchical Solar System Simulator");

            // ---------------------------------------------------------------
            // Status panel (top left)
            // ---------------------------------------------------------------
            float panelX = 12.0f;
            float panelY = static_cast<float>(windowH) - 50.0f;
            float lineH  = 18.0f;

            // Simulation speed
            std::ostringstream speedStr;
            speedStr << "Speed: " << std::fixed << std::setprecision(2)
                     << std::abs(simState.timeScale) << "x";
            if (simState.timeScale < 0.0f) speedStr << "  [REVERSED]";
            glColor3f(0.5f, 1.0f, 0.6f);
            drawText(panelX, panelY, speedStr.str());
            panelY -= lineH;

            // Pause status
            if (simState.paused) {
                glColor3f(1.0f, 0.5f, 0.3f);
                drawText(panelX, panelY, "Status: PAUSED");
            } else {
                glColor3f(0.5f, 1.0f, 0.6f);
                drawText(panelX, panelY, "Status: Running");
            }
            panelY -= lineH;

            // Zoom level
            std::ostringstream zoomStr;
            zoomStr << "Zoom: " << std::fixed << std::setprecision(2)
                    << simState.zoomLevel << "x";
            glColor3f(0.5f, 0.8f, 1.0f);
            drawText(panelX, panelY, zoomStr.str());
            panelY -= lineH;

            // Scene rotation
            std::ostringstream rotStr;
            rotStr << "Rotation: " << std::fixed << std::setprecision(1)
                   << simState.sceneRotation << " deg";
            glColor3f(0.8f, 0.7f, 1.0f);
            drawText(panelX, panelY, rotStr.str());
            panelY -= lineH;

            // Elapsed simulation time
            std::ostringstream timeStr;
            timeStr << "Sim time: " << std::fixed << std::setprecision(1)
                    << simState.elapsedTime << "s";
            glColor3f(0.7f, 0.9f, 0.7f);
            drawText(panelX, panelY, timeStr.str());
            panelY -= lineH;

            // Selected planet
            if (simState.selectedPlanet >= 0 &&
                simState.selectedPlanet < static_cast<int>(planets.size()))
            {
                const Planet& sel = planets[simState.selectedPlanet];
                std::ostringstream selStr;
                selStr << "Selected: " << sel.name
                       << "  (size: " << std::fixed << std::setprecision(2)
                       << sel.size << ")";
                glColor3f(1.0f, 1.0f, 0.4f);
                drawText(panelX, panelY, selStr.str());
            } else {
                glColor3f(0.6f, 0.6f, 0.6f);
                drawText(panelX, panelY, "Selected: None");
            }

            // ---------------------------------------------------------------
            // Controls reference (bottom left)
            // ---------------------------------------------------------------
            float ctrlX = 12.0f;
            float ctrlY = 160.0f;
            float ctrlH = 16.0f;

            glColor3f(0.7f, 0.7f, 0.9f);
            drawText(ctrlX, ctrlY, "--- Controls ---");
            ctrlY -= ctrlH;

            glColor3f(0.75f, 0.75f, 0.75f);
            drawText(ctrlX, ctrlY, "SPACE   Pause / Resume");       ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "+ / -   Speed up / Slow down"); ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "R       Reverse time");         ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "T       Reset speed");          ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "Z / X   Zoom in / out");        ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "LMB drag  Rotate scene");       ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "MMB drag  Pan camera");         ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "RMB     Reset camera");         ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "C       Reset camera (key)");   ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "1-8     Select planet");        ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "[ / ]   Scale planet size");    ctrlY -= ctrlH;
            drawText(ctrlX, ctrlY, "ESC     Exit");

            // ---------------------------------------------------------------
            // Planet index reference (bottom right)
            // ---------------------------------------------------------------
            float idxX = static_cast<float>(windowW) - 160.0f;
            float idxY = 150.0f;

            glColor3f(0.7f, 0.7f, 0.9f);
            drawText(idxX, idxY, "--- Planets ---");
            idxY -= ctrlH;

            const char* planetNames[] = {
                "1: Mercury", "2: Venus",   "3: Earth",   "4: Mars",
                "5: Jupiter", "6: Saturn",  "7: Uranus",  "8: Neptune"
            };
            for (int i = 0; i < 8; ++i) {
                if (simState.selectedPlanet == i) {
                    glColor3f(1.0f, 1.0f, 0.3f);
                } else {
                    glColor3f(0.7f, 0.7f, 0.7f);
                }
                drawText(idxX, idxY, planetNames[i]);
                idxY -= ctrlH;
            }

            glEnable(GL_DEPTH_TEST);

        glPopMatrix();   // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();       // projection
    glMatrixMode(GL_MODELVIEW);
}