#include "AudioEngine.h"

namespace drummatch
{

// ==============================================================================
// PolyVoice
// ==============================================================================

void AudioEngine::PolyVoice::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    const juce::SpinLock::ScopedLockType sl(lock);
    if (readerSource != nullptr)
        readerSource->prepareToPlay(samplesPerBlockExpected, sampleRate);
    if (resampler != nullptr)
        resampler->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioEngine::PolyVoice::releaseResources()
{
    const juce::SpinLock::ScopedLockType sl(lock);
    if (readerSource != nullptr)
        readerSource->releaseResources();
    if (resampler != nullptr)
        resampler->releaseResources();
}

void AudioEngine::PolyVoice::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (!active.load())
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    const juce::SpinLock::ScopedTryLockType sl(lock);
    if (!sl.isLocked())
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    juce::AudioSource* src = (resampler != nullptr) ? static_cast<juce::AudioSource*>(resampler.get())
                                                    : static_cast<juce::AudioSource*>(readerSource.get());
    if (src != nullptr)
    {
        src->getNextAudioBlock(bufferToFill);

        if (readerSource != nullptr && readerSource->getNextReadPosition() >= readerSource->getTotalLength())
        {
            active.store(false);
        }
    }
    else
    {
        bufferToFill.clearActiveBufferRegion();
        active.store(false);
    }
}

void AudioEngine::PolyVoice::play(std::unique_ptr<juce::AudioFormatReader> reader, double targetSampleRate)
{
    if (reader == nullptr)
        return;

    const double srcRate = reader->sampleRate;
    auto newReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    newReaderSource->setLooping(false);

    std::unique_ptr<juce::ResamplingAudioSource> newResampler;
    const double effectiveTarget = (targetSampleRate > 0.0) ? targetSampleRate : currentSampleRate;

    if (effectiveTarget > 0.0 && std::abs(srcRate - effectiveTarget) > 1.0)
    {
        newResampler = std::make_unique<juce::ResamplingAudioSource>(newReaderSource.get(), false, 2);
        newResampler->setResamplingRatio(srcRate / effectiveTarget);
        newResampler->prepareToPlay(512, effectiveTarget);
    }
    else if (effectiveTarget > 0.0)
    {
        newReaderSource->prepareToPlay(512, effectiveTarget);
    }

    {
        const juce::SpinLock::ScopedLockType sl(lock);
        readerSource = std::move(newReaderSource);
        resampler = std::move(newResampler);
        active.store(true);
    }
}

void AudioEngine::PolyVoice::stop()
{
    active.store(false);
    const juce::SpinLock::ScopedLockType sl(lock);
    if (readerSource != nullptr)
        readerSource->setNextReadPosition(0);
}

// ==============================================================================
// AudioEngine
// ==============================================================================

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();

    // 1. Inicializar las 16 voces polifónicas para los pads
    voices.reserve(numVoices);
    for (int i = 0; i < numVoices; ++i)
    {
        auto voice = std::make_unique<PolyVoice>();
        padMixer.addInputSource(voice.get(), false);
        voices.push_back(std::move(voice));
    }

    // 2. Conectar transportSource (audición/forma de onda) y padMixer al masterMixer
    masterMixer.addInputSource(&transportSource, false);
    masterMixer.addInputSource(&padMixer, false);

    // 3. Inicializar el dispositivo con masterGainSource
    deviceManager.initialiseWithDefaultDevices(0, 2);
    sourcePlayer.setSource(&masterGainSource);
    deviceManager.addAudioCallback(&sourcePlayer);

    transportSource.addChangeListener(this);
}

AudioEngine::~AudioEngine()
{
    transportSource.removeChangeListener(this);
    transportSource.stop();
    transportSource.setSource(nullptr);

    for (auto& v : voices)
        v->stop();

    deviceManager.removeAudioCallback(&sourcePlayer);
    sourcePlayer.setSource(nullptr);
}

void AudioEngine::stopPadVoice(int padIndex)
{
    if (padIndex >= 0 && padIndex < numVoices)
        voices[static_cast<size_t>(padIndex)]->stop();
}

void AudioEngine::playPadSample(const juce::File& audioFile, int padIndex)
{
    if (!audioFile.existsAsFile())
        return;

    currentFile = audioFile;

    // Notificar a los listeners para actualizar waveform display e info
    listeners.call([&audioFile](Listener& l) { l.sampleLoaded(audioFile); });

    // Preparar y disparar voz monofónica por pad (sin solapamiento en el mismo pad)
    if (auto* reader = formatManager.createReaderFor(audioFile))
    {
        double targetSampleRate = 44100.0;
        if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
            targetSampleRate = currentDevice->getCurrentSampleRate();

        int chosenVoice = -1;

        if (padIndex >= 0 && padIndex < numVoices)
        {
            // Asignación directa 1-a-1: cada pad tiene su propia voz fija.
            // Al tocar el mismo pad, su voz previa se corta y se reinicia de inmediato (retrigger monofónico).
            chosenVoice = padIndex;
        }
        else
        {
            // Si no se especificó padIndex, cortar cualquier voz que ya esté reproduciendo este mismo archivo
            for (int i = 0; i < numVoices; ++i)
            {
                if (voices[static_cast<size_t>(i)]->isPlaying() &&
                    voices[static_cast<size_t>(i)]->getCurrentFile() == audioFile)
                {
                    chosenVoice = i;
                    break;
                }
            }

            if (chosenVoice < 0)
            {
                for (int i = 0; i < numVoices; ++i)
                {
                    const int idx = (nextVoice + i) % numVoices;
                    if (!voices[static_cast<size_t>(idx)]->isPlaying())
                    {
                        chosenVoice = idx;
                        break;
                    }
                }
            }

            if (chosenVoice < 0)
                chosenVoice = nextVoice;

            nextVoice = (chosenVoice + 1) % numVoices;
        }

        auto& voice = voices[static_cast<size_t>(chosenVoice)];
        voice->setCurrentFile(audioFile);
        voice->play(std::unique_ptr<juce::AudioFormatReader>(reader), targetSampleRate);
    }
}

void AudioEngine::playSample(const juce::File& audioFile)
{
    if (!audioFile.existsAsFile())
        return;

    stop();
    transportSource.setSource(nullptr);
    currentReaderSource.reset();
    currentFile = audioFile;

    if (auto* reader = formatManager.createReaderFor(audioFile))
    {
        currentReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(currentReaderSource.get(), 0, nullptr, reader->sampleRate);
        transportSource.setPosition(0.0);
        transportSource.start();

        listeners.call([&audioFile](Listener& l) { l.sampleLoaded(audioFile); });
    }
}

void AudioEngine::play()
{
    transportSource.start();
}

void AudioEngine::stop()
{
    transportSource.stop();
    transportSource.setPosition(0.0);

    for (auto& v : voices)
        v->stop();
}

bool AudioEngine::isPlaying() const
{
    return transportSource.isPlaying();
}

double AudioEngine::getCurrentPosition() const
{
    return transportSource.getCurrentPosition();
}

double AudioEngine::getLengthInSeconds() const
{
    return transportSource.getLengthInSeconds();
}

void AudioEngine::setPosition(double newPositionInSeconds)
{
    transportSource.setPosition(newPositionInSeconds);
}

void AudioEngine::setGain(float newGain)
{
    masterGainSource.setGain(newGain);
    transportSource.setGain(newGain);
}

float AudioEngine::getGain() const
{
    return masterGainSource.getGain();
}

void AudioEngine::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        const bool playing = transportSource.isPlaying();
        listeners.call([playing](Listener& l) { l.playbackStateChanged(playing); });
    }
}

void AudioEngine::addListener(Listener* listener)
{
    listeners.add(listener);
}

void AudioEngine::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

} // namespace drummatch
