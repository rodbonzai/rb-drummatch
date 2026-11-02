#include "PadGridComponent.h"

namespace drummatch
{

PadGridComponent::PadGridComponent()
{
    const auto defaultDefs = PadDefinition::getDefaultLayout();
    setPadDefinitions(defaultDefs);

    varyButton.onClick = [this]()
    {
        if (onShuffleMatchRequested)
            onShuffleMatchRequested();
    };
    addAndMakeVisible(varyButton);

    autoKitButton.onClick = [this]()
    {
        if (onAutoMatchRequested)
            onAutoMatchRequested();
    };
    addAndMakeVisible(autoKitButton);
}

void PadGridComponent::setPadDefinitions(const std::vector<PadDefinition>& definitions)
{
    padComponents.clear();
    padComponents.reserve(definitions.size());

    for (const auto& def : definitions)
    {
        auto pad = std::make_unique<PadComponent>(def);
        pad->onClicked = [this](const PadDefinition& pDef)
        {
            selectedPadIndex = pDef.index;
            for (auto& p : padComponents)
                p->setSelected(p->getPadDefinition().index == selectedPadIndex);

            if (onPadSelected)
                onPadSelected(pDef);
        };

        addAndMakeVisible(*pad);
        padComponents.push_back(std::move(pad));
    }

    resized();
}

std::vector<PadDefinition> PadGridComponent::getPadDefinitions() const
{
    std::vector<PadDefinition> result;
    result.reserve(padComponents.size());
    for (const auto& p : padComponents)
        result.push_back(p->getPadDefinition());
    return result;
}

void PadGridComponent::updatePadDefinition(const PadDefinition& def)
{
    for (auto& pad : padComponents)
    {
        if (pad->getPadDefinition().index == def.index)
        {
            pad->setPadDefinition(def);
            break;
        }
    }
}

int PadGridComponent::findPadIndexForCategory(InstrumentCategory cat) const
{
    // 1. Coincidencia exacta con la categoría del pad
    for (const auto& pad : padComponents)
    {
        if (pad->getPadDefinition().category == cat)
            return pad->getPadDefinition().index;
    }

    // 2. Mapeos inteligentes a pads relacionados
    if (cat == InstrumentCategory::GhostKick)
        return findPadIndexForCategory(InstrumentCategory::Kick);

    if (cat == InstrumentCategory::GhostSnare)
        return findPadIndexForCategory(InstrumentCategory::Snare);

    if (cat == InstrumentCategory::Clap)
        return findPadIndexForCategory(InstrumentCategory::Snare);

    if (cat == InstrumentCategory::CrossStick || cat == InstrumentCategory::Rim)
    {
        for (const auto& pad : padComponents)
        {
            const auto pCat = pad->getPadDefinition().category;
            if (pCat == InstrumentCategory::Rim || pCat == InstrumentCategory::CrossStick)
                return pad->getPadDefinition().index;
        }
        return findPadIndexForCategory(InstrumentCategory::Snare);
    }

    if (cat == InstrumentCategory::ClosedHatHard)
        return findPadIndexForCategory(InstrumentCategory::ClosedHat);

    if (cat == InstrumentCategory::OpenHat)
    {
        for (const auto& pad : padComponents)
        {
            if (pad->getPadDefinition().category == InstrumentCategory::OpenHat)
                return pad->getPadDefinition().index;
        }
        return findPadIndexForCategory(InstrumentCategory::ClosedHat);
    }

    if (cat == InstrumentCategory::CrashCymbal || cat == InstrumentCategory::RideCymbal)
    {
        for (const auto& pad : padComponents)
        {
            const auto pCat = pad->getPadDefinition().category;
            if (pCat == InstrumentCategory::CrashCymbal || pCat == InstrumentCategory::RideCymbal)
                return pad->getPadDefinition().index;
        }
        return findPadIndexForCategory(InstrumentCategory::OpenHat);
    }

    if (cat == InstrumentCategory::Percussion01 || cat == InstrumentCategory::Percussion02 || cat == InstrumentCategory::Other)
    {
        // Enviar a pads de percusión A11 o A12 (nunca a Kick)
        for (const auto& pad : padComponents)
        {
            const auto pCat = pad->getPadDefinition().category;
            if (pCat == InstrumentCategory::Percussion01 || pCat == InstrumentCategory::Percussion02)
                return pad->getPadDefinition().index;
        }
    }

    // Si aún no se encuentra, buscar cualquier pad que no sea Kick
    for (const auto& pad : padComponents)
    {
        if (pad->getPadDefinition().category != InstrumentCategory::Kick)
            return pad->getPadDefinition().index;
    }

    return 0;
}

void PadGridComponent::assignSampleToCategory(InstrumentCategory cat, const Sample& sample)
{
    const int targetIdx = findPadIndexForCategory(cat);
    assignSampleToPad(targetIdx, sample);
}

void PadGridComponent::assignSampleToPad(int padIndex, const Sample& sample)
{
    for (auto& pad : padComponents)
    {
        if (pad->getPadDefinition().index == padIndex)
        {
            auto def = pad->getPadDefinition();
            def.assignedSample = sample;
            pad->setPadDefinition(def);
            break;
        }
    }
}

void PadGridComponent::clearPadSample(int padIndex)
{
    for (auto& pad : padComponents)
    {
        if (pad->getPadDefinition().index == padIndex)
        {
            auto def = pad->getPadDefinition();
            def.assignedSample.reset();
            def.matchScore = 0.0f;
            def.matchReason = "";
            def.alternativeIndex = 0;
            def.totalAlternatives = 0;
            def.isReference = false;
            pad->setPadDefinition(def);
            break;
        }
    }
}

void PadGridComponent::clearAllPads()
{
    selectedPadIndex = -1;
    for (auto& pad : padComponents)
    {
        auto def = pad->getPadDefinition();
        def.assignedSample.reset();
        def.matchScore = 0.0f;
        def.matchReason = "";
        def.alternativeIndex = 0;
        def.totalAlternatives = 0;
        def.isReference = false;
        pad->setPadDefinition(def);
        pad->setSelected(false);
    }
}

void PadGridComponent::triggerPad(int padIndex)
{
    selectPad(padIndex);
    for (auto& pad : padComponents)
    {
        if (pad->getPadDefinition().index == padIndex)
        {
            pad->triggerPressAnimation();
            break;
        }
    }
}

juce::String PadGridComponent::setReferenceSampleId(const juce::String& sampleId)
{
    juce::String matchedPadId = "";
    for (auto& pad : padComponents)
    {
        auto def = pad->getPadDefinition();
        const bool isRef = (sampleId.isNotEmpty() && def.assignedSample.has_value() && def.assignedSample->id == sampleId);
        if (def.isReference != isRef)
        {
            def.isReference = isRef;
            if (isRef)
            {
                def.matchScore = 100.0f;
                def.matchReason = "Active Reference Sample";
                def.alternativeIndex = 0;
                def.totalAlternatives = 1;
            }
            pad->setPadDefinition(def);
        }
        if (isRef)
            matchedPadId = def.padId;
    }
    return matchedPadId;
}

std::optional<PadDefinition> PadGridComponent::getPadDefinition(int padIndex) const
{
    for (const auto& pad : padComponents)
    {
        if (pad->getPadDefinition().index == padIndex)
            return pad->getPadDefinition();
    }
    return std::nullopt;
}

void PadGridComponent::selectPad(int padIndex)
{
    selectedPadIndex = padIndex;
    for (auto& p : padComponents)
    {
        const bool match = (p->getPadDefinition().index == padIndex);
        p->setSelected(match);
        if (match && onPadSelected)
            onPadSelected(p->getPadDefinition());
    }
}

// ==============================================================================
// Drag & Drop
// ==============================================================================

bool PadGridComponent::isInterestedInDragSource(const SourceDetails& details)
{
    return details.description.toString().startsWith("drummatch_sample:");
}

void PadGridComponent::highlightTargetPadForSample(const juce::String& sampleId)
{
    if (!getSampleCategoryCallback)
        return;

    const auto cat = getSampleCategoryCallback(sampleId);
    const int targetPad = findPadIndexForCategory(cat);

    if (highlightedDragPadIndex != targetPad)
    {
        clearDragHighlight();
        highlightedDragPadIndex = targetPad;

        for (auto& pad : padComponents)
        {
            if (pad->getPadDefinition().index == targetPad)
            {
                pad->setDragTargetHighlighted(true);
                break;
            }
        }
    }
}

void PadGridComponent::clearDragHighlight()
{
    for (auto& pad : padComponents)
        pad->setDragTargetHighlighted(false);
    highlightedDragPadIndex = -1;
}

void PadGridComponent::itemDragEnter(const SourceDetails& details)
{
    const juce::String sampleId = details.description.toString().fromFirstOccurrenceOf("drummatch_sample:", false, false);
    highlightTargetPadForSample(sampleId);
}

void PadGridComponent::itemDragMove(const SourceDetails& details)
{
    const juce::String sampleId = details.description.toString().fromFirstOccurrenceOf("drummatch_sample:", false, false);
    highlightTargetPadForSample(sampleId);
}

void PadGridComponent::itemDragExit(const SourceDetails&)
{
    clearDragHighlight();
}

void PadGridComponent::itemDropped(const SourceDetails& details)
{
    const juce::String sampleId = details.description.toString().fromFirstOccurrenceOf("drummatch_sample:", false, false);

    int targetPad = highlightedDragPadIndex;
    if (targetPad < 0 && getSampleCategoryCallback)
        targetPad = findPadIndexForCategory(getSampleCategoryCallback(sampleId));

    clearDragHighlight();

    if (targetPad >= 0 && onSampleDroppedOnPad)
    {
        onSampleDroppedOnPad(sampleId, targetPad);
    }
}

// ==============================================================================

void PadGridComponent::paint(juce::Graphics& g)
{
    // Placa gris verdosa (#A3A69B)
    g.fillAll(theme::panelStandard);

    g.setColour(theme::chassis.brighter(0.1f));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);

    // Serigrafía "PAD GRID - MPC LIVE 2"
    g.setColour(theme::textSecondary);
    g.setFont(theme::getSerigraphyFont(11.0f));
    g.drawText("PAD GRID - MPC LIVE 2", 14, 10, 220, 20, juce::Justification::centredLeft, false);
}

void PadGridComponent::resized()
{
    auto area = getLocalBounds();

    // Encabezado
    auto headerArea = area.removeFromTop(38).reduced(12, 4);
    headerArea.removeFromLeft(220); // Espacio del título

    autoKitButton.setBounds(headerArea.removeFromRight(88).withHeight(30));
    headerArea.removeFromRight(8);
    varyButton.setBounds(headerArea.removeFromRight(64).withHeight(30));

    if (padComponents.size() < 16)
        return;

    area.reduce(10, 6);

    const int cols = 4;
    const int rows = 4;
    const int gap = 8;

    const int padW = (area.getWidth() - (cols - 1) * gap) / cols;
    const int padH = (area.getHeight() - (rows - 1) * gap) / rows;

    const int layoutMap[4][4] = {
        { 12, 13, 14, 15 },
        {  8,  9, 10, 11 },
        {  4,  5,  6,  7 },
        {  0,  1,  2,  3 }
    };

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            const int padIdx = layoutMap[r][c];
            if (padIdx >= 0 && padIdx < static_cast<int>(padComponents.size()))
            {
                const int x = area.getX() + c * (padW + gap);
                const int y = area.getY() + r * (padH + gap);
                padComponents[static_cast<size_t>(padIdx)]->setBounds(x, y, padW, padH);
            }
        }
    }
}

} // namespace drummatch
