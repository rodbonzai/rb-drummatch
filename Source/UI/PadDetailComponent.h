#pragma once

#include <optional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Domain/PadDefinition.h"
#include "../Domain/InstrumentCategory.h"
#include "IconButton.h"
#include "MPC60Theme.h"

namespace drummatch
{

class PadDetailComponent : public juce::Component
{
public:
    PadDetailComponent();
    ~PadDetailComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSelectedPad(const PadDefinition& pad);
    void clearSelection();

    std::function<void(const PadDefinition&)> onPadUpdated;
    std::function<void(int padIndex)> onPlaySampleRequested;
    std::function<void(int padIndex)> onClearSampleRequested;
    std::function<void()> onResetLayoutRequested;
    std::function<void(int padIndex)> onNextCandidateRequested;
    std::function<void(int padIndex)> onPrevCandidateRequested;

private:
    std::optional<PadDefinition> currentPad;

    juce::String padIdText { "PAD ---" };
    juce::String infoText { "NO PAD SELECTED" };
    juce::String candCountText { "- / -" };

    juce::Rectangle<int> padIdArea;
    juce::Rectangle<int> infoArea;
    juce::Rectangle<int> candCountArea;

    IconButton prevCandButton  { "PREV", IconType::None, theme::ButtonStyle::Cream };
    IconButton nextCandButton  { "NEXT", IconType::None, theme::ButtonStyle::Cream };

    IconButton playSampleButton  { "PLAY", IconType::Play, theme::ButtonStyle::Cream };
    IconButton clearSampleButton { "CLEAR PAD", IconType::None, theme::ButtonStyle::Danger };
    IconButton resetButton       { "RESET MPC", IconType::None, theme::ButtonStyle::Cream };
};

} // namespace drummatch
