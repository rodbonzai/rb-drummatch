#pragma once

#include <queue>
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "SampleCatalog.h"
#include "../Domain/LibraryFolder.h"
#include "../Domain/Sample.h"

namespace drummatch
{

class LibraryScanner : private juce::Thread
{
public:
    LibraryScanner(SampleCatalog& catalog);
    ~LibraryScanner() override;

    // Inicia el escaneo asíncrono de una carpeta específica
    void scanFolderAsync(const LibraryFolder& folder);

    // Inicia el escaneo de todas las carpetas habilitadas
    void scanAllEnabledFoldersAsync();

    // Cancela el escaneo actual
    void cancelScan();

    bool isCurrentlyScanning() const;

private:
    void run() override;

    void processFolder(const LibraryFolder& folder);
    bool isAudioFile(const juce::File& file) const;
    Sample createSampleFromFile(const juce::File& file, const juce::File& rootDir, const juce::String& libraryName);

    SampleCatalog& catalog;
    juce::AudioFormatManager formatManager;

    std::queue<LibraryFolder> scanQueue;
    juce::CriticalSection queueLock;

    std::atomic<bool> isScanning { false };
};

} // namespace drummatch

