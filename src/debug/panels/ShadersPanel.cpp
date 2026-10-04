// "Shaders" debug panel: the files of the shader program, a reload button and the last error.
// See docs/modules/gfx/shaders.md
#include "debug/panels/ShadersPanel.hpp"

#include "core/Paths.hpp"
#include "gfx/Shader.hpp"

#include <imgui.h>

#include <string>

namespace debug {

namespace {

// Text color of a failed load (red, green, blue, alpha): a light red that stands out from
// the white text of the rest of the panel.
constexpr ImVec4 ERROR_TEXT_COLOR{1.0F, 0.4F, 0.4F, 1.0F};

} // namespace

void drawShadersPanel(gfx::Shader& shader) {
    if (ImGui::Begin("Shaders")) {
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

        ImGui::Separator();
        // Button returns true only in the frame in which it was clicked. The result of
        // reload() is not needed here: the lines below read it from lastError().
        if (ImGui::Button("Reload shaders")) {
            shader.reload();
        }

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
    ImGui::End();
}

} // namespace debug
