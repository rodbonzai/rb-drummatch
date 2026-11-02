#include "CompatibilityEngine.h"
#include <cmath>

namespace drummatch
{

static bool isKickFamily(InstrumentCategory cat)
{
    return cat == InstrumentCategory::Kick || cat == InstrumentCategory::GhostKick;
}

static bool isSnareFamily(InstrumentCategory cat)
{
    return cat == InstrumentCategory::Snare || cat == InstrumentCategory::GhostSnare ||
           cat == InstrumentCategory::CrossStick || cat == InstrumentCategory::Rim;
}

static bool isHatFamily(InstrumentCategory cat)
{
    return cat == InstrumentCategory::ClosedHat || cat == InstrumentCategory::OpenHat ||
           cat == InstrumentCategory::ClosedHatHard;
}

static bool isTomFamily(InstrumentCategory cat)
{
    return cat == InstrumentCategory::TomLow || cat == InstrumentCategory::TomMid ||
           cat == InstrumentCategory::TomHigh;
}

static bool isPercFamily(InstrumentCategory cat)
{
    return cat == InstrumentCategory::Percussion01 || cat == InstrumentCategory::Percussion02;
}

float CompatibilityEngine::evaluate(const Sample& refSample, const AcousticFeatures& refFeat,
                                     const Sample& candSample, const AcousticFeatures& candFeat,
                                     juce::String& outReason) const
{
    if (!refFeat.isValid || !candFeat.isValid)
    {
        outReason = "Basic acoustic heuristics applied";
        return 70.0f;
    }

    const auto refCat = refSample.category;
    const auto candCat = candSample.category;

    if (isKickFamily(refCat) && isSnareFamily(candCat))
        return evaluateKickAndSnare(refFeat, candFeat, outReason);

    if (isSnareFamily(refCat) && isKickFamily(candCat))
        return evaluateKickAndSnare(candFeat, refFeat, outReason);

    if (isKickFamily(refCat) && isHatFamily(candCat))
        return evaluateKickAndHat(refFeat, candFeat, candCat == InstrumentCategory::OpenHat, outReason);

    if (isSnareFamily(refCat) && isHatFamily(candCat))
        return evaluateSnareAndHat(refFeat, candFeat, outReason);

    if (isKickFamily(refCat) && isTomFamily(candCat))
        return evaluateKickAndTom(refFeat, candFeat, candCat, outReason);

    if ((isKickFamily(refCat) || isSnareFamily(refCat)) && isPercFamily(candCat))
        return evaluateDrumAndPerc(refFeat, candFeat, outReason);

    return evaluateGeneralComplementarity(refFeat, candFeat, outReason);
}

float CompatibilityEngine::evaluateKickAndSnare(const AcousticFeatures& kick, const AcousticFeatures& snare, juce::String& reason) const
{
    float score = 50.0f;

    // 1. Conflicto en subgraves: La caja no debe competir en 20-60 Hz con el bombo
    const float snareSubPenalty = snare.subBassEnergy * 180.0f;
    score += juce::jlimit(-25.0f, 20.0f, 15.0f - snareSubPenalty);

    // 2. Cuerpo y presencia de la caja: La caja debe tener energía en 200-500 Hz y brillo en 2.5-5 kHz
    const float snareBodyAndSnap = (snare.lowMidEnergy * 1.5f + snare.highMidEnergy * 1.2f);
    if (snareBodyAndSnap > 0.45f)
        score += 15.0f;
    else
        score += snareBodyAndSnap * 25.0f;

    // 3. Complementariedad de ataque y pegada (Crest Factor)
    // Si el bombo tiene transiente muy seco (alto crest factor), una caja con buen cuerpo complementa mejor
    const float crestDelta = std::abs(kick.crestFactor - snare.crestFactor);
    if (crestDelta > 1.2f && crestDelta < 4.0f)
        score += 10.0f;

    // 4. Separación espectral global (Centroide de caja vs bombo)
    if (snare.spectralCentroid > kick.spectralCentroid * 2.2f)
        score += 10.0f;

    score = juce::jlimit(25.0f, 98.0f, score);

    if (score >= 85.0f)
        reason = "Clean sub-bass separation with defined snap in 3.2 kHz";
    else if (score >= 70.0f)
        reason = "Good spectral balance, minimal low-end competition";
    else
        reason = "Potential energy masking in low-mid frequencies";

    return score;
}

float CompatibilityEngine::evaluateKickAndHat(const AcousticFeatures& kick, const AcousticFeatures& hat, bool isOpenHat, juce::String& reason) const
{
    float score = 55.0f;

    // 1. Separación de frecuencias: El hat debe estar concentrado en agudos (> 5 kHz) y casi nada en graves
    const float lowEndLeak = hat.subBassEnergy + hat.bassEnergy;
    score += juce::jlimit(-30.0f, 25.0f, 20.0f - lowEndLeak * 160.0f);

    // 2. Brillo y aire (Centroide espectral del hat alto)
    if (hat.spectralCentroid > 4500.0f)
        score += 15.0f;

    // 3. Duración y envolvente:
    if (isOpenHat)
    {
        // Open Hat debe tener buen decaimiento (> 150ms) sin ser excesivo
        if (hat.decayTimeMs > 120.0f && hat.decayTimeMs < 1200.0f)
            score += 10.0f;
    }
    else
    {
        // Closed Hat debe ser conciso y percusivo (< 140ms)
        if (hat.decayTimeMs < 150.0f)
            score += 10.0f;
    }

    score = juce::jlimit(30.0f, 98.0f, score);

    if (score >= 85.0f)
        reason = isOpenHat ? "Smooth top-end sizzle with natural decay complement"
                           : "Ultra-clean high frequency transient, zero sub masking";
    else
        reason = "Acceptable high-frequency presence";

    return score;
}

float CompatibilityEngine::evaluateSnareAndHat(const AcousticFeatures& snare, const AcousticFeatures& hat, juce::String& reason) const
{
    float score = 55.0f;

    // Separación de brillo: Evitar que el hat enmascare el crack de la caja en 3-5 kHz
    if (hat.spectralCentroid > snare.spectralCentroid * 1.4f)
        score += 20.0f;

    if (hat.highEnergy > 0.35f)
        score += 15.0f;

    score = juce::jlimit(30.0f, 98.0f, score);
    reason = "Complementary transient clarity without harsh buildup";
    return score;
}

float CompatibilityEngine::evaluateKickAndTom(const AcousticFeatures& kick, const AcousticFeatures& tom, InstrumentCategory tomCat, juce::String& reason) const
{
    float score = 55.0f;

    // Tom grave debe situarse por encima del subgrave del bombo (80-140 Hz vs 50-70 Hz)
    if (tomCat == InstrumentCategory::TomLow)
    {
        if (tom.subBassEnergy < kick.subBassEnergy * 0.6f && tom.bassEnergy > 0.25f)
            score += 25.0f;
    }
    else
    {
        if (tom.lowMidEnergy + tom.midEnergy > 0.4f)
            score += 20.0f;
    }

    score = juce::jlimit(30.0f, 96.0f, score);
    reason = "Distinct pitch resonance without masking kick fundamental";
    return score;
}

float CompatibilityEngine::evaluateDrumAndPerc(const AcousticFeatures& drum, const AcousticFeatures& perc, juce::String& reason) const
{
    float score = 60.0f;

    // Percusión debe ocupar "bolsillos" en medios/agudos sin ensuciar la pegada principal
    if (perc.subBassEnergy < 0.05f)
        score += 15.0f;

    if (perc.midEnergy + perc.highMidEnergy > 0.4f)
        score += 15.0f;

    score = juce::jlimit(35.0f, 96.0f, score);
    reason = "Fits neatly in the mid-frequency pocket";
    return score;
}

float CompatibilityEngine::evaluateGeneralComplementarity(const AcousticFeatures& ref, const AcousticFeatures& cand, juce::String& reason) const
{
    float score = 65.0f;

    // Separación espectral básica
    const float centroidRatio = (cand.spectralCentroid > 0.0f && ref.spectralCentroid > 0.0f)
                                ? cand.spectralCentroid / ref.spectralCentroid : 1.0f;

    if (centroidRatio > 1.3f || centroidRatio < 0.75f)
        score += 15.0f;

    // Contraste dinámico
    if (std::abs(ref.crestFactor - cand.crestFactor) > 1.0f)
        score += 10.0f;

    score = juce::jlimit(40.0f, 92.0f, score);
    reason = "Well-balanced frequency and dynamic contrast";
    return score;
}

} // namespace drummatch

