// Campaign: the five nights of the story as one table of numbers, and the rules of the
// progress through them.
#include "game/Campaign.hpp"

#include "game/Intro.hpp"

#include <algorithm>
#include <stdexcept>

namespace game {

namespace {

// One row per night, in their order: night 1 is row 0.
constexpr std::array<CampaignNight, CAMPAIGN_NIGHT_COUNT> NIGHTS = {{
    {.title = "First Frost",
     .mazeWidth = 10,
     .mazeHeight = 10,
     .crystalCount = 13,
     .flaskCount = 1,
     .requiredFraction = 0.7F,
     .batteryLifetimeSeconds = 180.0F,
     .shade = false,
     .storyNotes = 5,
     .hintNotes = 2,
     .firstStoryLine = 0,
     .storyLineCount = 5,
     .endLine = "The near lamps are lit. The village slept a little."},
    {.title = "The Shepherds' Gates",
     .mazeWidth = 13,
     .mazeHeight = 13,
     .crystalCount = 19,
     .flaskCount = 1,
     .requiredFraction = 0.7F,
     .batteryLifetimeSeconds = 165.0F,
     .shade = true,
     .storyNotes = 5,
     .hintNotes = 3,
     .firstStoryLine = 5,
     .storyLineCount = 5,
     .endLine = "The lane is lit as far as the well."},
    {.title = "Lamp's Back",
     .mazeWidth = 16,
     .mazeHeight = 16,
     .crystalCount = 26,
     .flaskCount = 2,
     .requiredFraction = 0.7F,
     .batteryLifetimeSeconds = 150.0F,
     .shade = true,
     .storyNotes = 5,
     .hintNotes = 4,
     .firstStoryLine = 10,
     .storyLineCount = 5,
     .endLine = "It followed you to the gate and stopped at the lamplight."},
    {.title = "What the Moon Misses",
     .mazeWidth = 19,
     .mazeHeight = 19,
     .crystalCount = 33,
     .flaskCount = 2,
     .requiredFraction = 0.8F,
     .batteryLifetimeSeconds = 135.0F,
     .shade = true,
     .storyNotes = 5,
     .hintNotes = 5,
     .firstStoryLine = 15,
     .storyLineCount = 5,
     .endLine = "Lamps to the edge of the village. One window still dark."},
    {.title = "The Last Lamp",
     .mazeWidth = 22,
     .mazeHeight = 22,
     .crystalCount = 40,
     .flaskCount = 3,
     .requiredFraction = 0.8F,
     .batteryLifetimeSeconds = 120.0F,
     .shade = true,
     .storyNotes = 4,
     .hintNotes = 6,
     .firstStoryLine = 20,
     .storyLineCount = 4,
     .endLine = ""},
}};

// The four lines of the ending card. The third has two forms (endingLines).
constexpr const char* ENDING_FIRST_LINE = "Every lamp in the village has its splinter now.";
constexpr const char* ENDING_SECOND_LINE = "The last one hangs in a window you know.";
constexpr const char* ENDING_LEFT_SOME_LINE =
    "You left a few behind. The moon will come back for them.";
constexpr const char* ENDING_LEFT_NONE_LINE = "You left none behind. The moon will look harder.";
constexpr const char* ENDING_LAST_LINE = "So will you.";

// The numbers that stir the bits of a seed (campaignNightSeed): the ones of a well
// known mixing function for 32 bit numbers (the last step of MurmurHash3). Any odd
// numbers with many bits set would do. They only have to stay the same for ever,
// because the mazes of a saved campaign follow from them.
constexpr std::uint32_t NIGHT_STEP = 0x9E3779B9U;
constexpr std::uint32_t FIRST_MULTIPLIER = 0x85EBCA6BU;
constexpr std::uint32_t SECOND_MULTIPLIER = 0xC2B2AE35U;
constexpr int FIRST_SHIFT = 16;
constexpr int SECOND_SHIFT = 13;

} // namespace

const CampaignNight& campaignNight(int night) {
    if (night < 1 || night > CAMPAIGN_NIGHT_COUNT) {
        throw std::out_of_range("campaignNight: the campaign has no night with this number");
    }
    return NIGHTS.at(static_cast<std::size_t>(night - 1));
}

std::string campaignNightLabel(int night) {
    return "Night " + std::to_string(night);
}

std::string campaignNightName(int night) {
    return campaignNightLabel(night) + ": " + campaignNight(night).title;
}

std::uint32_t campaignNightSeed(std::uint32_t campaignSeed, int night) {
    // Every night gets a number of its own, far from the one of its neighbour, and the
    // bits are stirred so that two campaign seeds next to each other do not give mazes
    // that are related. Multiplying unsigned numbers wraps around at 2^32, which is
    // well defined.
    std::uint32_t bits = campaignSeed + static_cast<std::uint32_t>(night) * NIGHT_STEP;
    bits ^= bits >> FIRST_SHIFT;
    bits *= FIRST_MULTIPLIER;
    bits ^= bits >> SECOND_SHIFT;
    bits *= SECOND_MULTIPLIER;
    bits ^= bits >> FIRST_SHIFT;
    return drawnCampaignSeed(bits);
}

std::uint32_t drawnCampaignSeed(std::uint32_t randomNumber) {
    return 1 + randomNumber % (CAMPAIGN_SEED_LIMIT - 1);
}

InteractableSettings campaignInteractables(int night, const InteractableSettings& settings) {
    const CampaignNight& level = campaignNight(night);
    InteractableSettings result = settings;
    result.noteCount = level.storyNotes + level.hintNotes;
    result.storyNoteCount = level.storyNotes;
    result.firstStoryLine = level.firstStoryLine;
    // The lines of a night are chosen for it: the first night, the one without a shade,
    // has no line about the shadow, so nothing has to be skipped.
    result.shadeLines = true;
    return result;
}

CampaignStage campaignStage(int campaignNight) {
    if (campaignNight <= 1) {
        return CampaignStage::NotStarted;
    }
    return campaignNight > CAMPAIGN_NIGHT_COUNT ? CampaignStage::Finished : CampaignStage::Running;
}

NightStatus nightStatus(int campaignNight, int night) {
    if (night < campaignNight) {
        return NightStatus::Finished;
    }
    return night == campaignNight ? NightStatus::Open : NightStatus::Locked;
}

int nightToOffer(int campaignNight) {
    return campaignStage(campaignNight) == CampaignStage::Finished
               ? 1
               : std::clamp(campaignNight, 1, CAMPAIGN_NIGHT_COUNT);
}

int nightAfterWin(int campaignNight, int wonNight) {
    return wonNight == campaignNight ? campaignNight + 1 : campaignNight;
}

NightEndOffer nightEndOffer(int campaignNight, int wonNight, bool counts) {
    const bool hasNext = wonNight >= 1 && wonNight < CAMPAIGN_NIGHT_COUNT;
    return counts && hasNext && wonNight + 1 == campaignNight ? NightEndOffer::NextNight
                                                              : NightEndOffer::BackToNights;
}

int bestAfterWin(int bestSeconds, float elapsedSeconds) {
    // The float is brought into the limits first: a huge one would not fit a whole number.
    const int seconds =
        static_cast<int>(std::clamp(elapsedSeconds, 1.0F, static_cast<float>(MAX_BEST_SECONDS)));
    return bestSeconds == NO_BEST_TIME ? seconds : std::min(bestSeconds, seconds);
}

std::array<const char*, STORY_CARD_LINE_COUNT> endingLines(bool crystalsLeft) {
    return {ENDING_FIRST_LINE, ENDING_SECOND_LINE,
            crystalsLeft ? ENDING_LEFT_SOME_LINE : ENDING_LEFT_NONE_LINE, ENDING_LAST_LINE};
}

StoryCardFrame nightCardFrame(float seconds) {
    StoryCardFrame frame;
    frame.finished = seconds >= NIGHT_CARD_SECONDS;
    frame.skippable = seconds >= INTRO_SKIP_DELAY_SECONDS;
    // Two lines, the label and the title: both come and go together.
    const float opacity = cardTextOpacity(seconds, NIGHT_CARD_SECONDS);
    frame.lineOpacity.at(0) = opacity;
    frame.lineOpacity.at(1) = opacity;
    return frame;
}

StoryCardFrame endingCardFrame(float seconds) {
    StoryCardFrame frame;
    frame.finished = seconds >= ENDING_CARD_SECONDS;
    frame.skippable = seconds >= INTRO_SKIP_DELAY_SECONDS;
    frame.hintOpacity = skipHintOpacity(std::max(seconds, 0.0F));
    // Each line is a card that begins a little later than the one above it and ends
    // with all the others: seen from its own beginning, the card is that much shorter.
    for (std::size_t line = 0; line < STORY_CARD_LINE_COUNT; ++line) {
        const float begins = static_cast<float>(line) * ENDING_LINE_SECONDS;
        frame.lineOpacity.at(line) =
            cardTextOpacity(seconds - begins, ENDING_CARD_SECONDS - begins);
    }
    return frame;
}

} // namespace game
