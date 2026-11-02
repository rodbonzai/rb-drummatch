#include "MatchRanking.h"

namespace drummatch
{

MatchRanking::MatchRanking(Database& database, juce::AudioFormatManager& formatManager)
    : juce::Thread("DrumMatchRankingThread"),
      db(database),
      formatMgr(formatManager)
{
}

MatchRanking::~MatchRanking()
{
    stopThread(3000);
}

void MatchRanking::rankCandidatesAsync(const Sample& referenceSample,
                                       const std::vector<Sample>& allCandidates,
                                       const std::vector<PadDefinition>& padLayout,
                                       int varietyOffset)
{
    stopThread(2000);

    currentReference = referenceSample;
    candidatePool = allCandidates;
    layout = padLayout;
    currentVarietyOffset = varietyOffset;

    startThread(juce::Thread::Priority::normal);
}

AcousticFeatures MatchRanking::getOrExtractFeatures(const Sample& sample)
{
    auto it = featureCache.find(sample.id);
    if (it != featureCache.end())
        return it->second;

    const auto feat = extractor.extract(sample.file, formatMgr);
    featureCache[sample.id] = feat;
    return feat;
}

std::vector<Sample> MatchRanking::getCandidatePoolForCategory(InstrumentCategory padCat) const
{
    std::vector<Sample> pool;

    // 1. Coincidencias exactas
    for (const auto& s : candidatePool)
    {
        if (s.category == padCat)
            pool.push_back(s);
    }

    // 2. Fallbacks inteligentes según familias de instrumentos si no hay suficientes
    if (pool.size() < 3)
    {
        for (const auto& s : candidatePool)
        {
            if (s.category == padCat)
                continue;

            bool isFallbackMatch = false;

            switch (padCat)
            {
                case InstrumentCategory::ClosedHatHard:
                case InstrumentCategory::ClosedHat:
                    isFallbackMatch = (s.category == InstrumentCategory::ClosedHat ||
                                       s.category == InstrumentCategory::ClosedHatHard ||
                                       s.category == InstrumentCategory::OpenHat);
                    break;

                case InstrumentCategory::OpenHat:
                    isFallbackMatch = (s.category == InstrumentCategory::ClosedHat ||
                                       s.category == InstrumentCategory::ClosedHatHard);
                    break;

                case InstrumentCategory::GhostKick:
                    isFallbackMatch = (s.category == InstrumentCategory::Kick);
                    break;

                case InstrumentCategory::GhostSnare:
                case InstrumentCategory::CrossStick:
                case InstrumentCategory::Rim:
                    isFallbackMatch = (s.category == InstrumentCategory::Snare ||
                                       s.category == InstrumentCategory::GhostSnare ||
                                       s.category == InstrumentCategory::CrossStick ||
                                       s.category == InstrumentCategory::Rim ||
                                       s.category == InstrumentCategory::Clap ||
                                       s.category == InstrumentCategory::Percussion01);
                    break;

                case InstrumentCategory::Snare:
                    isFallbackMatch = (s.category == InstrumentCategory::GhostSnare ||
                                       s.category == InstrumentCategory::Clap ||
                                       s.category == InstrumentCategory::CrossStick ||
                                       s.category == InstrumentCategory::Rim);
                    break;

                case InstrumentCategory::Clap:
                    isFallbackMatch = (s.category == InstrumentCategory::Snare ||
                                       s.category == InstrumentCategory::GhostSnare ||
                                       s.category == InstrumentCategory::CrossStick);
                    break;

                case InstrumentCategory::TomLow:
                case InstrumentCategory::TomMid:
                case InstrumentCategory::TomHigh:
                    isFallbackMatch = (s.category == InstrumentCategory::TomLow ||
                                       s.category == InstrumentCategory::TomMid ||
                                       s.category == InstrumentCategory::TomHigh ||
                                       s.category == InstrumentCategory::Percussion01);
                    break;

                case InstrumentCategory::Percussion01:
                case InstrumentCategory::Percussion02:
                    isFallbackMatch = (s.category == InstrumentCategory::Percussion01 ||
                                       s.category == InstrumentCategory::Percussion02 ||
                                       s.category == InstrumentCategory::Other);
                    break;

                case InstrumentCategory::CrashCymbal:
                case InstrumentCategory::RideCymbal:
                    isFallbackMatch = (s.category == InstrumentCategory::CrashCymbal ||
                                       s.category == InstrumentCategory::RideCymbal ||
                                       s.category == InstrumentCategory::OpenHat);
                    break;

                default:
                    break;
            }

            if (isFallbackMatch)
                pool.push_back(s);
        }
    }

    return pool;
}

void MatchRanking::run()
{
    if (!currentReference.file.existsAsFile())
        return;

    const auto refFeat = getOrExtractFeatures(currentReference);
    std::map<int, std::vector<MatchCandidate>> tempRankings;

    const int totalPads = static_cast<int>(layout.size());

    // Identificar de manera unívoca el pad asignado al sample de referencia activo
    int refPadIndex = -1;
    for (const auto& p : layout)
    {
        if (p.isReference || (p.assignedSample.has_value() && p.assignedSample->id == currentReference.id))
        {
            refPadIndex = p.index;
            break;
        }
    }

    if (refPadIndex == -1)
    {
        for (const auto& p : layout)
        {
            if (p.category == currentReference.category)
            {
                refPadIndex = p.index;
                break;
            }
        }
    }

    // Fase 1: Calcular compatibilidad para todos los candidatos de cada pad
    for (int padIdx = 0; padIdx < totalPads; ++padIdx)
    {
        if (threadShouldExit())
            return;

        const auto& pad = layout[static_cast<size_t>(padIdx)];
        std::vector<MatchCandidate> candidatesForPad;

        if (pad.index == refPadIndex)
        {
            MatchCandidate refMatch;
            refMatch.sample = currentReference;
            refMatch.score = 100.0f;
            refMatch.reason = "Active Reference Sample";
            refMatch.category = pad.category;
            refMatch.padIndex = pad.index;
            candidatesForPad.push_back(refMatch);
        }
        else
        {
            const auto pool = getCandidatePoolForCategory(pad.category);

            for (const auto& cand : pool)
            {
                if (threadShouldExit())
                    return;

                if (cand.id == currentReference.id)
                    continue;

                const auto candFeat = getOrExtractFeatures(cand);
                juce::String matchReason;
                const float score = compatibility.evaluate(currentReference, refFeat, cand, candFeat, matchReason);

                MatchCandidate m;
                m.sample = cand;
                m.score = score;
                m.reason = matchReason;
                m.category = pad.category;
                m.padIndex = pad.index;
                candidatesForPad.push_back(m);
            }

            // Ordenar de mayor a menor compatibilidad acústica
            std::sort(candidatesForPad.begin(), candidatesForPad.end(),
                      [](const MatchCandidate& a, const MatchCandidate& b) { return a.score > b.score; });
        }

        tempRankings[pad.index] = std::move(candidatesForPad);

        const float progress = static_cast<float>(padIdx + 1) / static_cast<float>(totalPads);
        juce::MessageManager::callAsync([this, progress]()
        {
            if (onRankingProgress)
                onRankingProgress(progress);
        });
    }

    // Fase 2: Resolver el Kit Completo garantizando EXCLUSIVIDAD (sin duplicados en el kit)
    std::set<juce::String> usedSampleIds;
    usedSampleIds.insert(currentReference.id);

    std::vector<MatchCandidate> bestMatches;
    bestMatches.reserve(totalPads);

    for (int padIdx = 0; padIdx < totalPads; ++padIdx)
    {
        const auto& pad = layout[static_cast<size_t>(padIdx)];
        auto& list = tempRankings[pad.index];

        if (pad.index == refPadIndex)
        {
            MatchCandidate refMatch;
            refMatch.sample = currentReference;
            refMatch.score = 100.0f;
            refMatch.reason = "Active Reference Sample";
            refMatch.category = pad.category;
            refMatch.padIndex = pad.index;
            bestMatches.push_back(refMatch);
            continue;
        }

        // Filtrar candidatos que aún no han sido usados en este kit
        std::vector<MatchCandidate> unusedCandidates;
        for (const auto& cand : list)
        {
            if (usedSampleIds.find(cand.sample.id) == usedSampleIds.end())
            {
                unusedCandidates.push_back(cand);
            }
        }

        if (!unusedCandidates.empty())
        {
            // Aplicar variedad: si el usuario pide variar, tomamos del top-N de candidatos no usados
            const int pickIndex = (currentVarietyOffset > 0) ?
                                  (currentVarietyOffset % static_cast<int>(unusedCandidates.size())) : 0;

            const auto chosen = unusedCandidates[static_cast<size_t>(pickIndex)];
            usedSampleIds.insert(chosen.sample.id);
            bestMatches.push_back(chosen);
        }
        else if (!list.empty())
        {
            // Si no quedan candidatos no usados, usamos el mejor disponible de su categoría
            bestMatches.push_back(list.front());
        }
    }

    {
        const juce::ScopedLock sl(resultsLock);
        rankedResultsByPad = std::move(tempRankings);
    }

    juce::MessageManager::callAsync([this, bestMatches]()
    {
        if (onRankingCompleted)
            onRankingCompleted(bestMatches);
    });
}

std::optional<MatchCandidate> MatchRanking::getCandidateAt(int padIndex, int candidateIndex) const
{
    const juce::ScopedLock sl(resultsLock);
    auto it = rankedResultsByPad.find(padIndex);
    if (it != rankedResultsByPad.end())
    {
        if (candidateIndex >= 0 && candidateIndex < static_cast<int>(it->second.size()))
            return it->second[static_cast<size_t>(candidateIndex)];
    }
    return std::nullopt;
}

int MatchRanking::getTotalCandidatesForPad(int padIndex) const
{
    const juce::ScopedLock sl(resultsLock);
    auto it = rankedResultsByPad.find(padIndex);
    if (it != rankedResultsByPad.end())
        return static_cast<int>(it->second.size());
    return 0;
}

} // namespace drummatch
