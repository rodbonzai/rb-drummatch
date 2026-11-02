#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Audio/AudioEngine.h"
#include "MPC60Theme.h"

namespace drummatch
{

class WaveformComponent : public juce::Component,
                          public juce::Timer,
                          public AudioEngine::Listener,
                          public juce::ChangeListener
{
public:
    explicit WaveformComponent(AudioEngine& engine);
    ~WaveformComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;

    // AudioEngine::Listener callbacks
    void playbackStateChanged(bool isPlaying) override;
    void sampleLoaded(const juce::File& file) override;

    // juce::ChangeListener callback (AudioThumbnail)
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void loadFile(const juce::File& file);

private:
    AudioEngine& audioEngine;

    juce::AudioThumbnailCache thumbnailCache { 5 };
    juce::AudioThumbnail thumbnail;

    class TransportButton : public juce::Button
    {
    public:
        enum class Type { Play, Pause, Stop };
        explicit TransportButton(Type t);
        void setType(Type t);
        void paintButton(juce::Graphics& g, bool isHighlighted, bool isDown) override;

    private:
        Type type;
    };

    TransportButton playButton { TransportButton::Type::Play };
    TransportButton stopButton { TransportButton::Type::Stop };

    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    juce::Label sampleNameLabel;
    juce::Label formatBadgeLabel;
    juce::Label timeLabel;

    class MPC60KnobLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider& slider) override;
    };

    MPC60KnobLookAndFeel knobLookAndFeel;

    juce::File currentFile;
    juce::Rectangle<int> lcdArea;
    juce::Rectangle<int> waveArea;
};

} // namespace drummatch
