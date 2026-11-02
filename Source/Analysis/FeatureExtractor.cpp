#include "FeatureExtractor.h"
#include <cmath>
#include <algorithm>

namespace drummatch
{

FeatureExtractor::FeatureExtractor()
    : fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
}

AcousticFeatures FeatureExtractor::extract(const juce::File& file, juce::AudioFormatManager& formatManager)
{
    if (!file.existsAsFile())
        return {};

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->lengthInSamples == 0)
        return {};

    const int maxSamples = static_cast<int>(juce::jmin<juce::int64>(reader->lengthInSamples,
                                                                   static_cast<juce::int64>(reader->sampleRate * 2.5))); // Máx 2.5s

    juce::AudioBuffer<float> readBuffer(static_cast<int>(reader->numChannels), maxSamples);
    reader->read(&readBuffer, 0, maxSamples, 0, true, true);

    // Mezclar a mono
    juce::AudioBuffer<float> monoBuffer(1, maxSamples);
    monoBuffer.clear();
    const float scale = 1.0f / static_cast<float>(readBuffer.getNumChannels());
    for (int ch = 0; ch < readBuffer.getNumChannels(); ++ch)
    {
        monoBuffer.addFrom(0, 0, readBuffer, ch, 0, maxSamples, scale);
    }

    return extractFromBuffer(monoBuffer, reader->sampleRate);
}

AcousticFeatures FeatureExtractor::extractFromBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    AcousticFeatures feat;
    const int numSamples = buffer.getNumSamples();
    if (numSamples < 128 || sampleRate <= 0.0)
        return feat;

    const float* channelData = buffer.getReadPointer(0);

    // 1. Dinámica básica
    feat.peak = buffer.getMagnitude(0, 0, numSamples);
    feat.rms = buffer.getRMSLevel(0, 0, numSamples);
    feat.crestFactor = (feat.rms > 1e-5f) ? (feat.peak / feat.rms) : 1.0f;

    // 2. Envolvente temporal y transiente
    int peakIndex = 0;
    float maxVal = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float absVal = std::abs(channelData[i]);
        if (absVal > maxVal)
        {
            maxVal = absVal;
            peakIndex = i;
        }
    }

    feat.attackTimeMs = static_cast<float>(peakIndex / sampleRate) * 1000.0f;

    // Decaimiento hasta el 10% (-20 dB) del pico
    const float threshold10 = maxVal * 0.1f;
    int decayIndex = numSamples;
    for (int i = peakIndex; i < numSamples; ++i)
    {
        if (std::abs(channelData[i]) < threshold10)
        {
            decayIndex = i;
            break;
        }
    }
    feat.decayTimeMs = static_cast<float>((decayIndex - peakIndex) / sampleRate) * 1000.0f;

    // Densidad del transiente inicial (primeros 25 ms)
    const int samples25ms = juce::jmin(numSamples, static_cast<int>(sampleRate * 0.025));
    const float transientRms = buffer.getRMSLevel(0, 0, samples25ms);
    feat.transientStrength = (feat.rms > 1e-5f) ? (transientRms / feat.rms) : 1.0f;

    // 3. Análisis espectral FFT
    std::array<float, fftSize * 2> fftData;
    std::fill(fftData.begin(), fftData.end(), 0.0f);

    // Tomar los primeros 2048 samples centrados en el transiente / inicio
    const int copyLen = juce::jmin(numSamples, fftSize);
    std::copy_n(channelData, copyLen, fftData.begin());

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // fftData ahora contiene fftSize / 2 magnitudes espectrales positivas
    const int numBins = fftSize / 2;
    const float binFreqWidth = static_cast<float>(sampleRate / static_cast<double>(fftSize));

    double sumMagnitudes = 0.0;
    double weightedFreqSum = 0.0;

    double subBassSum = 0.0;
    double bassSum = 0.0;
    double lowMidSum = 0.0;
    double midSum = 0.0;
    double highMidSum = 0.0;
    double highSum = 0.0;

    for (int i = 0; i < numBins; ++i)
    {
        const float mag = fftData[static_cast<size_t>(i)];
        const float freq = static_cast<float>(i) * binFreqWidth;

        sumMagnitudes += mag;
        weightedFreqSum += static_cast<double>(freq) * mag;

        // Distribución por bandas de frecuencia
        if (freq >= 20.0f && freq < 60.0f)
            subBassSum += mag;
        else if (freq >= 60.0f && freq < 250.0f)
            bassSum += mag;
        else if (freq >= 250.0f && freq < 500.0f)
            lowMidSum += mag;
        else if (freq >= 500.0f && freq < 2000.0f)
            midSum += mag;
        else if (freq >= 2000.0f && freq < 6000.0f)
            highMidSum += mag;
        else if (freq >= 6000.0f && freq < 20000.0f)
            highSum += mag;
    }

    if (sumMagnitudes > 1e-6)
    {
        // Spectral Centroid
        feat.spectralCentroid = static_cast<float>(weightedFreqSum / sumMagnitudes);

        // Spectral Rolloff (85% de la energía espectral)
        const double rolloffTarget = sumMagnitudes * 0.85;
        double accumulated = 0.0;
        feat.spectralRolloff = static_cast<float>(sampleRate * 0.5);
        for (int i = 0; i < numBins; ++i)
        {
            accumulated += fftData[static_cast<size_t>(i)];
            if (accumulated >= rolloffTarget)
            {
                feat.spectralRolloff = static_cast<float>(i) * binFreqWidth;
                break;
            }
        }

        // Energías normalizadas de bandas
        const double totalBandEnergy = subBassSum + bassSum + lowMidSum + midSum + highMidSum + highSum;
        if (totalBandEnergy > 1e-6)
        {
            feat.subBassEnergy = static_cast<float>(subBassSum / totalBandEnergy);
            feat.bassEnergy = static_cast<float>(bassSum / totalBandEnergy);
            feat.lowMidEnergy = static_cast<float>(lowMidSum / totalBandEnergy);
            feat.midEnergy = static_cast<float>(midSum / totalBandEnergy);
            feat.highMidEnergy = static_cast<float>(highMidSum / totalBandEnergy);
            feat.highEnergy = static_cast<float>(highSum / totalBandEnergy);
        }
    }

    feat.isValid = true;
    return feat;
}

} // namespace drummatch

