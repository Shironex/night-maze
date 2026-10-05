// "Shaders" debug panel: the files of every shader program, a reload button and the errors.
// See docs/modules/gfx/shader-hot-reload.md
#include "debug/panels/ShadersPanel.hpp"

#include "core/Paths.hpp"
#include "debug/PanelLayout.hpp"
#include "debug/Theme.hpp"
#include "gfx/Shader.hpp"

#include <imgui.h>

#include <string>

namespace debug {

namespace {

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
        // argument of "%s" and never as the format string itself. The red of the text
        // is a colour of the theme (Theme.hpp), shared with the Assets panel.
        ImGui::PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR);
        ImGui::TextWrapped("%s", shader.lastError().c_str());
        ImGui::PopStyleColor();
    }
}

} // namespace

void drawShadersPanel(std::span<gfx::Shader* const> shaders) {
    // First run only: the bottom edge of the window, right of the Collision panel (the
    // constant is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(SHADERS_PLACEMENT);
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
