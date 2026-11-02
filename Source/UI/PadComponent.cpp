#include "PadComponent.h"

namespace drummatch
{

PadComponent::PadComponent(const PadDefinition& def)
    : definition(def)
{
    setRepaintsOnMouseActivity(true);
}

PadComponent::~PadComponent()
{
    stopTimer();
}

void PadComponent::setPadDefinition(const PadDefinition& def)
{
    definition = def;
    repaint();
}

void PadComponent::setSelected(bool isSelected)
{
    if (selected != isSelected)
    {
        selected = isSelected;
        repaint();
    }
}

void PadComponent::setDragTargetHighlighted(bool highlighted)
{
    if (isDragHighlighted != highlighted)
    {
        isDragHighlighted = highlighted;
        repaint();
    }
}

void PadComponent::triggerPressAnimation()
{
    isTriggered = true;
    repaint();
    startTimer(110);
}

void PadComponent::timerCallback()
{
    stopTimer();
    isTriggered = false;
    repaint();
}

void PadComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void PadComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    isPressed = false;
    repaint();
}

void PadComponent::mouseDown(const juce::MouseEvent&)
{
    isPressed = true;
    triggerPressAnimation();
    if (onClicked)
        onClicked(definition);
    repaint();
}

void PadComponent::mouseUp(const juce::MouseEvent&)
{
    isPressed = false;
    repaint();
}

void PadComponent::paint(juce::Graphics& g)
{
    const auto fullArea = getLocalBounds().toFloat().reduced(2.0f);
    const bool isRef = definition.isReference;
    const bool isDown = isPressed || isTriggered;

    // 1. Contorno de Selección o Resaltado de Referencia
    if (isRef)
    {
        // El Pad de Referencia tiene contorno dorado prominente
        g.setColour(theme::padRefBorder);
        g.drawRoundedRectangle(fullArea, 4.0f, 2.0f);
    }
    else if (selected || isDragHighlighted)
    {
        // Pad seleccionado normal: contorno rojo del color del LED
        g.setColour(theme::padLed);
        g.drawRoundedRectangle(fullArea, 4.0f, 2.0f);
    }

    // 2. Área del cuerpo físico del pad
    auto padArea = fullArea.reduced((selected || isDragHighlighted || isRef) ? 3.0f : 1.0f);
    const float yOffset = isDown ? 2.0f : 0.0f;
    padArea = padArea.withTrimmedBottom(isDown ? 1.0f : 3.0f).translated(0.0f, yOffset);

    // Sombra exterior sólida inferior (3px cuando no está presionado)
    if (!isDown)
    {
        g.setColour(theme::padShadow);
        g.fillRoundedRectangle(padArea.translated(0.0f, 3.0f), 3.0f);
    }

    // Cuerpo de goma del pad (#5F5E5C)
    juce::Colour padColor = theme::padBody;
    if (isHovered)
        padColor = padColor.brighter(0.08f);

    g.setColour(padColor);
    g.fillRoundedRectangle(padArea, 3.0f);

    // Textura / bisel de goma: sombra interior inferior
    g.setColour(juce::Colour(0x35000000));
    g.fillRoundedRectangle(padArea.withTrimmedTop(padArea.getHeight() * 0.7f), 3.0f);

    // Borde fino del pad
    g.setColour(isRef ? theme::padRefBorder.withAlpha(0.6f) : juce::Colour(0xff444341));
    g.drawRoundedRectangle(padArea, 3.0f, 1.0f);

    // Brillo sutil superior de 1 px
    if (!isDown)
    {
        g.setColour(juce::Colour(0x25ffffff));
        g.drawLine(padArea.getX() + 2.0f, padArea.getY() + 1.0f,
                   padArea.getRight() - 2.0f, padArea.getY() + 1.0f, 1.0f);
    }

    // 3. LED en la esquina superior derecha
    const float ledSize = 8.0f;
    const auto ledRect = juce::Rectangle<float>(padArea.getRight() - ledSize - 8.0f,
                                                padArea.getY() + 7.0f,
                                                ledSize, ledSize);

    const bool hasSample = definition.assignedSample.has_value();
    const juce::Colour activeLedColor = isRef ? theme::padRefLed : theme::padLed;

    if (selected || isDown)
    {
        // LED al 100% con resplandor amplio
        g.setColour(activeLedColor.withAlpha(0.40f));
        g.fillEllipse(ledRect.expanded(4.0f));

        g.setColour(activeLedColor);
        g.fillEllipse(ledRect);

        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.fillEllipse(ledRect.reduced(2.0f));
    }
    else if (isRef)
    {
        // Referencia sin estar seleccionada: LED dorado brillante
        g.setColour(theme::padRefLed.withAlpha(0.35f));
        g.fillEllipse(ledRect.expanded(2.5f));

        g.setColour(theme::padRefLed);
        g.fillEllipse(ledRect);

        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.fillEllipse(ledRect.reduced(2.0f));
    }
    else if (hasSample)
    {
        // Asignado normal = LED al 45%
        g.setColour(theme::padLed.withAlpha(0.45f));
        g.fillEllipse(ledRect);

        g.setColour(juce::Colour(0xff331110));
        g.drawEllipse(ledRect, 1.0f);
    }
    else
    {
        // Vacío = LED apagado (#383838)
        g.setColour(juce::Colour(0xff383838));
        g.fillEllipse(ledRect);

        g.setColour(juce::Colour(0xff222222));
        g.drawEllipse(ledRect, 1.0f);
    }

    // 4. Badge "REF" en el Pad de Referencia o Porcentaje de Compatibilidad
    if (isRef)
    {
        const auto refBadge = juce::Rectangle<float>(ledRect.getX() - 36.0f, padArea.getY() + 6.0f, 30.0f, 11.0f);
        g.setColour(theme::padRefBadgeBg);
        g.fillRoundedRectangle(refBadge, 2.0f);
        g.setColour(juce::Colour(0xff1C1C1C));
        g.drawRoundedRectangle(refBadge, 2.0f, 1.0f);

        g.setColour(theme::padRefBadgeText);
        g.setFont(theme::getMonoFont(8.5f).withStyle(juce::Font::bold));
        g.drawText("REF", refBadge, juce::Justification::centred, false);
    }
    else if (definition.matchScore > 0.0f && hasSample && !isDragHighlighted)
    {
        const auto scoreBadge = juce::Rectangle<float>(ledRect.getX() - 36.0f, padArea.getY() + 6.0f, 30.0f, 11.0f);

        juce::Colour scoreBg;
        juce::Colour scoreTextCol;
        if (definition.matchScore >= 80.0f)
        {
            // Alta compatibilidad (Verde LCD estilo MPC)
            scoreBg = theme::lcdBg;
            scoreTextCol = theme::lcdText;
        }
        else if (definition.matchScore >= 60.0f)
        {
            // Media compatibilidad (Crema clásica hardware)
            scoreBg = theme::btnCreamTop;
            scoreTextCol = theme::textMain;
        }
        else
        {
            // Baja compatibilidad (Gris carbón)
            scoreBg = theme::btnDarkTop;
            scoreTextCol = theme::btnDarkText;
        }

        g.setColour(scoreBg);
        g.fillRoundedRectangle(scoreBadge, 2.0f);
        g.setColour(juce::Colour(0xff1C1C1C));
        g.drawRoundedRectangle(scoreBadge, 2.0f, 1.0f);

        g.setColour(scoreTextCol);
        g.setFont(theme::getMonoFont(8.5f).withStyle(juce::Font::bold));
        const juce::String scoreText = juce::String(juce::roundToInt(definition.matchScore)) + "%";
        g.drawText(scoreText, scoreBadge, juce::Justification::centred, false);
    }

    // 5. Textos del Pad (#F0EDE0)
    // ID + Atajo de teclado arriba a la izquierda: ej "A01 [Z]"
    juce::String padHeader = definition.padId;
    if (definition.keyShortcut.isNotEmpty())
        padHeader += " [" + definition.keyShortcut + "]";

    g.setColour(isRef ? theme::padRefLed : theme::padText.withAlpha(0.85f));
    g.setFont(theme::getMonoFont(9.5f).withStyle(juce::Font::bold));
    g.drawText(padHeader,
               padArea.toNearestInt().reduced(8, 6),
               juce::Justification::topLeft, false);

    // Etiqueta del instrumento en 11 px bold
    g.setColour(theme::padText);
    g.setFont(theme::getSerigraphyFont(11.0f));
    auto labelArea = padArea.toNearestInt();
    labelArea.removeFromTop(20);
    labelArea.removeFromBottom(24);
    g.drawText(definition.label, labelArea, juce::Justification::centred, true);

    // Nombre del sample abajo en mono
    auto sampleArea = padArea.toNearestInt();
    sampleArea = sampleArea.removeFromBottom(22).reduced(6, 2);

    if (hasSample)
    {
        g.setColour(isRef ? theme::padRefLed : theme::padText);
        g.setFont(theme::getMonoFont(9.5f).withStyle(isRef ? juce::Font::bold : juce::Font::plain));
        g.drawText(definition.assignedSample->getFileName().toUpperCase(),
                   sampleArea, juce::Justification::centred, true);
    }
    else
    {
        g.setColour(theme::padText.withAlpha(0.35f));
        g.setFont(theme::getMonoFont(9.0f));
        g.drawText("- EMPTY -", sampleArea, juce::Justification::centred, false);
    }
}

} // namespace drummatch
