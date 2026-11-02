#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Domain/PadDefinition.h"
#include "MPC60Theme.h"

namespace drummatch
{

class PadComponent : public juce::Component,
                     private juce::Timer
{
public:
    explicit PadComponent(const PadDefinition& def);
    ~PadComponent() override;

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setPadDefinition(const PadDefinition& def);
    const PadDefinition& getPadDefinition() const { return definition; }

    void setSelected(bool isSelected);
    bool isSelected() const { return selected; }

    void setDragTargetHighlighted(bool highlighted);
    bool isDragTargetHighlighted() const { return isDragHighlighted; }

    void triggerPressAnimation();

    std::function<void(const PadDefinition&)> onClicked;

private:
    void timerCallback() override;

    PadDefinition definition;
    bool isHovered = false;
    bool isPressed = false;
    bool isTriggered = false;
    bool selected = false;
    bool isDragHighlighted = false;
};

} // namespace drummatch
