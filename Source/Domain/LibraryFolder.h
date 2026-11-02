#pragma once

#include <juce_core/juce_core.h>

namespace drummatch
{

struct LibraryFolder
{
    juce::String id;
    juce::File directory;
    juce::String name;
    bool isEnabled = true;
    int sampleCount = 0;
    bool isScanning = false;
    juce::Time lastScanned;

    juce::String getPath() const
    {
        return directory.getFullPathName();
    }
};

} // namespace drummatch

