// The rules of the HUD that are plain arithmetic: how large it is drawn and how long
// a sentence stays.
#include "debug/HudRules.hpp"

#include <algorithm>

namespace debug {

float hudScale(float displayScale, float windowHeight) {
    return std::max(displayScale, windowHeight / HUD_REFERENCE_HEIGHT);
}

float hintOpacity(float seconds, float holdSeconds, float fadeSeconds) {
    if (seconds < 0.0F) {
        return 0.0F;
    }
    if (seconds <= holdSeconds) {
        return 1.0F;
    }
    // Without a fade the sentence is gone the moment its time is over. This also keeps
    // the division below away from 0.
    if (fadeSeconds <= 0.0F) {
        return 0.0F;
    }
    // How much of the fade is over, from 0 to 1, and what is left of the sentence.
    const float faded = (seconds - holdSeconds) / fadeSeconds;
    return std::max(1.0F - faded, 0.0F);
}

} // namespace debug
