// =============================================================================
// controls.cpp
// -----------------------------------------------------------------------------
// Implements all user input handling for the Solar System Simulator.
//
// Responsibilities:
//   - Keyboard input  : speed, pause, reverse, zoom, planet selection, scaling
//   - Mouse input     : scroll wheel zoom
//
// DESIGN PRINCIPLE — Controls as Pure State Mutators:
//   This file ONLY modifies SimState and Planet data. It never calls any
//   OpenGL functions, never draws anything, and never advances simulation
//   time. It is a clean input layer — user gestures map to state changes,
//   and the renderer + update loop react to that state next frame.
//
// All callback functions in this file are registered with GLUT in main.cpp
// via glutKeyboardFunc, glutSpecialFunc, and glutMouseFunc.
// =============================================================================

#include "controls.h"
#include "renderer.h"    // SimState definition
#include "planet.h"
#include "math_utils.h"  // clampf

#include <vector>
#include <cmath>         // std::abs

// =============================================================================
// Constants — tuning values for all controls
// =============================================================================

// How much each '+'/'-' keypress multiplies / divides the time scale
static const float SPEED_STEP_FACTOR    = 1.5f;

// Hard limits on time scale magnitude (prevents runaway speed or near-freeze)
static const float MIN_TIME_SCALE       = 0.05f;
static const float MAX_TIME_SCALE       = 20.0f;

// How much each zoom keypress or scroll tick scales the view
static const float ZOOM_STEP_KEYBOARD   = 1.15f;
static const float ZOOM_STEP_SCROLL     = 1.10f;

// Hard limits on zoom level
static const float MIN_ZOOM             = 0.15f;
static const float MAX_ZOOM             = 5.0f;

// How much '[' and ']' scale a planet's size per keypress
static const float PLANET_SCALE_STEP    = 1.15f;

// Default / reset values
static const float DEFAULT_TIME_SCALE   = 1.0f;
static const float DEFAULT_ZOOM         = 1.0f;

// =============================================================================
// handleKeyboard(key, x, y, simState, planets)
// =============================================================================
// Processes standard ASCII keyboard input from GLUT's glutKeyboardFunc.
//
// Key mapping:
//   SPACE   — Toggle pause / resume
//   +  / =  — Increase simulation speed (= included for keyboards without numpad)
//   -       — Decrease simulation speed
//   R / r   — Reverse time direction
//   T / t   — Reset speed to default (1.0x, forward)
//   Z / z   — Zoom in
//   X / x   — Zoom out
//   1 – 8   — Select planet by index
//   [       — Decrease selected planet size
//   ]       — Increase selected planet size
//   ESC     — Exit application
//
// Parameters:
//   key      - ASCII character code of the pressed key
//   x, y     - mouse cursor position at time of keypress (unused)
//   simState - simulation state to modify
//   planets  - planet vector (for selection and size scaling)
// =============================================================================
void handleKeyboard(unsigned char         key,
                    int                   /*x*/,
                    int                   /*y*/,
                    SimState&             simState,
                    std::vector<Planet>&  planets)
{
    switch (key)
    {
        // ---------------------------------------------------------------------
        // SPACE — Pause / Resume
        // ---------------------------------------------------------------------
        // Toggles the paused flag. The timer update in main.cpp checks this
        // flag and skips angle advancement when true. The renderer still runs
        // every frame so the HUD stays responsive while paused.
        // ---------------------------------------------------------------------
        case ' ':
            simState.paused = !simState.paused;
            break;

        // ---------------------------------------------------------------------
        // + / = — Speed Up
        // ---------------------------------------------------------------------
        // Multiplies the magnitude of timeScale by SPEED_STEP_FACTOR while
        // preserving the current sign (direction). This means pressing '+'
        // while reversed speeds up the reverse motion rather than slowing it.
        //
        // We operate on the absolute value and reapply the sign so the
        // direction (positive/negative) is never accidentally flipped here.
        // ---------------------------------------------------------------------
        case '+':
        case '=':   // convenience alias (same physical key, no shift needed)
        {
            float sign      = (simState.timeScale >= 0.0f) ? 1.0f : -1.0f;
            float magnitude = std::abs(simState.timeScale) * SPEED_STEP_FACTOR;
            magnitude       = clampf(magnitude, MIN_TIME_SCALE, MAX_TIME_SCALE);
            simState.timeScale = sign * magnitude;
            break;
        }

        // ---------------------------------------------------------------------
        // - — Slow Down
        // ---------------------------------------------------------------------
        // Divides the magnitude by SPEED_STEP_FACTOR (same as multiplying by
        // 1/factor), preserving sign. Clamped to MIN_TIME_SCALE so the
        // simulation never completely freezes from this key alone.
        // ---------------------------------------------------------------------
        case '-':
        {
            float sign      = (simState.timeScale >= 0.0f) ? 1.0f : -1.0f;
            float magnitude = std::abs(simState.timeScale) / SPEED_STEP_FACTOR;
            magnitude       = clampf(magnitude, MIN_TIME_SCALE, MAX_TIME_SCALE);
            simState.timeScale = sign * magnitude;
            break;
        }

        // ---------------------------------------------------------------------
        // R / r — Reverse Time
        // ---------------------------------------------------------------------
        // Flips the sign of timeScale. All angle updates in planet.cpp use
        //   angle += speed * dt
        // where dt = frameTime * timeScale. A negative timeScale makes dt
        // negative, causing all orbits and rotations to run in reverse.
        // The magnitude (speed) is preserved — only direction changes.
        // ---------------------------------------------------------------------
        case 'R':
        case 'r':
            simState.timeScale = -simState.timeScale;
            break;

        // ---------------------------------------------------------------------
        // T / t — Reset Speed to Default
        // ---------------------------------------------------------------------
        // Restores timeScale to 1.0 (normal forward speed).
        // Also clears the paused flag so the simulation always resumes
        // after a reset — avoids the confusing state of reset + still paused.
        // ---------------------------------------------------------------------
        case 'T':
        case 't':
            simState.timeScale = DEFAULT_TIME_SCALE;
            simState.paused    = false;
            break;

        // ---------------------------------------------------------------------
        // Z / z — Zoom In
        // ---------------------------------------------------------------------
        case 'Z':
        case 'z':
        {
            float newZoom    = simState.zoomLevel * ZOOM_STEP_KEYBOARD;
            simState.zoomLevel = clampf(newZoom, MIN_ZOOM, MAX_ZOOM);
            break;
        }

        // ---------------------------------------------------------------------
        // X / x — Zoom Out
        // ---------------------------------------------------------------------
        case 'X':
        case 'x':
        {
            float newZoom    = simState.zoomLevel / ZOOM_STEP_KEYBOARD;
            simState.zoomLevel = clampf(newZoom, MIN_ZOOM, MAX_ZOOM);
            break;
        }

        // ---------------------------------------------------------------------
        // 1 – 8 — Select Planet
        // ---------------------------------------------------------------------
        // Digits 1–8 map to planet indices 0–7 (Mercury through Neptune).
        // Pressing the same digit again deselects (toggles off).
        // Pressing a digit out of range (if planets vector is smaller) is
        // safely ignored by the bounds check.
        // ---------------------------------------------------------------------
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8':
        {
            int index = static_cast<int>(key - '1');   // '1'→0, '8'→7

            if (index >= 0 && index < static_cast<int>(planets.size())) {
                // Toggle: pressing the same key deselects
                if (simState.selectedPlanet == index) {
                    simState.selectedPlanet = -1;
                } else {
                    simState.selectedPlanet = index;
                }
            }
            break;
        }

        // ---------------------------------------------------------------------
        // [ — Decrease Selected Planet Size
        // ---------------------------------------------------------------------
        // Scales the selected planet down by PLANET_SCALE_STEP.
        // Planet::scaleSize() handles the lower clamp (0.2 * baseSize)
        // so we never shrink a planet to zero here.
        // Does nothing if no planet is selected.
        // ---------------------------------------------------------------------
        case '[':
        {
            if (simState.selectedPlanet >= 0 &&
                simState.selectedPlanet < static_cast<int>(planets.size()))
            {
                planets[simState.selectedPlanet].scaleSize(
                    1.0f / PLANET_SCALE_STEP);
            }
            break;
        }

        // ---------------------------------------------------------------------
        // ] — Increase Selected Planet Size
        // ---------------------------------------------------------------------
        // Scales the selected planet up by PLANET_SCALE_STEP.
        // Planet::scaleSize() handles the upper clamp (5.0 * baseSize).
        // ---------------------------------------------------------------------
        case ']':
        {
            if (simState.selectedPlanet >= 0 &&
                simState.selectedPlanet < static_cast<int>(planets.size()))
            {
                planets[simState.selectedPlanet].scaleSize(PLANET_SCALE_STEP);
            }
            break;
        }

        // ---------------------------------------------------------------------
        // ESC — Exit Application
        // ---------------------------------------------------------------------
        case 27:   // ASCII 27 = Escape
            std::exit(0);
            break;

        default:
            break;   // unrecognised key — silently ignore
    }
}

// =============================================================================
// handleSpecialKeys(key, x, y, simState)
// =============================================================================
// Processes GLUT special keys (arrow keys, F-keys, etc.) from
// glutSpecialFunc. Currently maps arrow keys to zoom for users who prefer
// them over Z/X.
//
// GLUT_KEY_UP    — Zoom in  (same as Z)
// GLUT_KEY_DOWN  — Zoom out (same as X)
// GLUT_KEY_LEFT  — Slow down time
// GLUT_KEY_RIGHT — Speed up time
// =============================================================================
void handleSpecialKeys(int       key,
                       int       /*x*/,
                       int       /*y*/,
                       SimState& simState)
{
    switch (key)
    {
        // Arrow Up — Zoom In
        case GLUT_KEY_UP:
        {
            float newZoom      = simState.zoomLevel * ZOOM_STEP_KEYBOARD;
            simState.zoomLevel = clampf(newZoom, MIN_ZOOM, MAX_ZOOM);
            break;
        }

        // Arrow Down — Zoom Out
        case GLUT_KEY_DOWN:
        {
            float newZoom      = simState.zoomLevel / ZOOM_STEP_KEYBOARD;
            simState.zoomLevel = clampf(newZoom, MIN_ZOOM, MAX_ZOOM);
            break;
        }

        // Arrow Right — Speed Up (mirrors '+' behaviour)
        case GLUT_KEY_RIGHT:
        {
            float sign      = (simState.timeScale >= 0.0f) ? 1.0f : -1.0f;
            float magnitude = std::abs(simState.timeScale) * SPEED_STEP_FACTOR;
            magnitude       = clampf(magnitude, MIN_TIME_SCALE, MAX_TIME_SCALE);
            simState.timeScale = sign * magnitude;
            break;
        }

        // Arrow Left — Slow Down (mirrors '-' behaviour)
        case GLUT_KEY_LEFT:
        {
            float sign      = (simState.timeScale >= 0.0f) ? 1.0f : -1.0f;
            float magnitude = std::abs(simState.timeScale) / SPEED_STEP_FACTOR;
            magnitude       = clampf(magnitude, MIN_TIME_SCALE, MAX_TIME_SCALE);
            simState.timeScale = sign * magnitude;
            break;
        }

        default:
            break;
    }
}

// =============================================================================
// handleMouse(button, state, x, y, simState)
// =============================================================================
// Processes mouse button events from GLUT's glutMouseFunc.
//
// GRAPHICS CONCEPT — Mouse Wheel in GLUT:
//   FreeGLUT reports mouse wheel events as button presses:
//     button 3 = scroll UP   (zoom in)
//     button 4 = scroll DOWN (zoom out)
//   These arrive with state = GLUT_DOWN. We ignore GLUT_UP for wheel events
//   since the scroll wheel has no "release" concept in GLUT's model.
//
//   Regular mouse button clicks (left = 0, middle = 1, right = 2) are
//   received here too. Currently only wheel zoom is implemented, but the
//   structure is in place for future click-to-select functionality.
//
// Parameters:
//   button   - which button was activated (0=left, 1=mid, 2=right, 3/4=wheel)
//   state    - GLUT_DOWN or GLUT_UP
//   x, y     - cursor position in window pixels (y=0 at top)
//   simState - simulation state to modify
// =============================================================================
void handleMouse(int       button,
                 int       state,
                 int       /*x*/,
                 int       /*y*/,
                 SimState& simState)
{
    // Only act on button-down events (ignore releases for wheel)
    if (state != GLUT_DOWN) return;

    switch (button)
    {
        // ---------------------------------------------------------------------
        // Mouse Wheel Up — Zoom In
        // ---------------------------------------------------------------------
        // FreeGLUT encodes scroll-up as button 3.
        // Each tick multiplies zoom by ZOOM_STEP_SCROLL (slightly smaller
        // step than keyboard zoom for finer control with the wheel).
        // ---------------------------------------------------------------------
        case 3:   // scroll wheel up
        {
            float newZoom      = simState.zoomLevel * ZOOM_STEP_SCROLL;
            simState.zoomLevel = clampf(newZoom, MIN_ZOOM, MAX_ZOOM);
            break;
        }

        // ---------------------------------------------------------------------
        // Mouse Wheel Down — Zoom Out
        // ---------------------------------------------------------------------
        // FreeGLUT encodes scroll-down as button 4.
        // ---------------------------------------------------------------------
        case 4:   // scroll wheel down
        {
            float newZoom      = simState.zoomLevel / ZOOM_STEP_SCROLL;
            simState.zoomLevel = clampf(newZoom, MIN_ZOOM, MAX_ZOOM);
            break;
        }

        // Left click, right click, middle click — reserved for future use
        // (e.g. click-to-select planet by world-space hit test)
        case GLUT_LEFT_BUTTON:
        case GLUT_RIGHT_BUTTON:
        case GLUT_MIDDLE_BUTTON:
        default:
            break;
    }
}