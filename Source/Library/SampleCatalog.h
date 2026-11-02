#pragma once

#include <vector>
#include <optional>
#include <juce_core/juce_core.h>
#include "../Domain/Sample.h"
#include "../Domain/LibraryFolder.h"
#include "../Persistence/Database.h"

namespace drummatch
{

class SampleCatalog
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void librariesChanged() {}
        virtual void samplesUpdated() {}
        virtual void referenceSampleChanged(const std::optional<Sample>& ref) {}
        virtual void scanProgress(const juce::String& folderName, float progress, int foundCount) {}
        virtual void scanCompleted(const juce::String& folderName, int totalFound) {}
    };

    explicit SampleCatalog(Database& database);
    ~SampleCatalog() = default;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Carga inicial desde SQLite
    void loadFromDatabase();

    // Gestión de Carpetas
    bool addLibraryFolder(const juce::File& folder);
    void removeLibraryFolder(const juce::String& folderId);
    void setFolderEnabled(const juce::String& folderId, bool enabled);
    void setFolderScanning(const juce::String& folderId, bool isScanning);
    std::vector<LibraryFolder> getLibraryFolders() const;

    // Gestión de Samples
    void addOrUpdateSamples(const std::vector<Sample>& newSamples, const juce::String& libraryId);
    void clearSamplesForLibrary(const juce::String& libraryId);
    std::vector<Sample> getSamples(bool onlyFromEnabledLibraries = true, const juce::String& searchFilter = {}) const;
    std::optional<Sample> findSampleById(const juce::String& sampleId) const;
    int getTotalSampleCount() const;

    // Sample de Referencia
    void setReferenceSample(const Sample& sample);
    void clearReferenceSample();
    std::optional<Sample> getReferenceSample() const;
    bool isReferenceSample(const juce::String& sampleId) const;

    // Notificaciones de escaneo
    void notifyScanProgress(const juce::String& folderName, float progress, int foundCount);
    void notifyScanCompleted(const juce::String& folderName, int totalFound);

private:
    Database& db;
    mutable juce::CriticalSection cs;
    std::vector<LibraryFolder> folders;
    std::vector<Sample> samples;
    std::optional<Sample> referenceSample;
    juce::ListenerList<Listener> listeners;

    void notifyLibrariesChanged();
    void notifySamplesUpdated();
    void notifyReferenceSampleChanged();
};

} // namespace drummatch
