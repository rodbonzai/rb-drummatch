#pragma once

#include <juce_core/juce_core.h>

namespace drummatch
{

struct AcousticFeatures
{
    float rms = 0.0f;
    float peak = 0.0f;
    float crestFactor = 0.0f;

    // Descriptores espectrales
    float spectralCentroid = 0.0f; // Centro de masa espectral (Hz)
    float spectralRolloff = 0.0f;  // Frecuencia donde se concentra el 85% de la energía (Hz)

    // Energía normalizada por bandas de frecuencia (0.0 - 1.0)
    float subBassEnergy = 0.0f;    // 20 - 60 Hz (Subgrave)
    float bassEnergy = 0.0f;       // 60 - 250 Hz (Cuerpo/Punch)
    float lowMidEnergy = 0.0f;     // 250 - 500 Hz (Caja/Cálido)
    float midEnergy = 0.0f;        // 500 - 2000 Hz (Presencia media)
    float highMidEnergy = 0.0f;    // 2000 - 6000 Hz (Ataque/Snap)
    float highEnergy = 0.0f;       // 6000 - 20000 Hz (Aire/Brillo)

    // Descriptores temporales y transientes
    float attackTimeMs = 0.0f;
    float decayTimeMs = 0.0f;
    float transientStrength = 0.0f;

    bool isValid = false;
};

} // namespace drummatch

