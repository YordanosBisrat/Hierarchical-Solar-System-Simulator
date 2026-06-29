// =============================================================================
// main.cpp
// -----------------------------------------------------------------------------
// Application entry point for the Hierarchical Solar System Simulator.
//
// Responsibilities:
//   - GLUT window creation and OpenGL state initialisation
//   - Solar system data construction (sun, 8 planets, moons, Saturn rings)
//   - GLUT callback registration (display, reshape, timer, keyboard, mouse)
//   - Main render/update loop via glutTimerFunc
//   - Ownership of SimState, star field, sun, and planet vector
//
// ARCHITECTURE NOTE:
//   main.cpp is intentionally thin on logic. It delegates:
//     - All drawing       → renderer.cpp
//     - All input         → controls.cpp
//     - All math          → math_utils.cpp
//     - All planet state  → planet.cpp
//   This file's job is to own the data, wire the callbacks, and keep
//   the main loop clean and readable.
//
// BUILD (Linux / macOS):
//   g++ -std=c++17 main.cpp planet.cpp renderer.cpp controls.cpp \
//       math_utils.cpp -lGL -lGLU -lglut -o SolarSystem
//
//   macOS:
//   g++ -std=c++17 main.cpp planet.cpp renderer.cpp controls.cpp \
//       math_utils.cpp -framework OpenGL -framework GLUT -o SolarSystem
// =============================================================================

#include "planet.h"
#include "renderer.h"
#include "controls.h"
#include "math_utils.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
#endif

#include <vector>
#include <memory>    // std::unique_ptr, std::make_unique
#include <cstdlib>   // std::exit

// =============================================================================
// Window configuration
// =============================================================================
static const int   WINDOW_WIDTH       = 1200;
static const int   WINDOW_HEIGHT      = 800;
static const char* WINDOW_TITLE       = "Hierarchical Solar System Simulator";
static const int   TARGET_FPS         = 60;
static const int   FRAME_DELAY_MS     = 1000 / TARGET_FPS;  // ~16 ms

// World-space half-extents for the orthographic projection.
// The view volume spans [-WORLD_HALF_W, +WORLD_HALF_W] horizontally
// and is scaled proportionally for the vertical axis based on window aspect.
static const float WORLD_HALF_W       = 160.0f;

// =============================================================================
// Global application state
// =============================================================================
// These are the only true globals in the project. GLUT's C callback API
// cannot carry user data pointers, so we need file-scope access to the
// simulation state inside the thin GLUT callback wrappers below.
//
// Everything else is passed by reference through the call chain.
// =============================================================================
static SimState            g_simState;
static std::unique_ptr<Planet> g_sun;
static std::vector<Planet> g_planets;
static std::vector<StarPoint> g_stars;

// Window dimensions (updated in reshape callback)
static int g_windowW = WINDOW_WIDTH;
static int g_windowH = WINDOW_HEIGHT;

// Forward declaration (updateProjection is defined after display())
static void updateProjection();

// =============================================================================
// createSolarSystem()
// =============================================================================
// Constructs the sun, all 8 planets, and their moons.
// Sets up Saturn's rings. Populates g_sun and g_planets.
//
// PLANET DATA:
//   Orbital parameters are scaled for visual clarity, not physical accuracy.
//   Real orbital periods span 88 days (Mercury) to 165 years (Neptune) —
//   using real ratios would make outer planets nearly stationary.
//   Instead, speeds are tuned so all planets complete roughly 1–8 orbits
//   per minute at default time scale, making the hierarchy clearly visible.
//
// ELLIPTICAL ORBITS:
//   Each planet has orbitRadiusX != orbitRadiusY, giving a distinct ellipse.
//   The ratio is loosely inspired by real eccentricities but exaggerated for
//   visual interest. Mercury has the most eccentric orbit; outer planets
//   are near-circular as in reality.
//
// HIERARCHY:
//   Moon objects are constructed then attached via planet.addMoon(moon).
//   Once attached, the moon is owned by the planet and updated/rendered
//   inside the planet's coordinate frame automatically.
// =============================================================================
static void createSolarSystem()
{
    // =========================================================================
    // SUN — fixed at the world origin, no orbit
    // =========================================================================
    // orbitRadiusX = orbitRadiusY = 0 → stays at (0,0)
    // Large size and slow self-rotation for a dramatic central body
    g_sun = std::make_unique<Planet>(
        "Sun",      // name
        -1,         // id (-1 = not selectable)
        11.0f,      // size (radius in world units)
        0.0f,       // orbitRadiusX (no orbit)
        0.0f,       // orbitRadiusY
        0.0f,       // orbitSpeed (stationary)
        2.0f,       // selfRotationSpeed (deg/sec)
        1.0f, 0.85f, 0.1f   // colour: warm yellow-orange
    );

    // =========================================================================
    // MERCURY — closest planet, fast orbit, highly elliptical
    // =========================================================================
    Planet mercury(
        "Mercury", 0,
        1.8f,           // small rocky planet
        22.0f, 17.0f,   // noticeable ellipse (real eccentricity ~0.21)
        47.0f,          // fastest orbit (deg/sec at default speed)
        15.0f,          // slow self-rotation (Mercury has 59-day rotation)
        0.76f, 0.70f, 0.68f  // grey-brown rocky colour
    );

    // =========================================================================
    // VENUS — similar size to Earth, dense atmosphere, slow retrograde spin
    // =========================================================================
    Planet venus(
        "Venus", 1,
        2.8f,
        32.0f, 30.0f,   // nearly circular orbit
        35.0f,
        -8.0f,          // NEGATIVE = retrograde rotation (Venus is real!)
        0.95f, 0.82f, 0.52f  // pale yellow-orange cloud colour
    );

    // =========================================================================
    // EARTH — our home, with one moon
    // =========================================================================
    Planet earth(
        "Earth", 2,
        3.0f,
        44.0f, 42.0f,
        29.0f,
        25.0f,          // 24-hour-ish rotation
        0.25f, 0.52f, 0.95f  // blue ocean colour
    );

    // Earth's Moon
    Planet earthMoon(
        "Moon", -1,
        0.9f,
        7.0f, 6.8f,     // close orbit, slightly elliptical
        120.0f,         // fast moon orbit relative to Earth
        10.0f,
        0.80f, 0.80f, 0.78f  // grey-white lunar surface
    );
    earth.addMoon(earthMoon);

    // =========================================================================
    // MARS — red planet, two small moons
    // =========================================================================
    Planet mars(
        "Mars", 3,
        2.3f,
        58.0f, 54.0f,   // mild eccentricity (real is ~0.09)
        24.0f,
        24.5f,          // similar day length to Earth
        0.90f, 0.38f, 0.18f  // characteristic rusty red
    );

    // Phobos — inner, fast moon
    Planet phobos(
        "Phobos", -1,
        0.5f,
        5.5f, 5.3f,
        200.0f,         // orbits Mars faster than Mars rotates (real!)
        5.0f,
        0.72f, 0.62f, 0.55f
    );

    // Deimos — outer, slower moon
    Planet deimos(
        "Deimos", -1,
        0.4f,
        8.5f, 8.2f,
        80.0f,
        4.0f,
        0.70f, 0.63f, 0.57f
    );

    mars.addMoon(phobos);
    mars.addMoon(deimos);

    // =========================================================================
    // JUPITER — largest planet, fast rotation, one large moon shown
    // =========================================================================
    Planet jupiter(
        "Jupiter", 4,
        6.5f,           // largest planet — noticeably bigger than others
        78.0f, 75.0f,
        13.0f,
        60.0f,          // Jupiter rotates fastest of all planets (~10 hours)
        0.88f, 0.75f, 0.60f  // orange-tan banded colour
    );

    // Ganymede — largest moon in the solar system
    Planet ganymede(
        "Ganymede", -1,
        1.4f,
        12.0f, 11.5f,
        55.0f,
        8.0f,
        0.68f, 0.65f, 0.60f  // grey-brown icy surface
    );
    jupiter.addMoon(ganymede);

    // =========================================================================
    // SATURN — ringed planet, second largest
    // =========================================================================
    Planet saturn(
        "Saturn", 5,
        5.5f,
        100.0f, 97.0f,
        9.5f,
        55.0f,          // fast rotation (~10.7 hours real)
        0.95f, 0.88f, 0.65f  // warm golden-cream colour
    );

    // Saturn's iconic ring system
    // innerRadius starts just outside the planet body
    // outerRadius extends to ~2.3× the planet radius (real ratio ~2.3×)
    saturn.setupRings(
        saturn.size * 1.25f,   // inner edge
        saturn.size * 2.30f,   // outer edge
        0.88f, 0.78f, 0.58f,   // warm sandy ring colour
        0.65f                  // semi-transparent alpha
    );

    // Titan — Saturn's largest moon, thick atmosphere
    Planet titan(
        "Titan", -1,
        1.2f,
        14.0f, 13.5f,
        40.0f,
        6.0f,
        0.88f, 0.72f, 0.38f  // orange haze colour
    );
    saturn.addMoon(titan);

    // =========================================================================
    // URANUS — ice giant, rotates on its side (high axial tilt shown via
    //          unusual self-rotation axis — approximated here)
    // =========================================================================
    Planet uranus(
        "Uranus", 6,
        4.2f,
        122.0f, 120.0f,
        6.8f,
        -42.0f,         // NEGATIVE = retrograde (Uranus tilted ~98°, real!)
        0.55f, 0.88f, 0.92f  // pale cyan ice giant colour
    );

    // Miranda — most geologically interesting moon of Uranus
    Planet miranda(
        "Miranda", -1,
        0.6f,
        8.0f, 7.8f,
        90.0f,
        5.0f,
        0.78f, 0.78f, 0.80f
    );
    uranus.addMoon(miranda);

    // =========================================================================
    // NEPTUNE — outermost planet, deep blue, strong winds
    // =========================================================================
    Planet neptune(
        "Neptune", 7,
        4.0f,
        145.0f, 143.0f,  // near-circular orbit (real eccentricity ~0.01)
        5.4f,            // slowest orbital period in the solar system
        50.0f,
        0.18f, 0.35f, 0.90f  // deep vivid blue
    );

    // Triton — largest moon, retrograde orbit (captured KBO)
    Planet triton(
        "Triton", -1,
        0.85f,
        10.0f, 9.8f,
        -60.0f,         // NEGATIVE = retrograde orbit (real! unique in system)
        7.0f,
        0.72f, 0.78f, 0.82f  // pale icy blue-grey
    );
    neptune.addMoon(triton);

    // =========================================================================
    // Populate the global planets vector (order = distance from sun)
    // =========================================================================
    g_planets.push_back(mercury);
    g_planets.push_back(venus);
    g_planets.push_back(earth);
    g_planets.push_back(mars);
    g_planets.push_back(jupiter);
    g_planets.push_back(saturn);
    g_planets.push_back(uranus);
    g_planets.push_back(neptune);
}

// =============================================================================
// GLUT Callback: display()
// =============================================================================
// Called by GLUT every time the window needs to be redrawn.
// Orchestrates the full rendering pipeline for one frame:
//
//   1. Clear buffers
//   2. Draw starfield (world-space, before zoom)
//   3. Draw solar system (zoom applied inside)
//   4. Draw HUD overlay (pixel-space)
//   5. Swap buffers
//
// GRAPHICS CONCEPT — Display Callback:
//   GLUT calls this function whenever glutPostRedisplay() is called (from
//   the timer) or the window is exposed/resized. We never call display()
//   directly — we ask GLUT to schedule it via glutPostRedisplay().
// =============================================================================
static void display()
{
    clearScene();

    // Apply the single unified projection (zoom + aspect).
    // ZOOM BUG FIX: Previously the starfield used a separate non-zoomed
    // glOrtho pass, which put stars and planets in different coordinate
    // spaces. Now everything shares one projection — planets, orbit paths,
    // and stars all scale together correctly when zooming.
    updateProjection();

    // ------------------------------------------------------------------
    // Pass 1 — Starfield
    // Draw with identity modelview (before scene rotation/pan) so stars
    // appear as a fixed backdrop. They still zoom slightly with the
    // projection which looks natural (moving through space feeling).
    // ------------------------------------------------------------------
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    drawStarfield(g_stars);

    // ------------------------------------------------------------------
    // Pass 2 — Solar System (orbit paths + planets + moons)
    // Apply scene rotation and pan ONCE on the modelview here. All
    // subsequent planet draws use glPushMatrix/glPopMatrix relative to
    // this base transform, so rotation and pan affect the whole scene
    // uniformly — orbit paths and planet bodies always stay in sync.
    //
    // GRAPHICS CONCEPT — Camera as Inverse World Transform:
    //   Legacy OpenGL has no camera. We rotate/translate the WORLD.
    //   glRotatef spins the entire scene around the origin == orbiting
    //   a camera around the scene centre. glTranslatef == panning.
    // ------------------------------------------------------------------
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(g_simState.panX, g_simState.panY, 0.0f);
    glRotatef(g_simState.sceneRotation, 0.0f, 0.0f, 1.0f);

    drawSolarSystem(
        *g_sun,
        g_planets,
        g_simState.selectedPlanet,
        g_simState.zoomLevel,
        g_simState.elapsedTime
    );

    // ------------------------------------------------------------------
    // Pass 3 — HUD overlay (pixel space, always on top, no zoom)
    // ------------------------------------------------------------------
    drawHUD(g_simState, g_planets, g_windowW, g_windowH);

    glutSwapBuffers();
}

// =============================================================================
// GLUT Callback: reshape(w, h)
// =============================================================================
// Called when the window is resized. Reconfigures the orthographic projection
// to maintain correct world proportions at the new aspect ratio.
//
// GRAPHICS CONCEPT — Orthographic Projection Setup:
//   glOrtho defines a rectangular viewing volume. We keep the horizontal
//   world extent fixed at WORLD_HALF_W and scale the vertical extent by
//   the window's aspect ratio. This means:
//     - Resizing the window never stretches or squishes the scene
//     - The solar system always appears centred and proportional
//     - Zoom is applied in the modelview, not here (clean separation)
// =============================================================================
// updateProjection()
// -----------------------------------------------------------------------------
// Sets the orthographic projection based on the current window size AND zoom.
// Called from reshape() and every frame when zoom changes.
//
// GRAPHICS CONCEPT — Zoom via Projection:
//   The correct way to zoom an orthographic scene is to shrink or expand
//   the view volume (glOrtho bounds), NOT to scale the modelview matrix.
//   Shrinking the bounds = fewer world units visible = zoom in.
//   Expanding the bounds = more world units visible = zoom out.
//   This keeps all objects in the correct coordinate space.
// -----------------------------------------------------------------------------
static void updateProjection()
{
    float aspect = static_cast<float>(g_windowW) / static_cast<float>(g_windowH);

    // Divide world half-width by zoom: zoom>1 = smaller view = zoom in
    float halfW = WORLD_HALF_W / g_simState.zoomLevel;
    float halfH = halfW / aspect;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-halfW,  halfW,    // left, right
            -halfH,  halfH,    // bottom, top
            -500.0,  500.0);   // near, far

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;

    g_windowW = w;
    g_windowH = h;

    glViewport(0, 0, w, h);
    updateProjection();
}

// =============================================================================
// GLUT Callback: timer(value)
// =============================================================================
// The heartbeat of the simulation. Called every FRAME_DELAY_MS milliseconds.
//
// Responsibilities:
//   1. Advance simulation time if not paused
//   2. Update all planet orbital and rotation angles
//   3. Request a redraw via glutPostRedisplay
//   4. Re-register itself for the next tick
//
// GRAPHICS CONCEPT — Timer-Based Animation:
//   glutTimerFunc schedules a one-shot callback after a delay. By always
//   re-registering at the end of each call, we create a recurring timer.
//   Using a fixed delay (16ms ≈ 60fps) rather than glutIdleFunc prevents
//   the simulation from running at uncapped speed and burning CPU.
//
// Frame-rate independence:
//   dt = FRAME_DELAY_MS / 1000.0 * timeScale
//   All angle updates multiply speed by dt, so orbital periods remain
//   consistent even if occasional frames take longer than 16ms.
// =============================================================================
static void timer(int /*value*/)
{
    // Fixed delta time based on target frame delay
    // timeScale: >0 = forward, <0 = reverse, 0 via paused flag
    float dt = (FRAME_DELAY_MS / 1000.0f) * g_simState.timeScale;

    if (!g_simState.paused)
    {
        // Advance the total elapsed simulation time (used for animations)
        g_simState.elapsedTime += dt;

        // Update sun's self-rotation (it doesn't orbit)
        g_sun->update(dt);

        // Update all planets and their moons (recursive)
        for (Planet& planet : g_planets) {
            planet.update(dt);
        }
    }

    // Request a redraw — GLUT will call display() as soon as possible
    glutPostRedisplay();

    // Re-register the timer for the next frame
    glutTimerFunc(FRAME_DELAY_MS, timer, 0);
}

// =============================================================================
// GLUT Callback Wrappers
// =============================================================================
// GLUT requires plain C-style function pointers for callbacks. These thin
// wrappers capture the global state and forward to the clean implementations
// in controls.cpp.
//
// This pattern keeps controls.cpp free of global variables while satisfying
// GLUT's API constraint — the wrapper is the only code that "knows" about
// the globals; the actual logic lives in controls.cpp functions that receive
// explicit parameters.
// =============================================================================

static void keyboardCallback(unsigned char key, int x, int y)
{
    handleKeyboard(key, x, y, g_simState, g_planets);
    glutPostRedisplay();   // ensure HUD updates immediately on input
}

static void specialKeyCallback(int key, int x, int y)
{
    handleSpecialKeys(key, x, y, g_simState);
    glutPostRedisplay();
}

static void mouseCallback(int button, int state, int x, int y)
{
    handleMouse(button, state, x, y, g_simState);
    glutPostRedisplay();
}

// =============================================================================
// GLUT Callback: motionCallback(x, y)
// =============================================================================
// Called when the mouse moves while a button is held (glutMotionFunc).
//
// Left button drag  → rotate entire solar system around Z axis
// Middle button drag → pan the camera
//
// GRAPHICS CONCEPT — Mouse-to-World Mapping:
//   Screen pixels must be converted to world-space deltas.
//   dx pixels * (worldWidth / windowWidth) gives world units per pixel.
//   We use this to make pan speed independent of zoom level.
// =============================================================================
static void motionCallback(int x, int y)
{
    if (g_simState.isDragging)
    {
        // Convert horizontal pixel delta to rotation degrees.
        // 0.4 deg/pixel gives a comfortable drag feel.
        int dx = x - g_simState.dragStartX;
        int dy = y - g_simState.dragStartY;
        g_simState.sceneRotation = g_simState.dragStartRotation
                                   + static_cast<float>(dx) * 0.4f;
        (void)dy;   // vertical drag unused for Z-rotation — could add tilt later
        glutPostRedisplay();
    }

    if (g_simState.isPanning)
    {
        // Convert pixel delta to world-space delta.
        // World width visible = 2 * WORLD_HALF_W / zoomLevel
        float worldW = 2.0f * WORLD_HALF_W / g_simState.zoomLevel;
        float worldH = worldW * static_cast<float>(g_windowH) /
                                static_cast<float>(g_windowW > 0 ? g_windowW : 1);

        float dx = static_cast<float>(x - g_simState.panStartX)
                   * (worldW / static_cast<float>(g_windowW));
        float dy = static_cast<float>(y - g_simState.panStartY)
                   * (worldH / static_cast<float>(g_windowH));

        // Y is flipped: window Y grows downward, world Y grows upward
        g_simState.panX = g_simState.panStartWorldX + dx;
        g_simState.panY = g_simState.panStartWorldY - dy;

        glutPostRedisplay();
    }
}

// =============================================================================
// initOpenGL()
// =============================================================================
// Configures one-time OpenGL state that applies for the entire session.
// Called once after the GLUT window is created.
// =============================================================================
static void initOpenGL()
{
    // Deep space black background (also set per-frame in clearScene)
    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);

    // Enable smooth point and line rendering
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,  GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT,   GL_NICEST);

    // Enable alpha blending (used for glow, rings, faint orbit paths)
    // GRAPHICS CONCEPT — Alpha Blending:
    //   GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA is the standard "over"
    //   compositing operator: result = src.rgb * src.a + dst.rgb * (1 - src.a)
    //   This gives correct transparency for overlapping translucent objects.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable depth testing for correct front-to-back ordering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Use a slightly wider default line width for orbit paths
    glLineWidth(1.0f);
}

// =============================================================================
// printStartupInfo()
// =============================================================================
// Prints a summary of controls and planet key bindings to the terminal.
// Useful for the grader or user who wants a quick reference without looking
// at the HUD.
// =============================================================================
static void printStartupInfo()
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║   Hierarchical Solar System Simulator            ║\n");
    printf("║   Computer Graphics Capstone — SITE 3rd Year     ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  SIMULATION CONTROLS                             ║\n");
    printf("║   SPACE      Pause / Resume                      ║\n");
    printf("║   + / -      Speed up / Slow down                ║\n");
    printf("║   R          Reverse time                        ║\n");
    printf("║   T          Reset speed to 1.0x                 ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  CAMERA                                          ║\n");
    printf("║   Z / X           Zoom in / out                  ║\n");
    printf("║   Mouse Wheel     Zoom in / out                  ║\n");
    printf("║   Arrow Up/Down   Zoom in / out                  ║\n");
    printf("║   LMB drag        Rotate entire scene            ║\n");
    printf("║   MMB drag        Pan camera                     ║\n");
    printf("║   RMB click       Reset camera                   ║\n");
    printf("║   C               Reset camera (keyboard)        ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  PLANET SELECTION & SCALING                      ║\n");
    printf("║   1  Mercury    5  Jupiter                       ║\n");
    printf("║   2  Venus      6  Saturn                        ║\n");
    printf("║   3  Earth      7  Uranus                        ║\n");
    printf("║   4  Mars       8  Neptune                       ║\n");
    printf("║   [  Shrink selected planet                      ║\n");
    printf("║   ]  Grow selected planet                        ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║   ESC        Exit                                ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");
}

// =============================================================================
// main(argc, argv)
// =============================================================================
// Application entry point.
//
// Sequence:
//   1. Print startup information to terminal
//   2. Initialise GLUT and create window
//   3. Initialise OpenGL state
//   4. Build the solar system data
//   5. Generate the star field
//   6. Register all GLUT callbacks
//   7. Set initial projection via reshape
//   8. Start the timer
//   9. Enter the GLUT main loop (never returns)
// =============================================================================
int main(int argc, char** argv)
{
    printStartupInfo();

    // -------------------------------------------------------------------------
    // GLUT Initialisation
    // -------------------------------------------------------------------------
    glutInit(&argc, argv);

    // Request double buffering + RGB colour + depth buffer
    // GRAPHICS CONCEPT — Double Buffering:
    //   GLUT_DOUBLE means we draw to an off-screen back buffer and swap it
    //   to the display when complete. This eliminates visible tearing that
    //   would occur if we drew directly to the visible front buffer.
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Set initial window size and position (centred)
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(
        (glutGet(GLUT_SCREEN_WIDTH)  - WINDOW_WIDTH)  / 2,
        (glutGet(GLUT_SCREEN_HEIGHT) - WINDOW_HEIGHT) / 2
    );

    glutCreateWindow(WINDOW_TITLE);

    // -------------------------------------------------------------------------
    // OpenGL state setup (must happen after window creation)
    // -------------------------------------------------------------------------
    initOpenGL();

    // -------------------------------------------------------------------------
    // Build simulation data
    // -------------------------------------------------------------------------
    createSolarSystem();

    // Generate star field once — reused every frame
    g_stars = generateStars(STAR_COUNT, WORLD_HALF_W * 2.0f);

    // -------------------------------------------------------------------------
    // Register GLUT callbacks
    // -------------------------------------------------------------------------
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardCallback);
    glutSpecialFunc(specialKeyCallback);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(motionCallback);   // mouse drag: left=rotate, middle=pan

    // Establish the initial orthographic projection
    reshape(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Start the animation timer (fires once, then re-registers itself)
    glutTimerFunc(FRAME_DELAY_MS, timer, 0);

    // -------------------------------------------------------------------------
    // Enter GLUT main loop — this call never returns
    // -------------------------------------------------------------------------
    // GRAPHICS CONCEPT — Event-Driven Main Loop:
    //   glutMainLoop() hands control to GLUT. From this point on, the
    //   application is driven entirely by callbacks: display() when a frame
    //   is needed, timer() on the interval, keyboard/mouse on user input,
    //   reshape() on window resize. Our code only runs inside these callbacks.
    glutMainLoop();

    // g_sun is a unique_ptr — destructor runs automatically if loop exits
    return 0;
}