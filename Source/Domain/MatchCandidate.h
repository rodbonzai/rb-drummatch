#pragma once

#include "Sample.h"
#include "InstrumentCategory.h"

namespace drummatch
{

struct MatchCandidate
{
    Sample sample;
    float score = 0.0f; // 0.0 - 100.0
    juce::String reason;
    InstrumentCategory category = InstrumentCategory::Other;
    int padIndex = -1;

    juce::String getFormattedScore() const
    {
        return juce::String(juce::roundToInt(score)) + "%";
    }
};

} // namespace drummatch

