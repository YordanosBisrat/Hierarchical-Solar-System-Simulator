// =============================================================================
// renderer.h
// -----------------------------------------------------------------------------
// Declares the public interface of renderer.cpp.
//
// Also defines the SimState struct — the shared simulation state that both
// renderer.cpp (reads for HUD display) and controls.cpp (writes on input)
// need access to. Defining it here avoids a circular dependency.
// =============================================================================

#ifndef RENDERER_H
#define RENDERER_H

#include "planet.h"
#include "math_utils.h"
#include <vector>

// =============================================================================
// SimState
// -----------------------------------------------------------------------------
// Holds all top-level simulation control variables.
// Owned by main.cpp, passed by reference to renderer and controls.
//
// Using a struct (rather than globals) keeps all mutable state in one place
// and makes data flow explicit — every function that touches simulation state
// must receive this struct, making dependencies clear.
// =============================================================================
struct SimState {
    float timeScale;        // animation speed multiplier (negative = reversed)
    bool  paused;           // true = animation frozen
    float zoomLevel;        // camera zoom (1.0 = default view)
    int   selectedPlanet;   // index into planets vector (-1 = none selected)
    float elapsedTime;      // total simulation time in seconds (for animations)

    // Camera rotation (mouse drag rotates entire solar system around Z axis)
    float sceneRotation;    // degrees, accumulated from mouse drag

    // Camera pan (middle-mouse drag or WASD)
    float panX;             // world-space horizontal offset
    float panY;             // world-space vertical offset

    // Mouse drag state (not serialised — runtime only)
    bool  isDragging;       // true while left mouse button is held
    int   dragStartX;       // window pixel X where drag began
    int   dragStartY;       // window pixel Y where drag began
    float dragStartRotation;// sceneRotation value when drag started
    bool  isPanning;        // true while middle mouse button is held
    int   panStartX;
    int   panStartY;
    float panStartWorldX;
    float panStartWorldY;

    // Constructor — sensible defaults
    SimState()
        : timeScale(1.0f)
        , paused(false)
        , zoomLevel(1.0f)
        , selectedPlanet(-1)
        , elapsedTime(0.0f)
        , sceneRotation(0.0f)
        , panX(0.0f)
        , panY(0.0f)
        , isDragging(false)
        , dragStartX(0)
        , dragStartY(0)
        , dragStartRotation(0.0f)
        , isPanning(false)
        , panStartX(0)
        , panStartY(0)
        , panStartWorldX(0.0f)
        , panStartWorldY(0.0f)
    {}
};

// =============================================================================
// Renderer function declarations
// =============================================================================

// Clear the colour/depth buffers and reset modelview to identity
void clearScene();

// Apply zoom level to the modelview matrix
void setupCamera(float zoom);

// Render the pre-generated starfield background
void drawStarfield(const std::vector<StarPoint>& stars);

// Render all planet and moon orbit path ellipses
void drawOrbitPaths(const std::vector<Planet>& planets);

// Render the full solar system (sun + all planets + moons + selection ring)
void drawSolarSystem(const Planet&              sun,
                     const std::vector<Planet>& planets,
                     int                        selectedIndex,
                     float                      zoom,
                     float                      time);

// Render the 2D HUD overlay (status panel + controls reference)
void drawHUD(const SimState&            simState,
             const std::vector<Planet>& planets,
             int                        windowW,
             int                        windowH);

#endif // RENDERER_H