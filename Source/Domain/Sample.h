#pragma once

#include <juce_core/juce_core.h>
#include "InstrumentCategory.h"

namespace drummatch
{

struct Sample
{
    juce::String id;
    juce::File file;
    juce::String relativePath;
    juce::String libraryName;
    juce::int64 fileSize = 0;
    juce::Time lastModified;
    double sampleRate = 0.0;
    int numChannels = 0;
    double lengthInSeconds = 0.0;
    int bitsPerSample = 16;
    juce::String format;
    InstrumentCategory category = InstrumentCategory::Other;
    bool isReference = false;

    juce::String getFileName() const
    {
        return file.getFileName();
    }

    juce::String getFormattedDuration() const
    {
        if (lengthInSeconds <= 0.0)
            return "-";
        return juce::String(lengthInSeconds, 2) + "s";
    }

    juce::String getFormattedSampleRate() const
    {
        if (sampleRate <= 0.0)
            return "-";
        return juce::String(sampleRate / 1000.0, 1) + " kHz";
    }

    juce::String getFormattedSize() const
    {
        return juce::File::descriptionOfSizeInBytes(fileSize);
    }
};

} // namespace drummatch

