// "Shaders" debug panel: the files of every shader program, a reload button and the errors.
// See docs/modules/gfx/shader-hot-reload.md
#include "debug/panels/ShadersPanel.hpp"

#include "core/Paths.hpp"
#include "gfx/Shader.hpp"

#include <imgui.h>

#include <string>

namespace debug {

namespace {

// Where the panel appears and how big it is the first time the program runs (later ImGui
// remembers it in imgui.ini): the bottom of the left edge of a 1280 x 720 window.
constexpr ImVec2 FIRST_POSITION{10.0F, 520.0F};
constexpr ImVec2 FIRST_SIZE{300.0F, 190.0F};

// Text color of a failed load (red, green, blue, alpha): a light red that stands out from
// the white text of the rest of the panel.
constexpr ImVec4 ERROR_TEXT_COLOR{1.0F, 0.4F, 0.4F, 1.0F};

// The lines of one program: its two files, whether it can be drawn with and how its last
// load went.
void drawShaderStatus(const gfx::Shader& shader) {
    // The label shows only the file name. The full path appears as a tooltip when the
    // mouse rests on the line. ImGui expects UTF-8, which core::pathText returns.
    const std::string vertexFile = core::pathText(shader.vertexPath().filename());
    const std::string vertexFullPath = core::pathText(shader.vertexPath());
    ImGui::Text("Vertex: %s", vertexFile.c_str());
    ImGui::SetItemTooltip("%s", vertexFullPath.c_str());

    const std::string fragmentFile = core::pathText(shader.fragmentPath().filename());
    const std::string fragmentFullPath = core::pathText(shader.fragmentPath());
    ImGui::Text("Fragment: %s", fragmentFile.c_str());
    ImGui::SetItemTooltip("%s", fragmentFullPath.c_str());

    // Valid means that there is a linked program to draw with. After a failed reload
    // it is still the previous program.
    ImGui::Text("Program: %s", shader.isValid() ? "valid" : "not valid");

    if (shader.lastError().empty()) {
        ImGui::TextUnformatted("Last load: OK");
    } else {
        ImGui::TextUnformatted("Last load: failed");
        // The message contains text written by the driver, so it goes in as an
        // argument of "%s" and never as the format string itself.
        ImGui::PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR);
        ImGui::TextWrapped("%s", shader.lastError().c_str());
        ImGui::PopStyleColor();
    }
}

} // namespace

void drawShadersPanel(std::span<gfx::Shader* const> shaders) {
    ImGui::SetNextWindowPos(FIRST_POSITION, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(FIRST_SIZE, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Shaders")) {
        // Button returns true only in the frame in which it was clicked. One button
        // reloads every program: after editing a file there is no need to know which
        // program it belongs to. The results of reload() are not needed here: the lines
        // below read them from lastError(). A program whose reload fails keeps working
        // with its previous version, and the others are reloaded all the same.
        if (ImGui::Button("Reload shaders")) {
            for (gfx::Shader* shader : shaders) {
                shader->reload();
            }
        }

        for (const gfx::Shader* shader : shaders) {
            ImGui::Separator();
            drawShaderStatus(*shader);
        }
    }
    ImGui::End();
}

} // namespace debug
