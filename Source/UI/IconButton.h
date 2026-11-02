#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MPC60Theme.h"

namespace drummatch
{

enum class IconType
{
    None,
    Play,
    Pause,
    Stop,
    Star,
    Cross,
    Reset,
    Folder,
    Dots
};

class IconButton : public juce::Button
{
public:
    IconButton(const juce::String& buttonName,
               IconType icon = IconType::None,
               theme::ButtonStyle style = theme::ButtonStyle::Cream);
    ~IconButton() override = default;

    void setIconType(IconType newType);
    void setButtonStyle(theme::ButtonStyle newStyle);

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    static juce::Path createPlayPath();
    static juce::Path createPausePath();
    static juce::Path createStopPath();
    static juce::Path createStarPath();
    static juce::Path createCrossPath();
    static juce::Path createResetPath();
    static juce::Path createFolderPath();
    static juce::Path createDotsPath();

private:
    IconType iconType = IconType::None;
    theme::ButtonStyle buttonStyle = theme::ButtonStyle::Cream;
};

} // namespace drummatch
