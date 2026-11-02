#pragma once

#include <vector>
#include <optional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "InstrumentCategory.h"
#include "Sample.h"

namespace drummatch
{

struct PadDefinition
{
    int index = 0;              // 0..15
    juce::String padId;         // "A01".."A16"
    juce::String label;         // e.g. "KICK"
    InstrumentCategory category = InstrumentCategory::Other;
    juce::Colour color = juce::Colours::grey;
    std::optional<Sample> assignedSample;
    float matchScore = 0.0f;          // 0.0 - 100.0 (Compatibilidad acústica)
    juce::String matchReason;         // Explicación de la complementariedad
    int alternativeIndex = 0;         // Índice de candidato alternativo
    int totalAlternatives = 0;        // Total de candidatos encontrados en esa categoría
    bool isReference = false;         // Si este pad contiene el sample de referencia
    juce::String keyShortcut;         // Tecla rápida de teclado (ej: "Z", "X", etc.)

    // Helper para generar el layout por defecto según drummatch.md Sección 5
    static std::vector<PadDefinition> getDefaultLayout()
    {
        std::vector<PadDefinition> pads;
        pads.reserve(16);

        // Fila 4 (Bottom en MPC: A01 - A04) -> Z, X, C, V
        // Fila 3 (A05 - A08)                 -> A, S, D, F
        // Fila 2 (A09 - A12)                 -> Q, W, E, R
        // Fila 1 (Top en MPC: A13 - A16)     -> 1, 2, 3, 4
        struct Def { int idx; const char* id; const char* lbl; InstrumentCategory cat; const char* key; };
        const Def defs[] = {
            { 0,  "A01", "KICK",            InstrumentCategory::Kick,           "Z" },
            { 1,  "A02", "SNARE",           InstrumentCategory::Snare,          "X" },
            { 2,  "A03", "CLOSED HAT",      InstrumentCategory::ClosedHat,      "C" },
            { 3,  "A04", "OPEN HAT",        InstrumentCategory::OpenHat,        "V" },
            { 4,  "A05", "GHOST KICK",      InstrumentCategory::GhostKick,      "A" },
            { 5,  "A06", "GHOST SNARE",     InstrumentCategory::GhostSnare,     "S" },
            { 6,  "A07", "CROSS STICK",     InstrumentCategory::CrossStick,     "D" },
            { 7,  "A08", "CLOSED HAT HARD", InstrumentCategory::ClosedHatHard, "F" },
            { 8,  "A09", "CRASH CYMBAL",    InstrumentCategory::CrashCymbal,    "Q" },
            { 9,  "A10", "RIDE",            InstrumentCategory::RideCymbal,     "W" },
            { 10, "A11", "PERC 01",         InstrumentCategory::Percussion01,   "E" },
            { 11, "A12", "PERC 02",         InstrumentCategory::Percussion02,   "R" },
            { 12, "A13", "TOM LOW",         InstrumentCategory::TomLow,         "1" },
            { 13, "A14", "TOM MID",         InstrumentCategory::TomMid,         "2" },
            { 14, "A15", "TOM HIGH",        InstrumentCategory::TomHigh,        "3" },
            { 15, "A16", "RIM",             InstrumentCategory::Rim,            "4" },
        };

        for (const auto& d : defs)
        {
            PadDefinition pad;
            pad.index = d.idx;
            pad.padId = d.id;
            pad.label = d.lbl;
            pad.category = d.cat;
            pad.color = getCategoryColor(d.cat);
            pad.keyShortcut = d.key;
            pads.push_back(pad);
        }

        return pads;
    }
};

} // namespace drummatch

