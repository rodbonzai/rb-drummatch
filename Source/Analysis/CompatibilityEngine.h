#pragma once

#include <juce_core/juce_core.h>
#include "../Domain/Sample.h"
#include "../Domain/AcousticFeatures.h"
#include "../Domain/MatchCandidate.h"

namespace drummatch
{

class CompatibilityEngine
{
public:
    CompatibilityEngine() = default;
    ~CompatibilityEngine() = default;

    // Evalúa la complementariedad acústica entre el sample de referencia y un candidato
    float evaluate(const Sample& refSample, const AcousticFeatures& refFeat,
                   const Sample& candSample, const AcousticFeatures& candFeat,
                   juce::String& outReason) const;

private:
    float evaluateKickAndSnare(const AcousticFeatures& kick, const AcousticFeatures& snare, juce::String& reason) const;
    float evaluateKickAndHat(const AcousticFeatures& kick, const AcousticFeatures& hat, bool isOpenHat, juce::String& reason) const;
    float evaluateSnareAndHat(const AcousticFeatures& snare, const AcousticFeatures& hat, juce::String& reason) const;
    float evaluateKickAndTom(const AcousticFeatures& kick, const AcousticFeatures& tom, InstrumentCategory tomCat, juce::String& reason) const;
    float evaluateDrumAndPerc(const AcousticFeatures& drum, const AcousticFeatures& perc, juce::String& reason) const;
    float evaluateGeneralComplementarity(const AcousticFeatures& ref, const AcousticFeatures& cand, juce::String& reason) const;
};

} // namespace drummatch

