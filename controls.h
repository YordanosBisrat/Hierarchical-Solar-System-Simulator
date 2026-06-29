// =============================================================================
// controls.h
// -----------------------------------------------------------------------------
// Declares the public input handler functions implemented in controls.cpp.
//
// These functions are registered as GLUT callbacks in main.cpp.
// Each one receives the relevant GLUT event parameters plus references to
// the shared SimState and planet vector they are allowed to modify.
// =============================================================================

#ifndef CONTROLS_H
#define CONTROLS_H

#include "renderer.h"   // SimState
#include "planet.h"
#include <vector>

#ifdef __APPLE__
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
#endif

// =============================================================================
// handleKeyboard
// =============================================================================
// Handles standard ASCII key presses (glutKeyboardFunc callback wrapper).
//
// Modifies: simState (speed, pause, zoom, selection)
//           planets  (size scaling via [ and ])
// =============================================================================
void handleKeyboard(unsigned char        key,
                    int                  x,
                    int                  y,
                    SimState&            simState,
                    std::vector<Planet>& planets);

// =============================================================================
// handleSpecialKeys
// =============================================================================
// Handles GLUT special keys: arrow keys, F-keys (glutSpecialFunc wrapper).
//
// Currently maps:
//   UP / DOWN arrows  → zoom in / out
//   LEFT / RIGHT arrows → slow down / speed up
// =============================================================================
void handleSpecialKeys(int       key,
                       int       x,
                       int       y,
                       SimState& simState);

// =============================================================================
// handleMouse
// =============================================================================
// Handles mouse button events including scroll wheel zoom (glutMouseFunc).
//
// Scroll wheel is reported by FreeGLUT as:
//   button 3 = wheel up   (zoom in)
//   button 4 = wheel down (zoom out)
// =============================================================================
void handleMouse(int       button,
                 int       state,
                 int       x,
                 int       y,
                 SimState& simState);

#endif // CONTROLS_H