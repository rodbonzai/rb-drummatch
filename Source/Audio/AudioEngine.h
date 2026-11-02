#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace drummatch
{

class AudioEngine : public juce::ChangeListener
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void playSample(const juce::File& audioFile);
    void playPadSample(const juce::File& audioFile, int padIndex = -1);
    void stopPadVoice(int padIndex);
    void play();
    void stop();
    bool isPlaying() const;

    double getCurrentPosition() const;
    double getLengthInSeconds() const;
    void setPosition(double newPositionInSeconds);

    void setGain(float newGain);
    float getGain() const;

    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    juce::File getCurrentFile() const { return currentFile; }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void playbackStateChanged(bool isPlaying) {}
        virtual void sampleLoaded(const juce::File& file) {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    class PolyVoice : public juce::AudioSource
    {
    public:
        PolyVoice() = default;
        ~PolyVoice() override = default;

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

        void play(std::unique_ptr<juce::AudioFormatReader> reader, double targetSampleRate);
        void stop();
        bool isPlaying() const { return active.load(); }

        juce::File getCurrentFile() const { return currentFile; }
        void setCurrentFile(const juce::File& f) { currentFile = f; }

    private:
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        std::unique_ptr<juce::ResamplingAudioSource> resampler;
        std::atomic<bool> active { false };
        double currentSampleRate = 44100.0;
        juce::File currentFile;
        juce::SpinLock lock;
    };

    class MasterGainAudioSource : public juce::AudioSource
    {
    public:
        explicit MasterGainAudioSource(juce::AudioSource* input) : inputSource(input) {}

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
        {
            if (inputSource != nullptr)
                inputSource->prepareToPlay(samplesPerBlockExpected, sampleRate);
        }

        void releaseResources() override
        {
            if (inputSource != nullptr)
                inputSource->releaseResources();
        }

        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
        {
            if (inputSource != nullptr)
            {
                inputSource->getNextAudioBlock(bufferToFill);
                const float g = gain.load();
                if (std::abs(g - 1.0f) > 0.001f)
                    bufferToFill.buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, g);
            }
        }

        void setGain(float newGain) { gain.store(juce::jlimit(0.0f, 1.5f, newGain)); }
        float getGain() const { return gain.load(); }

    private:
        juce::AudioSource* inputSource = nullptr;
        std::atomic<float> gain { 0.9f };
    };

    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    juce::AudioSourcePlayer sourcePlayer;

    juce::MixerAudioSource masterMixer;
    MasterGainAudioSource masterGainSource { &masterMixer };

    // Transporte para preview en la barra inferior (seeking, playhead de forma de onda)
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> currentReaderSource;

    // Voces polifónicas para pads (16 voces simultáneas sin cortes)
    static constexpr int numVoices = 16;
    juce::MixerAudioSource padMixer;
    std::vector<std::unique_ptr<PolyVoice>> voices;
    int nextVoice = 0;

    juce::File currentFile;
    juce::ListenerList<Listener> listeners;
};

} // namespace drummatch
