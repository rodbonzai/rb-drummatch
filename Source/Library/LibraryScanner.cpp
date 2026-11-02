#include "LibraryScanner.h"

namespace drummatch
{

LibraryScanner::LibraryScanner(SampleCatalog& cat)
    : juce::Thread("DrumMatchLibraryScanner"),
      catalog(cat)
{
    formatManager.registerBasicFormats(); // WAV y AIFF
}

LibraryScanner::~LibraryScanner()
{
    cancelScan();
    stopThread(4000);
}

void LibraryScanner::scanFolderAsync(const LibraryFolder& folder)
{
    {
        const juce::ScopedLock sl(queueLock);
        scanQueue.push(folder);
    }

    if (!isThreadRunning())
        startThread(juce::Thread::Priority::normal);
}

void LibraryScanner::scanAllEnabledFoldersAsync()
{
    const auto folders = catalog.getLibraryFolders();
    {
        const juce::ScopedLock sl(queueLock);
        for (const auto& f : folders)
        {
            if (f.isEnabled)
                scanQueue.push(f);
        }
    }

    if (!isThreadRunning())
        startThread(juce::Thread::Priority::normal);
}

void LibraryScanner::cancelScan()
{
    signalThreadShouldExit();
    {
        const juce::ScopedLock sl(queueLock);
        while (!scanQueue.empty())
            scanQueue.pop();
    }
}

bool LibraryScanner::isCurrentlyScanning() const
{
    return isScanning.load();
}

void LibraryScanner::run()
{
    isScanning.store(true);

    while (!threadShouldExit())
    {
        LibraryFolder currentFolder;
        {
            const juce::ScopedLock sl(queueLock);
            if (scanQueue.empty())
                break;
            currentFolder = scanQueue.front();
            scanQueue.pop();
        }

        processFolder(currentFolder);
    }

    isScanning.store(false);
}

bool LibraryScanner::isAudioFile(const juce::File& file) const
{
    const auto ext = file.getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".wave" || ext == ".aif" || ext == ".aiff";
}

Sample LibraryScanner::createSampleFromFile(const juce::File& file, const juce::File& rootDir, const juce::String& libraryName)
{
    Sample s;
    s.id = juce::String::toHexString(file.getFullPathName().hashCode64());
    s.file = file;
    s.relativePath = file.getRelativePathFrom(rootDir);
    s.libraryName = libraryName;
    s.fileSize = file.getSize();
    s.lastModified = file.getLastModificationTime();
    s.format = file.getFileExtension().toUpperCase().replace(".", "");
    s.category = detectCategoryFromPath(s.relativePath.isNotEmpty() ? s.relativePath : file.getFileName());

    // Leer metadatos reales del header usando AudioFormatReader
    if (std::unique_ptr<juce::AudioFormatReader> reader { formatManager.createReaderFor(file) })
    {
        s.sampleRate = reader->sampleRate;
        s.numChannels = static_cast<int>(reader->numChannels);
        s.bitsPerSample = static_cast<int>(reader->bitsPerSample);
        if (s.sampleRate > 0.0)
            s.lengthInSeconds = static_cast<double>(reader->lengthInSamples) / s.sampleRate;
    }

    return s;
}

void LibraryScanner::processFolder(const LibraryFolder& folder)
{
    if (!folder.directory.isDirectory())
        return;

    catalog.setFolderScanning(folder.id, true);

    std::vector<juce::File> foundFiles;

    // Primero recorremos para encontrar los archivos compatibles
    juce::RangedDirectoryIterator iter(folder.directory, true, "*", juce::File::findFiles);
    for (const auto& entry : iter)
    {
        if (threadShouldExit())
        {
            catalog.setFolderScanning(folder.id, false);
            return;
        }

        const auto file = entry.getFile();
        if (isAudioFile(file))
            foundFiles.push_back(file);
    }

    const int total = static_cast<int>(foundFiles.size());
    std::vector<Sample> parsedSamples;
    parsedSamples.reserve(total);

    for (int i = 0; i < total; ++i)
    {
        if (threadShouldExit())
        {
            catalog.setFolderScanning(folder.id, false);
            return;
        }

        parsedSamples.push_back(createSampleFromFile(foundFiles[static_cast<size_t>(i)], folder.directory, folder.name));

        // Notificar progreso periódico cada 10 archivos o al final
        if (i % 10 == 0 || i == total - 1)
        {
            const float progress = total > 0 ? static_cast<float>(i + 1) / static_cast<float>(total) : 1.0f;
            catalog.notifyScanProgress(folder.name, progress, i + 1);
        }
    }

    // Actualizar catálogo en memoria
    catalog.addOrUpdateSamples(parsedSamples, folder.name);
    catalog.setFolderScanning(folder.id, false);
    catalog.notifyScanCompleted(folder.name, total);
}

} // namespace drummatch

