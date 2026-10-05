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

// One program: a line with its two files and how its last load went, and under it the
// error message of a failed load.
void drawShaderStatus(const gfx::Shader& shader) {
    // The line shows only the file names. The full paths appear as a tooltip when the
    // mouse rests on the line. ImGui expects UTF-8, which core::pathText returns.
    const std::string vertexFile = core::pathText(shader.vertexPath().filename());
    const std::string fragmentFile = core::pathText(shader.fragmentPath().filename());
    const std::string vertexFullPath = core::pathText(shader.vertexPath());
    const std::string fragmentFullPath = core::pathText(shader.fragmentPath());

    if (shader.lastError().empty()) {
        ImGui::Text("%s + %s: OK", vertexFile.c_str(), fragmentFile.c_str());
        ImGui::SetItemTooltip("%s\n%s", vertexFullPath.c_str(), fragmentFullPath.c_str());
        return;
    }

    // A failed load. Valid means that there is still a linked program to draw with: the
    // one from before the failed reload. Without one (the very first load failed)
    // nothing is drawn with this program. The red of the text is a colour of the theme
    // (Theme.hpp), shared with the Assets panel.
    ImGui::PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR);
    ImGui::TextWrapped("%s + %s: FAILED, %s", vertexFile.c_str(), fragmentFile.c_str(),
                       shader.isValid() ? "the previous program stays in use"
                                        : "there is no program to draw with");
    ImGui::SetItemTooltip("%s\n%s", vertexFullPath.c_str(), fragmentFullPath.c_str());
    // The message contains text written by the driver, so it goes in as an argument of
    // "%s" and never as the format string itself. For an error inside an included file
    // it names that file (gfx::nameSourceFiles).
    ImGui::TextWrapped("%s", shader.lastError().c_str());
    ImGui::PopStyleColor();
}

} // namespace

void drawShadersPanel(std::span<gfx::Shader* const> shaders) {
    // First run only: the bottom edge of the window, right of the Collision panel (the
    // constant is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(SHADERS_PLACEMENT);
    if (ImGui::Begin("Shaders")) {
        // Button returns true only in the frame in which it was clicked. One button
        // reloads every program: after editing a file there is no need to know which
        // program it belongs to, and a file that several programs include
        // (common/lighting.glsl) is read again by each of them. The results of reload()
        // are not needed here: the lines below read them from lastError(). A program
        // whose reload fails keeps working with its previous version, and the others are
        // reloaded all the same.
        if (ImGui::Button("Reload shaders")) {
            for (gfx::Shader* shader : shaders) {
                shader->reload();
            }
        }

        ImGui::Separator();
        for (const gfx::Shader* shader : shaders) {
            drawShaderStatus(*shader);
        }
    }
    ImGui::End();
}

} // namespace debug
