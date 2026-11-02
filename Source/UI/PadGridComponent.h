#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PadComponent.h"
#include "IconButton.h"
#include "MPC60Theme.h"
#include "../Domain/PadDefinition.h"

namespace drummatch
{

class PadGridComponent : public juce::Component,
                         public juce::DragAndDropTarget
{
public:
    PadGridComponent();
    ~PadGridComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setPadDefinitions(const std::vector<PadDefinition>& definitions);
    std::vector<PadDefinition> getPadDefinitions() const;

    void updatePadDefinition(const PadDefinition& def);
    void assignSampleToCategory(InstrumentCategory cat, const Sample& sample);
    void assignSampleToPad(int padIndex, const Sample& sample);
    void clearPadSample(int padIndex);
    void clearAllPads();
    void triggerPad(int padIndex);
    juce::String setReferenceSampleId(const juce::String& sampleId);

    int getSelectedPadIndex() const { return selectedPadIndex; }
    std::optional<PadDefinition> getPadDefinition(int padIndex) const;
    void selectPad(int padIndex);

    int findPadIndexForCategory(InstrumentCategory cat) const;

    // juce::DragAndDropTarget callbacks
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    std::function<void(const PadDefinition&)> onPadSelected;
    std::function<InstrumentCategory(const juce::String& sampleId)> getSampleCategoryCallback;
    std::function<void(const juce::String& sampleId, int padIndex)> onSampleDroppedOnPad;
    std::function<void()> onAutoMatchRequested;
    std::function<void()> onShuffleMatchRequested;

private:
    void highlightTargetPadForSample(const juce::String& sampleId);
    void clearDragHighlight();

    std::vector<std::unique_ptr<PadComponent>> padComponents;
    int selectedPadIndex = -1;
    int highlightedDragPadIndex = -1;

    IconButton varyButton { "VARY", IconType::None, theme::ButtonStyle::Cream };
    IconButton autoKitButton { "AUTO KIT", IconType::None, theme::ButtonStyle::Primary };
};

} // namespace drummatch
