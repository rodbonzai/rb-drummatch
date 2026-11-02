#include "IconButton.h"
#include <cmath>

namespace drummatch
{

IconButton::IconButton(const juce::String& buttonName, IconType icon, theme::ButtonStyle style)
    : juce::Button(buttonName), iconType(icon), buttonStyle(style)
{
}

void IconButton::setIconType(IconType newType)
{
    iconType = newType;
    repaint();
}

void IconButton::setButtonStyle(theme::ButtonStyle newStyle)
{
    buttonStyle = newStyle;
    repaint();
}

juce::Path IconButton::createPlayPath()
{
    juce::Path p;
    p.addTriangle(0.0f, 0.0f, 0.0f, 10.0f, 9.0f, 5.0f);
    return p;
}

juce::Path IconButton::createPausePath()
{
    juce::Path p;
    p.addRoundedRectangle(0.0f, 0.0f, 3.0f, 10.0f, 0.5f);
    p.addRoundedRectangle(5.0f, 0.0f, 3.0f, 10.0f, 0.5f);
    return p;
}

juce::Path IconButton::createStopPath()
{
    juce::Path p;
    p.addRoundedRectangle(0.0f, 0.0f, 9.0f, 9.0f, 1.0f);
    return p;
}

juce::Path IconButton::createStarPath()
{
    juce::Path p;
    p.addTriangle(0.0f, 0.0f, 0.0f, 10.0f, 9.0f, 5.0f); // Default clean action
    return p;
}

juce::Path IconButton::createCrossPath()
{
    juce::Path p;
    p.startNewSubPath(1.5f, 1.5f);
    p.lineTo(8.5f, 8.5f);
    p.startNewSubPath(8.5f, 1.5f);
    p.lineTo(1.5f, 8.5f);
    juce::Path stroked;
    juce::PathStrokeType(1.8f, juce::PathStrokeType::mitered, juce::PathStrokeType::square).createStrokedPath(stroked, p);
    return stroked;
}

juce::Path IconButton::createResetPath()
{
    juce::Path p;
    p.addCentredArc(5.0f, 5.0f, 4.0f, 4.0f, 0.0f,
                    0.3f * juce::MathConstants<float>::pi,
                    2.1f * juce::MathConstants<float>::pi,
                    true);
    p.startNewSubPath(6.0f, 1.0f);
    p.lineTo(9.0f, 3.0f);
    p.lineTo(6.0f, 5.0f);
    return p;
}

juce::Path IconButton::createFolderPath()
{
    juce::Path p;
    p.startNewSubPath(0.0f, 1.5f);
    p.lineTo(3.5f, 1.5f);
    p.lineTo(5.0f, 3.0f);
    p.lineTo(11.0f, 3.0f);
    p.lineTo(11.0f, 9.5f);
    p.lineTo(0.0f, 9.5f);
    p.closeSubPath();
    return p;
}

juce::Path IconButton::createDotsPath()
{
    juce::Path p;
    p.addEllipse(0.0f, 3.0f, 3.0f, 3.0f);
    p.addEllipse(4.5f, 3.0f, 3.0f, 3.0f);
    p.addEllipse(9.0f, 3.0f, 3.0f, 3.0f);
    return p;
}

void IconButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::Path icon;
    bool hasIcon = (iconType != IconType::None);

    switch (iconType)
    {
        case IconType::Play:   icon = createPlayPath(); break;
        case IconType::Pause:  icon = createPausePath(); break;
        case IconType::Stop:   icon = createStopPath(); break;
        case IconType::Folder: icon = createFolderPath(); break;
        case IconType::Cross:  icon = createCrossPath(); break;
        case IconType::Reset:  icon = createResetPath(); break;
        case IconType::Dots:   icon = createDotsPath(); break;
        case IconType::Star:   icon = createPlayPath(); break;
        default: break;
    }

    theme::drawKeypadButton(g, getLocalBounds().toFloat(),
                            buttonStyle,
                            shouldDrawButtonAsDown,
                            shouldDrawButtonAsHighlighted,
                            getButtonText(),
                            hasIcon ? &icon : nullptr);
}

} // namespace drummatch
