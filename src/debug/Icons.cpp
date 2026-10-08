// Icons of the debug window: a handful of small line drawings made with the draw list.
#include "debug/Icons.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace debug {

namespace {

// Every icon is designed on a grid of 24 x 24 units, like the icons of most icon sets.
// The points below are written in these units and scaled to the wanted size.
constexpr float GRID_SIZE = 24.0F;

// Thickness of the lines in grid units, and the thinnest line in pixels: below one
// pixel a line turns grey and blurry.
constexpr float LINE_THICKNESS = 1.7F;
constexpr float MIN_LINE_PIXELS = 1.0F;

// Radius of the rounded corners of the screen of the Render icon, in grid units.
constexpr float SCREEN_ROUNDING = 2.0F;

// The lines inside the logo are thinner than its outline by this factor.
constexpr float LOGO_INNER_LINE = 0.75F;

// The pen an icon is drawn with: it turns grid units into pixels on the screen and
// keeps the colour and the thickness, so the code of an icon is only its shapes.
class Pen {
public:
    Pen(ImDrawList* drawList, const ImVec2& topLeft, float size, ImU32 color)
        : m_drawList(drawList),
          m_origin(topLeft),
          m_unit(size / GRID_SIZE),
          m_color(color),
          m_thickness(std::max(LINE_THICKNESS * m_unit, MIN_LINE_PIXELS)) {}

    // The lines that follow are drawn this many times as thick as usual.
    void setThicknessFactor(float factor) {
        m_thickness = std::max(LINE_THICKNESS * m_unit * factor, MIN_LINE_PIXELS);
    }

    // A straight line between two points of the grid.
    void line(const ImVec2& from, const ImVec2& to) const {
        m_drawList->AddLine(toScreen(from), toScreen(to), m_color, m_thickness);
    }

    // A line through all the points, one after the other. closed joins the last point
    // to the first one.
    void polyline(std::span<const ImVec2> points, bool closed) const {
        // The path of a draw list is a list of points that is collected first and
        // drawn as one line by PathStroke, with clean joints at the corners.
        for (const ImVec2& point : points) {
            m_drawList->PathLineTo(toScreen(point));
        }
        m_drawList->PathStroke(m_color, closed ? ImDrawFlags_Closed : ImDrawFlags_None,
                               m_thickness);
    }

    // The outline of a circle.
    void circle(const ImVec2& center, float radius) const {
        // The fourth argument is the number of straight pieces: 0 lets ImGui choose.
        m_drawList->AddCircle(toScreen(center), radius * m_unit, m_color, 0, m_thickness);
    }

    // The outline of a rectangle with rounded corners.
    void roundedRect(const ImVec2& min, const ImVec2& max, float rounding) const {
        m_drawList->AddRect(toScreen(min), toScreen(max), m_color, rounding * m_unit,
                            ImDrawFlags_None, m_thickness);
    }

    // A curve from one point to another. The two control points say in which direction
    // the curve leaves its start and arrives at its end (a cubic Bezier curve).
    void curve(const ImVec2& from, const ImVec2& control1, const ImVec2& control2,
               const ImVec2& to) const {
        m_drawList->AddBezierCubic(toScreen(from), toScreen(control1), toScreen(control2),
                                   toScreen(to), m_color, m_thickness);
    }

private:
    // A point of the grid as a point on the screen.
    ImVec2 toScreen(const ImVec2& point) const {
        return {m_origin.x + point.x * m_unit, m_origin.y + point.y * m_unit};
    }

    ImDrawList* m_drawList;
    ImVec2 m_origin;
    // Pixels per grid unit.
    float m_unit;
    ImU32 m_color;
    float m_thickness;
};

// A screen on its stand.
void drawRender(const Pen& pen) {
    pen.roundedRect({3.0F, 4.0F}, {21.0F, 16.0F}, SCREEN_ROUNDING);
    pen.line({12.0F, 16.0F}, {12.0F, 20.0F});
    pen.line({8.0F, 20.0F}, {16.0F, 20.0F});
}

// A flashlight that stands on its end: the wide head at the top, the handle below.
void drawLight(const Pen& pen) {
    constexpr std::array<ImVec2, 4> HEAD = {ImVec2{9.0F, 3.0F}, ImVec2{15.0F, 3.0F},
                                            ImVec2{16.0F, 8.0F}, ImVec2{8.0F, 8.0F}};
    constexpr std::array<ImVec2, 6> HANDLE = {ImVec2{8.0F, 8.0F},   ImVec2{9.0F, 11.0F},
                                              ImVec2{9.0F, 21.0F},  ImVec2{15.0F, 21.0F},
                                              ImVec2{15.0F, 11.0F}, ImVec2{16.0F, 8.0F}};
    pen.polyline(HEAD, true);
    pen.polyline(HANDLE, false);
}

// Two sliders, each a line with a knob on it.
void drawPostProcess(const Pen& pen) {
    constexpr float KNOB_RADIUS = 2.0F;
    pen.line({4.0F, 7.0F}, {14.0F, 7.0F});
    pen.circle({16.0F, 7.0F}, KNOB_RADIUS);
    pen.line({18.0F, 7.0F}, {20.0F, 7.0F});
    pen.line({4.0F, 17.0F}, {6.0F, 17.0F});
    pen.circle({8.0F, 17.0F}, KNOB_RADIUS);
    pen.line({10.0F, 17.0F}, {20.0F, 17.0F});
}

// A small maze: a square with a few walls in it.
void drawWorld(const Pen& pen) {
    constexpr std::array<ImVec2, 3> CORNER_WALL = {ImVec2{4.0F, 10.0F}, ImVec2{12.0F, 10.0F},
                                                   ImVec2{12.0F, 14.0F}};
    pen.roundedRect({4.0F, 4.0F}, {20.0F, 20.0F}, 1.0F);
    pen.polyline(CORNER_WALL, false);
    pen.line({12.0F, 4.0F}, {12.0F, 10.0F});
    pen.line({16.0F, 10.0F}, {16.0F, 20.0F});
    pen.line({8.0F, 14.0F}, {8.0F, 20.0F});
}

// A head and the line of the shoulders under it.
void drawPlayer(const Pen& pen) {
    pen.circle({12.0F, 8.0F}, 3.5F);
    pen.curve({5.0F, 20.0F}, {5.0F, 16.0F}, {8.0F, 14.0F}, {12.0F, 14.0F});
    pen.curve({12.0F, 14.0F}, {16.0F, 14.0F}, {19.0F, 16.0F}, {19.0F, 20.0F});
}

// A crystal: a kite with its two inner edges.
void drawCrystal(const Pen& pen) {
    constexpr std::array<ImVec2, 4> OUTLINE = {ImVec2{12.0F, 3.0F}, ImVec2{18.0F, 9.0F},
                                               ImVec2{12.0F, 21.0F}, ImVec2{6.0F, 9.0F}};
    pen.polyline(OUTLINE, true);
    pen.line({6.0F, 9.0F}, {18.0F, 9.0F});
    pen.line({12.0F, 3.0F}, {12.0F, 21.0F});
}

// The line of a heartbeat on a monitor.
void drawDiagnostics(const Pen& pen) {
    constexpr std::array<ImVec2, 6> BEAT = {ImVec2{3.0F, 12.0F},  ImVec2{7.0F, 12.0F},
                                            ImVec2{9.0F, 6.0F},   ImVec2{13.0F, 18.0F},
                                            ImVec2{15.0F, 12.0F}, ImVec2{21.0F, 12.0F}};
    pen.polyline(BEAT, false);
}

// A magnifying glass.
void drawSearch(const Pen& pen) {
    pen.circle({11.0F, 11.0F}, 6.0F);
    pen.line({15.5F, 15.5F}, {20.0F, 20.0F});
}

// A pin: its head and its needle.
void drawPin(const Pen& pen) {
    constexpr std::array<ImVec2, 6> HEAD = {ImVec2{9.0F, 3.0F},  ImVec2{15.0F, 3.0F},
                                            ImVec2{14.0F, 9.0F}, ImVec2{17.0F, 12.0F},
                                            ImVec2{7.0F, 12.0F}, ImVec2{10.0F, 9.0F}};
    pen.polyline(HEAD, true);
    pen.line({12.0F, 12.0F}, {12.0F, 20.0F});
}

// Four corners that point outwards: "make it large again".
void drawExpand(const Pen& pen) {
    constexpr std::array<ImVec2, 3> TOP_LEFT_CORNER = {ImVec2{4.0F, 9.0F}, ImVec2{4.0F, 4.0F},
                                                       ImVec2{9.0F, 4.0F}};
    constexpr std::array<ImVec2, 3> TOP_RIGHT_CORNER = {ImVec2{20.0F, 9.0F}, ImVec2{20.0F, 4.0F},
                                                        ImVec2{15.0F, 4.0F}};
    constexpr std::array<ImVec2, 3> BOTTOM_LEFT_CORNER = {ImVec2{4.0F, 15.0F}, ImVec2{4.0F, 20.0F},
                                                          ImVec2{9.0F, 20.0F}};
    constexpr std::array<ImVec2, 3> BOTTOM_RIGHT_CORNER = {
        ImVec2{20.0F, 15.0F}, ImVec2{20.0F, 20.0F}, ImVec2{15.0F, 20.0F}};
    pen.polyline(TOP_LEFT_CORNER, false);
    pen.polyline(TOP_RIGHT_CORNER, false);
    pen.polyline(BOTTOM_LEFT_CORNER, false);
    pen.polyline(BOTTOM_RIGHT_CORNER, false);
}

// An arrow head to the left and one to the right.
void drawPrevious(const Pen& pen) {
    constexpr std::array<ImVec2, 3> HEAD = {ImVec2{15.0F, 6.0F}, ImVec2{9.0F, 12.0F},
                                            ImVec2{15.0F, 18.0F}};
    pen.polyline(HEAD, false);
}

void drawNext(const Pen& pen) {
    constexpr std::array<ImVec2, 3> HEAD = {ImVec2{9.0F, 6.0F}, ImVec2{15.0F, 12.0F},
                                            ImVec2{9.0F, 18.0F}};
    pen.polyline(HEAD, false);
}

} // namespace

void drawIcon(ImDrawList* drawList, Icon icon, const ImVec2& topLeft, float size, ImU32 color) {
    const Pen pen(drawList, topLeft, size, color);
    switch (icon) {
    case Icon::Render:
        drawRender(pen);
        break;
    case Icon::Light:
        drawLight(pen);
        break;
    case Icon::PostProcess:
        drawPostProcess(pen);
        break;
    case Icon::World:
        drawWorld(pen);
        break;
    case Icon::Player:
        drawPlayer(pen);
        break;
    case Icon::Gameplay:
        drawCrystal(pen);
        break;
    case Icon::Diagnostics:
        drawDiagnostics(pen);
        break;
    case Icon::Search:
        drawSearch(pen);
        break;
    case Icon::Pin:
        drawPin(pen);
        break;
    case Icon::Expand:
        drawExpand(pen);
        break;
    case Icon::Previous:
        drawPrevious(pen);
        break;
    case Icon::Next:
        drawNext(pen);
        break;
    }
}

void drawLogo(ImDrawList* drawList, const ImVec2& topLeft, float size, ImU32 color) {
    // The crystal of the Gameplay icon, a little taller, with thinner inner edges.
    constexpr std::array<ImVec2, 4> OUTLINE = {ImVec2{12.0F, 2.0F}, ImVec2{20.0F, 9.5F},
                                               ImVec2{12.0F, 22.0F}, ImVec2{4.0F, 9.5F}};
    Pen pen(drawList, topLeft, size, color);
    pen.polyline(OUTLINE, true);
    pen.setThicknessFactor(LOGO_INNER_LINE);
    pen.line({4.0F, 9.5F}, {20.0F, 9.5F});
    pen.line({12.0F, 2.0F}, {12.0F, 22.0F});
}

} // namespace debug
