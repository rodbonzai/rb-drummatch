#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace drummatch
{

enum class InstrumentCategory
{
    Kick,
    GhostKick,
    Snare,
    GhostSnare,
    CrossStick,
    Rim,
    ClosedHat,
    OpenHat,
    ClosedHatHard,
    CrashCymbal,
    RideCymbal,
    TomLow,
    TomMid,
    TomHigh,
    Percussion01,
    Percussion02,
    Other,
    Clap
};

inline juce::String getCategoryName(InstrumentCategory cat)
{
    switch (cat)
    {
        case InstrumentCategory::Kick:          return "KICK";
        case InstrumentCategory::GhostKick:     return "GHOST KICK";
        case InstrumentCategory::Snare:         return "SNARE";
        case InstrumentCategory::GhostSnare:    return "GHOST SNARE";
        case InstrumentCategory::CrossStick:    return "CROSS STICK";
        case InstrumentCategory::Rim:           return "RIM";
        case InstrumentCategory::ClosedHat:     return "CLOSED HAT";
        case InstrumentCategory::OpenHat:       return "OPEN HAT";
        case InstrumentCategory::ClosedHatHard: return "CLOSED HAT HARD";
        case InstrumentCategory::CrashCymbal:   return "CRASH CYMBAL";
        case InstrumentCategory::RideCymbal:    return "RIDE";
        case InstrumentCategory::TomLow:        return "TOM LOW";
        case InstrumentCategory::TomMid:        return "TOM MID";
        case InstrumentCategory::TomHigh:       return "TOM HIGH";
        case InstrumentCategory::Percussion01:  return "PERC 01";
        case InstrumentCategory::Percussion02:  return "PERC 02";
        case InstrumentCategory::Other:         return "OTHER";
        case InstrumentCategory::Clap:          return "CLAP";
    }
    return "UNKNOWN";
}

inline juce::Colour getCategoryColor(InstrumentCategory cat)
{
    // Colores inspirados en MPC Live 2 / drummatch.md Sección 5
    switch (cat)
    {
        case InstrumentCategory::Kick:
        case InstrumentCategory::GhostKick:
            return juce::Colour(0xffe63946); // Rojo

        case InstrumentCategory::Snare:
        case InstrumentCategory::GhostSnare:
        case InstrumentCategory::CrossStick:
        case InstrumentCategory::Rim:
        case InstrumentCategory::Clap:
            return juce::Colour(0xff3a86ff); // Azul

        case InstrumentCategory::ClosedHat:
        case InstrumentCategory::OpenHat:
        case InstrumentCategory::ClosedHatHard:
            return juce::Colour(0xff2ec4b6); // Verde / Teal

        case InstrumentCategory::CrashCymbal:
        case InstrumentCategory::RideCymbal:
        case InstrumentCategory::TomLow:
        case InstrumentCategory::TomMid:
        case InstrumentCategory::TomHigh:
            return juce::Colour(0xffffb703); // Amarillo / Ámbar

        case InstrumentCategory::Percussion01:
        case InstrumentCategory::Percussion02:
            return juce::Colour(0xff8338ec); // Morado

        case InstrumentCategory::Other:
        default:
            return juce::Colour(0xff6c757d); // Gris neutro
    }
}

// Comprueba si un token de 2 o más letras existe delimitado o como prefijo/sufijo numérico (ej: "kc01", "cr_02", "_sd_")
inline bool hasDrumToken(const juce::String& text, const juce::String& token)
{
    const auto lower = text.toLowerCase();
    const auto tok = token.toLowerCase();
    const int tokLen = tok.length();
    int searchPos = 0;

    while ((searchPos = lower.indexOf(searchPos, tok)) >= 0)
    {
        // Límite previo: inicio de texto o carácter no alfabético
        const bool prevOk = (searchPos == 0) || !juce::CharacterFunctions::isLetter(lower[searchPos - 1]);

        // Límite posterior: fin de texto o carácter no alfabético (un dígito o delimitador es válido)
        const int endPos = searchPos + tokLen;
        const bool nextOk = (endPos >= lower.length()) || !juce::CharacterFunctions::isLetter(lower[endPos]);

        if (prevOk && nextOk)
            return true;

        searchPos += tokLen;
    }

    return false;
}

// Clasificación directa por nombre o token
inline InstrumentCategory classifyName(const juce::String& name)
{
    const auto lower = name.toLowerCase();

    // 1. KICKS (bombo, kick, bd, kc, ft) - MÁXIMA PRIORIDAD
    const bool isKick = lower.contains("kick") || lower.contains("bombo") ||
                        hasDrumToken(lower, "bd") || hasDrumToken(lower, "kc") ||
                        (hasDrumToken(lower, "ft") && !lower.contains("tom") && !lower.contains("floor"));
    if (isKick)
    {
        if (lower.contains("ghost"))
            return InstrumentCategory::GhostKick;
        return InstrumentCategory::Kick;
    }

    // 2. SNARES (caja, snare, snr, sd, sn) - MÁXIMA PRIORIDAD
    const bool isSnare = lower.contains("snare") || lower.contains("caja") ||
                         lower.contains("snr") || hasDrumToken(lower, "sd") || hasDrumToken(lower, "sn");
    if (isSnare)
    {
        if (lower.contains("ghost"))
            return InstrumentCategory::GhostSnare;
        return InstrumentCategory::Snare;
    }

    // 3. CLAPS (clap, clp, cl)
    if (lower.contains("clap") || hasDrumToken(lower, "clp") || hasDrumToken(lower, "cl"))
        return InstrumentCategory::Clap;

    // 4. OPEN HATS (open, semiopen, ohat, ophat, oh) - antes de closed hat
    if (lower.contains("open") || lower.contains("semiopen") || lower.contains("ohat") ||
        lower.contains("ophat") || hasDrumToken(lower, "oh"))
        return InstrumentCategory::OpenHat;

    // 5. CLOSED HATS (hithat, hihat, hat, closed, hh, ch)
    if (lower.contains("hard") && (lower.contains("hat") || lower.contains("hh") || lower.contains("hihat") || lower.contains("hithat")))
        return InstrumentCategory::ClosedHatHard;

    if (lower.contains("closed") || lower.contains("hithat") || lower.contains("hihat") ||
        lower.contains("hat") || hasDrumToken(lower, "hh") || hasDrumToken(lower, "ch"))
        return InstrumentCategory::ClosedHat;

    // 6. RIDES (ride, rd) - antes de crash para que "RIDE_CR" no sea clasificado como crash
    if (lower.contains("ride") || hasDrumToken(lower, "rd"))
        return InstrumentCategory::RideCymbal;

    // 7. CRASH / CYMBALS (crash, crsh, cymbal, cym, cr)
    if (lower.contains("crash") || lower.contains("crsh") || lower.contains("cymbal") ||
        hasDrumToken(lower, "cym") || hasDrumToken(lower, "cr"))
        return InstrumentCategory::CrashCymbal;

    // 8. RIMS & STICKS (rim, rimshot, rm, cross, sidestick, xstick, stick)
    if (lower.contains("rim") || lower.contains("rimshot") || hasDrumToken(lower, "rm"))
        return InstrumentCategory::Rim;

    if (lower.contains("cross") || lower.contains("xstick") || lower.contains("sidestick") ||
        lower.contains("cstick") || lower.contains("x-stick") || lower.contains("side-stick") ||
        hasDrumToken(lower, "stick"))
        return InstrumentCategory::CrossStick;

    // 9. TOMS
    if (lower.contains("tom") && (lower.contains("low") || lower.contains("floor") || hasDrumToken(lower, "ltom") || hasDrumToken(lower, "ftom")))
        return InstrumentCategory::TomLow;

    if (lower.contains("tom") && (lower.contains("hi") || lower.contains("high") || hasDrumToken(lower, "htom")))
        return InstrumentCategory::TomHigh;

    if (lower.contains("tom") || hasDrumToken(lower, "mtom"))
        return InstrumentCategory::TomMid;

    // 10. PERCUSIÓN
    if (lower.contains("perc") || lower.contains("shaker") || lower.contains("conga") || lower.contains("bongo") ||
        lower.contains("timbale") || lower.contains("tamb") || lower.contains("guiro") || lower.contains("triangle") ||
        lower.contains("cowbell") || lower.contains("bell") || lower.contains("clave") || lower.contains("cabasa") ||
        lower.contains("woodblock") || lower.contains("chime") || lower.contains("maraca"))
        return InstrumentCategory::Percussion01;

    // 11. 808 / SUB
    if (lower.contains("808") || lower.contains("sub"))
        return InstrumentCategory::Kick;

    return InstrumentCategory::Other;
}

// Inferencia jerárquica: el nombre de archivo es la máxima autoridad; las carpetas son fallback
inline InstrumentCategory detectCategoryFromPath(const juce::String& pathOrName)
{
    const juce::File f(pathOrName);
    const juce::String fileNameOnly = f.getFileNameWithoutExtension();

    // 1. Evaluar PRIMERO el nombre del archivo (KICK_CR_1 es KICK, SNR_CR es SNARE, RIDE_CR es RIDE)
    const auto catFromFile = classifyName(fileNameOnly);
    if (catFromFile != InstrumentCategory::Other)
        return catFromFile;

    // 2. Si el archivo es neutro (ej: 01.wav), evaluar la carpeta contenedora
    const juce::String dirPath = f.getParentDirectory().getFullPathName();
    if (dirPath.isNotEmpty())
    {
        const auto catFromDir = classifyName(dirPath);
        if (catFromDir != InstrumentCategory::Other)
            return catFromDir;
    }

    // 3. Fallback: evaluar la cadena completa
    return classifyName(pathOrName);
}

} // namespace drummatch

