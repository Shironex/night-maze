// Minimap: the settings of the map in the middle of the screen, where it stands, and the
// flat shapes it is drawn from (floors, walls, gate, crystals, flasks, levers, notes,
// player).
// See docs/modules/renderer/minimap.md
#pragma once

#include "game/Maze.hpp"
#include "game/MazeWorld.hpp"
#include "game/Round.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library: what the
// map shows is decided here and covered by tests. game::MinimapRenderer only copies the
// result to the graphics card and draws it.
//
// The map is a schematic drawn from the data of the maze, not a picture of the 3D scene.
// North is up and the map never turns: the maze axis x (the columns, towards the east)
// runs to the right, and the maze axis z (the rows, towards the south) runs DOWN the
// picture. Row 0 of the maze is its north edge (-Z), so it is the top row of the map.

/// Limits of the settings below. The debug UI uses them for its sliders, and
/// minimapRect brings the numbers into them: a slider accepts typed numbers too.
constexpr float MIN_MINIMAP_SIZE = 0.3F;
constexpr float MAX_MINIMAP_SIZE = 0.95F;
constexpr float MIN_MINIMAP_OPACITY = 0.1F;
constexpr float MAX_MINIMAP_OPACITY = 1.0F;

/// What can be changed about the map while the game runs. The debug UI edits the
/// fields. Whether the map is on the screen is not a setting: it is shown while the
/// player holds the map key (game::showsMap).
struct MinimapSettings {
    /// A switch for debugging: true keeps the map on the screen without the key, for
    /// screenshots and for tuning. It counts like a held key, so the player stands still.
    bool pinned = false;

    /// A switch for debugging: true shows the whole maze, also the cells the player has
    /// not discovered. It changes only what is drawn: the discovery of the round goes on
    /// underneath and is back when the switch is cleared.
    bool revealAll = false;

    /// The side of the square map as a part of the HEIGHT of the framebuffer of the
    /// window: 0.7 of 720 pixels is 504 pixels. A part and not a number of pixels, so
    /// the map covers the same share of the picture in every window and on a Retina
    /// display, where the framebuffer has twice as many pixels.
    float size = 0.7F;

    /// How much the map hides of the scene behind it: 1 hides it completely, lower
    /// values let it show through. High enough to read the map, and not 1: the player
    /// still sees a little of the corridor and knows that the game goes on behind it.
    float opacity = 0.88F;
};

/// Where the map stands in the framebuffer of the window: a square, in framebuffer
/// pixels, as glViewport wants it. x and y are its BOTTOM left corner, counted from the
/// bottom left corner of the framebuffer (OpenGL counts y upwards).
struct MinimapRect {
    int x = 0;
    int y = 0;
    /// Side of the square. 0 means that there is no room for a map at all.
    int size = 0;
};

/// The square of the map for a framebuffer of the given size (in framebuffer pixels,
/// never the size of the window in screen coordinates): in the middle of the picture,
/// with a side of settings.size of the height, rounded to whole pixels. In a window
/// that is narrower than that, the map is as wide as the window. A framebuffer without
/// pixels gives a square of size 0.
MinimapRect minimapRect(int framebufferWidth, int framebufferHeight,
                        const MinimapSettings& settings);

/// One corner of a triangle of the map, exactly as it lies in the vertex buffer:
/// 5 floats, 20 bytes.
struct MinimapVertex {
    /// A place in the maze seen from above, in metres: x is the world x (towards the
    /// east) and y is the world z (towards the south). post/minimap.vert turns it into
    /// a place in the picture (minimapProjection).
    glm::vec2 position{0.0F};

    /// The colour of the shape, as an sRGB value: the numbers the screen gets. They are
    /// written into the picture as they are (post/minimap.frag), never lit and never
    /// converted.
    glm::vec3 color{0.0F};
};

/// Number of floats in each field, the "size" parameter of glVertexAttribPointer.
constexpr int MINIMAP_POSITION_COMPONENTS = 2;
constexpr int MINIMAP_COLOR_COMPONENTS = 3;

// The vertex buffer is described to OpenGL as "5 floats per vertex, one after another".
// The line turns that into a compile error where the compiler adds padding (the same
// check as for gfx::Vertex).
static_assert(sizeof(MinimapVertex) ==
                  (MINIMAP_POSITION_COMPONENTS + MINIMAP_COLOR_COMPONENTS) * sizeof(float),
              "MinimapVertex must be 5 tightly packed floats");

// The colours of the map (red, green, blue), all sRGB values chosen on a screen. The
// wall, the gate, the crystal and the player have the colours the plan of the Maze
// panel uses for them (debug/Theme.hpp).

/// What is not discovered, and the land around the maze: a very dark blue. The picture
/// is cleared to it before the shapes are drawn.
constexpr glm::vec3 MINIMAP_BACKGROUND_COLOR{0.03F, 0.04F, 0.07F};
/// The thin line around the whole map, drawn by the overlay pass
/// (post/minimap_overlay.frag): a muted blue grey, between the background and the
/// walls. It shows where the map ends while most of it is still dark, without looking
/// like a wall of the maze.
constexpr glm::vec3 MINIMAP_BORDER_COLOR{0.30F, 0.36F, 0.48F};
/// The width of that line as a part of the HEIGHT of the framebuffer of the window,
/// like the size and the margin of the map, and the least it is in pixels: 1 pixel at
/// 720 pixels of height, 2 at 1440.
constexpr float MINIMAP_BORDER_WIDTH = 0.0015F;
constexpr float MIN_MINIMAP_BORDER_PIXELS = 1.0F;
/// The floor of a cell that is shown.
constexpr glm::vec3 MINIMAP_FLOOR_COLOR{0.15F, 0.18F, 0.25F};
/// The floor of the start cell: a lighter blue.
constexpr glm::vec3 MINIMAP_START_COLOR{0.24F, 0.33F, 0.52F};
/// The floor of the exit cell: a dark green.
constexpr glm::vec3 MINIMAP_EXIT_COLOR{0.17F, 0.42F, 0.24F};
/// A wall: a light grey blue.
constexpr glm::vec3 MINIMAP_WALL_COLOR{0.69F, 0.75F, 0.85F};
/// The gate of the exit while it blocks the way: the orange of its wood.
constexpr glm::vec3 MINIMAP_GATE_COLOR{0.84F, 0.56F, 0.32F};
/// The gate once it has opened: a dim blue green, close to the floor.
constexpr glm::vec3 MINIMAP_GATE_OPEN_COLOR{0.17F, 0.41F, 0.44F};
/// A crystal that is still there: turquoise.
constexpr glm::vec3 MINIMAP_CRYSTAL_COLOR{0.34F, 0.84F, 0.79F};
/// A flask of tea that is still there: a light rose, the one colour of the map that is
/// neither a cold blue green nor one of the oranges of the gate, the levers and the
/// player. Its shape is a plus sign, which nothing else on the map has.
constexpr glm::vec3 MINIMAP_FLASK_COLOR{0.95F, 0.62F, 0.72F};
/// A lever that is not pulled yet: a strong orange red, the colour of a switch.
constexpr glm::vec3 MINIMAP_LEVER_COLOR{0.93F, 0.36F, 0.24F};
/// A lever that is pulled: the same colour, dim. Its wall is no longer on the map.
constexpr glm::vec3 MINIMAP_LEVER_PULLED_COLOR{0.42F, 0.24F, 0.22F};
/// A note: the pale yellow of paper.
constexpr glm::vec3 MINIMAP_NOTE_COLOR{0.90F, 0.86F, 0.70F};
/// The arrow of the player: a warm yellow.
constexpr glm::vec3 MINIMAP_PLAYER_COLOR{1.0F, 0.72F, 0.33F};

/// Half of the side of the square piece of the world the map shows, in metres. The
/// square is centred on the maze and a little larger than its longer side, so the walls
/// on the border, which reach half of their thickness out of the maze, are not cut off.
/// A maze that is not square is shown with empty strips beside its shorter side: the
/// cells stay square.
float minimapHalfExtent(const Maze& maze);

/// The matrix that takes a MinimapVertex position (metres) to clip space, where the
/// picture is the square from -1 to 1: an orthographic projection of the square of
/// minimapHalfExtent around the middle of the maze. It also turns the picture the right
/// way up: the north edge of the maze (z = 0) lands at the top (y = +1), although z
/// grows towards the south.
glm::mat4 minimapProjection(const Maze& maze);

/// How many metres of the maze one pixel of a map picture of pixels by pixels covers.
/// The builder below needs it to keep thin things at least a pixel wide. pixels below
/// 1 is taken as 1.
float minimapMetresPerPixel(const Maze& maze, int pixels);

/// Where the player is drawn on the map and which way the arrow points.
struct MinimapPlayer {
    /// The feet of the player in the world. Only x and z are used.
    glm::vec3 position{0.0F};
    /// The yaw of the camera in degrees: 0 looks north (up on the map), 90 east (right).
    float yawDegrees = 0.0F;
};

/// Builds everything the minimap shows as a list of triangles: every three vertices are
/// one triangle (GL_TRIANGLES), and later triangles are drawn over earlier ones. In
/// this order:
///   1. the floor of every shown cell: the start cell and the exit cell in colours of
///      their own,
///   2. the walls of the shown cells, as thin rectangles on the cell edges. A wall is
///      drawn when at least one of the two cells it stands between is shown. The walls
///      are asked from the maze of the round (game::roundMaze, Maze::hasWall) in every
///      call, so a wall that a lever has opened is not drawn any more,
///   3. the gate, when the exit cell is shown: across the open side of that cell, in
///      one colour while it blocks the way and in another once it has opened,
///   4. a small diamond for every crystal that is not collected and whose cell is shown,
///   5. a small plus sign for every flask that is not picked up and whose cell is
///      shown: the same rule as for the crystals,
///   6. a small square for every lever and every note whose cell is shown, at the wall
///      it hangs on: a lever in one colour until it is pulled and in a dim one after,
///   7. the player: a triangle that points where the camera looks. It is always drawn,
///      also outside the maze (where the picture may cut it off).
///
/// A cell is "shown" when it is discovered in the round (Round::discovery), or always
/// with revealAll set. metresPerPixel (minimapMetresPerPixel) keeps the walls, the
/// crystals and the player from getting thinner than a pixel or two in a large maze.
///
/// The round must have been started on this world. With another round nothing breaks:
/// cells its grid does not have count as not discovered.
std::vector<MinimapVertex> buildMinimapVertices(const MazeWorld& world, const Round& round,
                                                bool revealAll, const MinimapPlayer& player,
                                                float metresPerPixel);

} // namespace game
