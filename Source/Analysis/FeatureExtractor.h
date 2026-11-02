#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "../Domain/AcousticFeatures.h"

namespace drummatch
{

class FeatureExtractor
{
public:
    FeatureExtractor();
    ~FeatureExtractor() = default;

    AcousticFeatures extract(const juce::File& file, juce::AudioFormatManager& formatManager);
    AcousticFeatures extractFromBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);

private:
    static constexpr int fftOrder = 11;             // 2048 puntos
    static constexpr int fftSize = 1 << fftOrder;   // 2048

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
};

} // namespace drummatch

