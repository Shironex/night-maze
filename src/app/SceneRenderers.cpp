// The objects on the graphics card the game draws with.
#include "app/SceneRenderers.hpp"

namespace game {

SceneRenderers::SceneRenderers(MenuBackground background)
    : mazeRenderer(assets),
      gameplayRenderer(assets),
      villageRenderer(assets),
      interactableRenderer(assets),
      terrainRenderer(assets),
      landRenderer(assets),
      puddleRenderer(assets),
      menuBackground(background) {}

} // namespace game
