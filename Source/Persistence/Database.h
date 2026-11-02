#pragma once

#include <vector>
#include <optional>
#include <juce_core/juce_core.h>
#include "../Domain/Sample.h"
#include "../Domain/LibraryFolder.h"
#include "../Domain/PadDefinition.h"

struct sqlite3;

namespace drummatch
{

class Database
{
public:
    Database();
    ~Database();

    bool open(const juce::File& dbFile);
    void close();
    bool isOpen() const { return db != nullptr; }

    // Bibliotecas
    std::vector<LibraryFolder> loadLibraries();
    bool saveLibrary(const LibraryFolder& folder);
    bool updateLibraryEnabled(const juce::String& folderId, bool enabled);
    bool updateLibraryCount(const juce::String& folderId, int count, juce::Time lastScanned);
    bool deleteLibrary(const juce::String& folderId);

    // Samples
    std::vector<Sample> loadAllSamples();
    bool saveSamplesBatch(const juce::String& libraryId, const std::vector<Sample>& samples);
    bool deleteSamplesForLibrary(const juce::String& libraryId);
    bool updateSampleCategoriesBatch(const std::vector<std::pair<juce::String, InstrumentCategory>>& updates);

    // Pad Layouts (Fase 2)
    std::vector<PadDefinition> loadPadLayout(const juce::String& layoutId = "default");
    bool savePadLayout(const juce::String& layoutId, const std::vector<PadDefinition>& pads);

    // Estado general (Reference sample, layout activo, etc.)
    juce::String getAppState(const juce::String& key, const juce::String& defaultValue = {});
    void setAppState(const juce::String& key, const juce::String& value);

    static juce::File getDefaultDatabaseFile();

private:
    sqlite3* db = nullptr;
    mutable juce::CriticalSection dbLock;

    bool execute(const char* sql);
    void createTables();
};

} // namespace drummatch

