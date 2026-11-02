#include "SampleCatalog.h"

namespace drummatch
{

SampleCatalog::SampleCatalog(Database& database)
    : db(database)
{
}

void SampleCatalog::addListener(Listener* listener)
{
    listeners.add(listener);
}

void SampleCatalog::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void SampleCatalog::loadFromDatabase()
{
    {
        const juce::ScopedLock sl(cs);
        folders = db.loadLibraries();
        samples = db.loadAllSamples();

        // Re-evaluar y corregir categorías con la heurística avanzada basada en nombres
        std::vector<std::pair<juce::String, InstrumentCategory>> updates;
        for (auto& s : samples)
        {
            const auto detected = detectCategoryFromPath(s.file.getFullPathName().isNotEmpty() ? s.file.getFullPathName() : s.relativePath);
            if (s.category != detected)
            {
                s.category = detected;
                updates.emplace_back(s.id, detected);
            }
        }

        if (!updates.empty())
        {
            db.updateSampleCategoriesBatch(updates);
        }

        const auto refId = db.getAppState("reference_sample_id");
        if (refId.isNotEmpty())
        {
            for (auto& s : samples)
            {
                if (s.id == refId)
                {
                    referenceSample = s;
                    referenceSample->isReference = true;
                    break;
                }
            }
        }
    }

    notifyLibrariesChanged();
    notifySamplesUpdated();
    notifyReferenceSampleChanged();
}

bool SampleCatalog::addLibraryFolder(const juce::File& folder)
{
    if (!folder.isDirectory())
        return false;

    const auto absPath = folder.getFullPathName();
    const auto folderId = juce::String::toHexString(absPath.hashCode64());

    LibraryFolder lf;
    lf.id = folderId;
    lf.directory = folder;
    lf.name = folder.getFileName();
    lf.isEnabled = true;
    lf.sampleCount = 0;
    lf.isScanning = false;

    {
        const juce::ScopedLock sl(cs);
        for (const auto& f : folders)
        {
            if (f.directory == folder || f.id == folderId)
                return false; // Ya existe
        }
        folders.push_back(lf);
    }

    db.saveLibrary(lf);

    notifyLibrariesChanged();
    return true;
}

void SampleCatalog::removeLibraryFolder(const juce::String& folderId)
{
    {
        const juce::ScopedLock sl(cs);
        folders.erase(std::remove_if(folders.begin(), folders.end(),
                                     [&](const LibraryFolder& f) { return f.id == folderId; }),
                      folders.end());

        samples.erase(std::remove_if(samples.begin(), samples.end(),
                                     [&](const Sample& s) { return s.libraryName == folderId; }),
                      samples.end());

        if (referenceSample.has_value() && referenceSample->libraryName == folderId)
        {
            referenceSample.reset();
            db.setAppState("reference_sample_id", "");
        }
    }

    db.deleteLibrary(folderId);

    notifyLibrariesChanged();
    notifySamplesUpdated();
    notifyReferenceSampleChanged();
}

void SampleCatalog::setFolderEnabled(const juce::String& folderId, bool enabled)
{
    {
        const juce::ScopedLock sl(cs);
        for (auto& f : folders)
        {
            if (f.id == folderId)
            {
                f.isEnabled = enabled;
                break;
            }
        }
    }

    db.updateLibraryEnabled(folderId, enabled);

    notifyLibrariesChanged();
    notifySamplesUpdated();
}

void SampleCatalog::setFolderScanning(const juce::String& folderId, bool isScanning)
{
    {
        const juce::ScopedLock sl(cs);
        for (auto& f : folders)
        {
            if (f.id == folderId)
            {
                f.isScanning = isScanning;
                break;
            }
        }
    }

    notifyLibrariesChanged();
}

std::vector<LibraryFolder> SampleCatalog::getLibraryFolders() const
{
    const juce::ScopedLock sl(cs);
    return folders;
}

void SampleCatalog::addOrUpdateSamples(const std::vector<Sample>& newSamples, const juce::String& libraryId)
{
    {
        const juce::ScopedLock sl(cs);

        samples.erase(std::remove_if(samples.begin(), samples.end(),
                                     [&](const Sample& s) { return s.libraryName == libraryId; }),
                      samples.end());

        samples.insert(samples.end(), newSamples.begin(), newSamples.end());

        for (auto& f : folders)
        {
            if (f.name == libraryId || f.id == libraryId)
            {
                f.sampleCount = static_cast<int>(newSamples.size());
                f.lastScanned = juce::Time::getCurrentTime();
                f.isScanning = false;
                db.updateLibraryCount(f.id, f.sampleCount, f.lastScanned);
                break;
            }
        }
    }

    // Persistir lote de samples en SQLite dentro de transacción
    db.saveSamplesBatch(libraryId, newSamples);

    notifyLibrariesChanged();
    notifySamplesUpdated();
}

void SampleCatalog::clearSamplesForLibrary(const juce::String& libraryId)
{
    {
        const juce::ScopedLock sl(cs);
        samples.erase(std::remove_if(samples.begin(), samples.end(),
                                     [&](const Sample& s) { return s.libraryName == libraryId; }),
                      samples.end());

        for (auto& f : folders)
        {
            if (f.name == libraryId || f.id == libraryId)
            {
                f.sampleCount = 0;
                db.updateLibraryCount(f.id, 0, juce::Time::getCurrentTime());
                break;
            }
        }
    }

    db.deleteSamplesForLibrary(libraryId);

    notifyLibrariesChanged();
    notifySamplesUpdated();
}

std::vector<Sample> SampleCatalog::getSamples(bool onlyFromEnabledLibraries, const juce::String& searchFilter) const
{
    const juce::ScopedLock sl(cs);
    std::vector<Sample> result;
    result.reserve(samples.size());

    const auto filterLower = searchFilter.trim().toLowerCase();

    for (const auto& s : samples)
    {
        if (onlyFromEnabledLibraries)
        {
            bool folderEnabled = false;
            for (const auto& f : folders)
            {
                if ((f.name == s.libraryName || f.id == s.libraryName) && f.isEnabled)
                {
                    folderEnabled = true;
                    break;
                }
            }
            if (!folderEnabled)
                continue;
        }

        if (filterLower.isNotEmpty())
        {
            const auto fileNameLower = s.getFileName().toLowerCase();
            const auto categoryLower = getCategoryName(s.category).toLowerCase();
            const auto libLower = s.libraryName.toLowerCase();

            if (!fileNameLower.contains(filterLower) &&
                !categoryLower.contains(filterLower) &&
                !libLower.contains(filterLower))
            {
                continue;
            }
        }

        Sample copy = s;
        if (referenceSample.has_value() && referenceSample->id == s.id)
            copy.isReference = true;

        result.push_back(copy);
    }

    std::sort(result.begin(), result.end(), [](const Sample& a, const Sample& b)
    {
        return a.getFileName().compareNatural(b.getFileName()) < 0;
    });

    return result;
}

std::optional<Sample> SampleCatalog::findSampleById(const juce::String& sampleId) const
{
    const juce::ScopedLock sl(cs);
    for (const auto& s : samples)
    {
        if (s.id == sampleId)
            return s;
    }
    return std::nullopt;
}

int SampleCatalog::getTotalSampleCount() const
{
    const juce::ScopedLock sl(cs);
    return static_cast<int>(samples.size());
}

void SampleCatalog::setReferenceSample(const Sample& sample)
{
    {
        const juce::ScopedLock sl(cs);
        referenceSample = sample;
        referenceSample->isReference = true;
    }

    db.setAppState("reference_sample_id", sample.id);

    notifyReferenceSampleChanged();
    notifySamplesUpdated();
}

void SampleCatalog::clearReferenceSample()
{
    {
        const juce::ScopedLock sl(cs);
        referenceSample.reset();
    }

    db.setAppState("reference_sample_id", "");

    notifyReferenceSampleChanged();
    notifySamplesUpdated();
}

std::optional<Sample> SampleCatalog::getReferenceSample() const
{
    const juce::ScopedLock sl(cs);
    return referenceSample;
}

bool SampleCatalog::isReferenceSample(const juce::String& sampleId) const
{
    const juce::ScopedLock sl(cs);
    return referenceSample.has_value() && referenceSample->id == sampleId;
}

void SampleCatalog::notifyScanProgress(const juce::String& folderName, float progress, int foundCount)
{
    juce::MessageManager::callAsync([this, folderName, progress, foundCount]()
    {
        listeners.call([&](Listener& l) { l.scanProgress(folderName, progress, foundCount); });
    });
}

void SampleCatalog::notifyScanCompleted(const juce::String& folderName, int totalFound)
{
    juce::MessageManager::callAsync([this, folderName, totalFound]()
    {
        listeners.call([&](Listener& l) { l.scanCompleted(folderName, totalFound); });
    });
}

void SampleCatalog::notifyLibrariesChanged()
{
    juce::MessageManager::callAsync([this]()
    {
        listeners.call([](Listener& l) { l.librariesChanged(); });
    });
}

void SampleCatalog::notifySamplesUpdated()
{
    juce::MessageManager::callAsync([this]()
    {
        listeners.call([](Listener& l) { l.samplesUpdated(); });
    });
}

void SampleCatalog::notifyReferenceSampleChanged()
{
    juce::MessageManager::callAsync([this]()
    {
        const auto ref = getReferenceSample();
        listeners.call([&](Listener& l) { l.referenceSampleChanged(ref); });
    });
}

} // namespace drummatch
