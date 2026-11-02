#pragma once

#include <vector>
#include <map>
#include <set>
#include <memory>
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "FeatureExtractor.h"
#include "CompatibilityEngine.h"
#include "../Domain/Sample.h"
#include "../Domain/MatchCandidate.h"
#include "../Domain/PadDefinition.h"
#include "../Persistence/Database.h"

namespace drummatch
{

class MatchRanking : private juce::Thread
{
public:
    MatchRanking(Database& database, juce::AudioFormatManager& formatManager);
    ~MatchRanking() override;

    // Inicia el cálculo de compatibilidad asíncrono con soporte de variedad y exclusividad
    void rankCandidatesAsync(const Sample& referenceSample,
                             const std::vector<Sample>& allCandidates,
                             const std::vector<PadDefinition>& padLayout,
                             int varietyOffset = 0);

    // Navegación de alternativas por pad (Fase 5)
    std::optional<MatchCandidate> getCandidateAt(int padIndex, int candidateIndex) const;
    int getTotalCandidatesForPad(int padIndex) const;

    std::function<void(const std::vector<MatchCandidate>& bestMatches)> onRankingCompleted;
    std::function<void(float progress)> onRankingProgress;

private:
    void run() override;

    AcousticFeatures getOrExtractFeatures(const Sample& sample);
    std::vector<Sample> getCandidatePoolForCategory(InstrumentCategory padCat) const;

    Database& db;
    juce::AudioFormatManager& formatMgr;
    FeatureExtractor extractor;
    CompatibilityEngine compatibility;

    Sample currentReference;
    std::vector<Sample> candidatePool;
    std::vector<PadDefinition> layout;
    int currentVarietyOffset = 0;

    mutable juce::CriticalSection resultsLock;
    std::map<int, std::vector<MatchCandidate>> rankedResultsByPad;
    std::map<juce::String, AcousticFeatures> featureCache;
};

} // namespace drummatch
