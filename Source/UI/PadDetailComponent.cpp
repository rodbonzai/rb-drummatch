#include "PadDetailComponent.h"

namespace drummatch
{

PadDetailComponent::PadDetailComponent()
{
    playSampleButton.onClick = [this]()
    {
        if (currentPad.has_value() && onPlaySampleRequested)
            onPlaySampleRequested(currentPad->index);
    };
    addAndMakeVisible(playSampleButton);

    clearSampleButton.onClick = [this]()
    {
        if (currentPad.has_value() && onClearSampleRequested)
            onClearSampleRequested(currentPad->index);
    };
    addAndMakeVisible(clearSampleButton);

    resetButton.onClick = [this]()
    {
        if (onResetLayoutRequested)
            onResetLayoutRequested();
    };
    addAndMakeVisible(resetButton);

    prevCandButton.onClick = [this]()
    {
        if (currentPad.has_value() && onPrevCandidateRequested)
            onPrevCandidateRequested(currentPad->index);
    };
    addAndMakeVisible(prevCandButton);

    nextCandButton.onClick = [this]()
    {
        if (currentPad.has_value() && onNextCandidateRequested)
            onNextCandidateRequested(currentPad->index);
    };
    addAndMakeVisible(nextCandButton);

    clearSelection();
}

void PadDetailComponent::setSelectedPad(const PadDefinition& pad)
{
    currentPad = pad;

    padIdText = "PAD " + pad.padId;

    const juce::String labelStr = pad.label.isNotEmpty() ? pad.label.toUpperCase() : "PAD " + pad.padId;
    const juce::String catStr = getCategoryName(pad.category).toUpperCase();

    if (pad.assignedSample.has_value())
    {
        infoText = labelStr + "  /  " + catStr + "  /  " + pad.assignedSample->getFileName().toUpperCase();
        if (pad.isReference)
        {
            infoText += "  [REF]";
        }
        else if (pad.matchScore > 0.0f)
        {
            infoText += "  [" + juce::String(juce::roundToInt(pad.matchScore)) + "% MATCH]";
        }
        playSampleButton.setEnabled(true);
        clearSampleButton.setEnabled(true);
    }
    else
    {
        infoText = labelStr + "  /  " + catStr + "  /  EMPTY";
        playSampleButton.setEnabled(false);
        clearSampleButton.setEnabled(false);
    }

    if (pad.isReference)
    {
        candCountText = "REF";
        prevCandButton.setEnabled(false);
        nextCandButton.setEnabled(false);
    }
    else if (pad.totalAlternatives > 0)
    {
        candCountText = juce::String(pad.alternativeIndex + 1) + "/" + juce::String(pad.totalAlternatives);
        prevCandButton.setEnabled(pad.alternativeIndex > 0);
        nextCandButton.setEnabled(pad.alternativeIndex < pad.totalAlternatives - 1);
    }
    else
    {
        candCountText = "- / -";
        prevCandButton.setEnabled(false);
        nextCandButton.setEnabled(false);
    }

    repaint();
}

void PadDetailComponent::clearSelection()
{
    currentPad.reset();

    padIdText = "PAD ---";
    infoText = "NO PAD SELECTED";
    candCountText = "- / -";

    playSampleButton.setEnabled(false);
    clearSampleButton.setEnabled(false);
    prevCandButton.setEnabled(false);
    nextCandButton.setEnabled(false);

    repaint();
}

void PadDetailComponent::paint(juce::Graphics& g)
{
    // Placa gris verdosa (#A3A69B) que unifica el panel central
    g.fillAll(theme::panelStandard);

    // 1. Campo LCD para ID del Pad (ej: "PAD A03")
    if (!padIdArea.isEmpty())
    {
        theme::drawLCDBox(g, padIdArea.toFloat(), 2.0f);
        g.setColour(theme::lcdText);
        g.setFont(theme::getMonoFont(11.0f).withStyle(juce::Font::bold));
        g.drawText(padIdText, padIdArea, juce::Justification::centred, false);
    }

    // 2. Campo LCD central ancho: "LABEL / CAT / FILENAME"
    if (!infoArea.isEmpty())
    {
        theme::drawLCDBox(g, infoArea.toFloat(), 2.0f);
        g.setColour(theme::lcdText);
        g.setFont(theme::getMonoFont(11.0f));
        g.drawText(infoText, infoArea.reduced(8, 0), juce::Justification::centredLeft, true);
    }

    // 3. Campo LCD pequeño para contador de candidatos (ej: "1/3")
    if (!candCountArea.isEmpty())
    {
        theme::drawLCDBox(g, candCountArea.toFloat(), 2.0f);
        g.setColour(theme::lcdText);
        g.setFont(theme::getMonoFont(10.0f).withStyle(juce::Font::bold));
        g.drawText(candCountText, candCountArea, juce::Justification::centred, false);
    }
}

void PadDetailComponent::resized()
{
    auto area = getLocalBounds().reduced(10, 4);

    // Fila 1: LCD Pad ID, LCD Info, y navegación PREV · 1/3 · NEXT
    auto row1 = area.removeFromTop(28);

    padIdArea = row1.removeFromLeft(82);
    row1.removeFromLeft(6);

    nextCandButton.setBounds(row1.removeFromRight(52));
    row1.removeFromRight(4);
    candCountArea = row1.removeFromRight(40);
    row1.removeFromRight(4);
    prevCandButton.setBounds(row1.removeFromRight(52));
    row1.removeFromRight(6);

    infoArea = row1;

    area.removeFromTop(6);

    // Fila 2: PLAY, CLEAR PAD y a la derecha RESET MPC
    auto row2 = area.removeFromTop(28);

    playSampleButton.setBounds(row2.removeFromLeft(76));
    row2.removeFromLeft(6);
    clearSampleButton.setBounds(row2.removeFromLeft(96));

    resetButton.setBounds(row2.removeFromRight(96));
}

} // namespace drummatch
