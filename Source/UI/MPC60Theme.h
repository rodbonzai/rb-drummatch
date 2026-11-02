#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace drummatch
{

namespace theme
{
    // =========================================================================
    // Tokens de Color — AKAI MPC 60 (1988)
    // =========================================================================
    inline const juce::Colour bgExterior       { 0xff7E7C74 }; // Fondo exterior
    inline const juce::Colour chassis          { 0xffBAB7AA }; // Chasis beige-gris
    inline const juce::Colour panelStandard    { 0xffA3A69B }; // Placa gris verdosa
    inline const juce::Colour panelSecondary   { 0xffD3CFBC }; // Panel crema secundario
    inline const juce::Colour textMain         { 0xff1E1E1C }; // Texto principal oscuro
    inline const juce::Colour textSecondary    { 0xff34352F }; // Serigrafía técnica

    // Acento Primario — Teclas REC (Rojo)
    inline const juce::Colour redPrimary       { 0xffB52A2A };
    inline const juce::Colour redTop           { 0xffC73A38 };
    inline const juce::Colour redShadow        { 0xff6B1414 };
    inline const juce::Colour redText          { 0xffFFFFFF };

    // Botón Normal (Crema MPC)
    inline const juce::Colour btnCreamTop      { 0xffF3EDD6 };
    inline const juce::Colour btnCreamBottom   { 0xffDDD5B8 };
    inline const juce::Colour btnCreamBorder   { 0xff9C988B };
    inline const juce::Colour btnCreamShadow   { 0xff8C8675 };

    // Botón de Peligro / Secundario (Gris Oscuro Cursores MPC)
    inline const juce::Colour btnDarkTop       { 0xff4A4A48 };
    inline const juce::Colour btnDarkBottom    { 0xff2E2E2D };
    inline const juce::Colour btnDarkBorder    { 0xff222221 };
    inline const juce::Colour btnDarkShadow    { 0xff161615 };
    inline const juce::Colour btnDarkText      { 0xffFFFFFF };

    // Pantalla LCD (Verde retro)
    inline const juce::Colour lcdBg            { 0xff5FE04F };
    inline const juce::Colour lcdText          { 0xff123A10 };
    inline const juce::Colour lcdFrame         { 0xff1C1C1C };
    inline const juce::Colour lcdPlaceholder   { 0xff2B6B27 };

    // Pads de Goma
    inline const juce::Colour padBody          { 0xff5F5E5C };
    inline const juce::Colour padText          { 0xffF0EDE0 };
    inline const juce::Colour padLed           { 0xffFF4B3A };
    inline const juce::Colour padShadow        { 0xff3A3937 };

    // Reference Pad Tokens (Dorado cálido / Resaltado MPC)
    inline const juce::Colour padRefLed        { 0xffFFD166 };
    inline const juce::Colour padRefBorder     { 0xffFFD166 };
    inline const juce::Colour padRefBadgeBg    { 0xffFFD166 };
    inline const juce::Colour padRefBadgeText  { 0xff1E1E1C };

    // Panel de Samples (Azul DATA ENTRY)
    inline const juce::Colour bluePanelBg      { 0xff2F5683 };
    inline const juce::Colour blueRowDark      { 0xff264A74 };
    inline const juce::Colour blueRowLight     { 0xff2A517E };
    inline const juce::Colour blueTextMain     { 0xffF3EDD6 };
    inline const juce::Colour blueTextSec      { 0xffC9D6E8 };
    inline const juce::Colour blueBadgeBorder  { 0xff547CAE };

    // =========================================================================
    // Tipografía clásica MPC (Helvetica Neue Bold con tracking -1% a -2%)
    // =========================================================================
    inline juce::Font getSerigraphyFont(float size = 11.0f)
    {
        return juce::Font(juce::FontOptions("Helvetica Neue", size, juce::Font::bold))
                   .withExtraKerningFactor(-0.015f);
    }

    inline juce::Font getMonoFont(float size = 11.0f)
    {
        return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
    }

    // =========================================================================
    // Helpers de Renderizado Visual
    // =========================================================================
    inline void drawLCDBox(juce::Graphics& g, const juce::Rectangle<float>& bounds, float cornerRadius = 2.0f)
    {
        // Marco exterior negro
        g.setColour(lcdFrame);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Fondo verde LCD con margen de 2 px
        const auto inner = bounds.reduced(2.0f);
        g.setColour(lcdBg);
        g.fillRoundedRectangle(inner, juce::jmax(1.0f, cornerRadius - 1.0f));

        // Sombra interior sutil en borde superior e izquierdo
        g.setColour(juce::Colour(0x33000000));
        g.drawLine(inner.getX(), inner.getY(), inner.getRight(), inner.getY(), 1.0f);
        g.drawLine(inner.getX(), inner.getY(), inner.getX(), inner.getBottom(), 1.0f);
    }

    enum class ButtonStyle
    {
        Cream,   // Botón normal crema
        Primary, // Botón rojo estilo REC
        Danger   // Botón gris oscuro
    };

    inline void drawKeypadButton(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                 ButtonStyle style, bool isDown, bool isHovered,
                                 const juce::String& text, const juce::Path* iconPath = nullptr)
    {
        const float shadowHeight = isDown ? 1.0f : 3.0f;
        const float yOffset = isDown ? 2.0f : 0.0f;

        const auto fullArea = bounds;
        auto buttonArea = fullArea.withTrimmedBottom(isDown ? 0.0f : 3.0f).translated(0.0f, yOffset);

        juce::Colour topCol, botCol, borderCol, shadowCol, textCol;

        switch (style)
        {
            case ButtonStyle::Primary:
                topCol    = isHovered ? redTop.brighter(0.08f) : redTop;
                botCol    = isHovered ? redPrimary.brighter(0.08f) : redPrimary;
                borderCol = redShadow.darker(0.2f);
                shadowCol = redShadow;
                textCol   = redText;
                break;

            case ButtonStyle::Danger:
                topCol    = isHovered ? btnDarkTop.brighter(0.1f) : btnDarkTop;
                botCol    = isHovered ? btnDarkBottom.brighter(0.1f) : btnDarkBottom;
                borderCol = btnDarkBorder;
                shadowCol = btnDarkShadow;
                textCol   = btnDarkText;
                break;

            case ButtonStyle::Cream:
            default:
                topCol    = isHovered ? btnCreamTop.brighter(0.06f) : btnCreamTop;
                botCol    = isHovered ? btnCreamBottom.brighter(0.06f) : btnCreamBottom;
                borderCol = btnCreamBorder;
                shadowCol = btnCreamShadow;
                textCol   = textMain;
                break;
        }

        // Sombra sólida inferior de tecla física (solo cuando no está presionada)
        if (!isDown)
        {
            g.setColour(shadowCol);
            g.fillRoundedRectangle(fullArea.withTrimmedTop(fullArea.getHeight() - 4.0f), 3.0f);
        }

        // Cuerpo de la tecla con degradado vertical
        juce::ColourGradient grad(topCol, buttonArea.getX(), buttonArea.getY(),
                                  botCol, buttonArea.getX(), buttonArea.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(buttonArea, 3.0f);

        // Borde fino
        g.setColour(borderCol);
        g.drawRoundedRectangle(buttonArea, 3.0f, 1.0f);

        // Brillo superior de 1 px
        if (!isDown)
        {
            g.setColour(topCol.brighter(0.25f));
            g.drawLine(buttonArea.getX() + 2.0f, buttonArea.getY() + 1.0f,
                       buttonArea.getRight() - 2.0f, buttonArea.getY() + 1.0f, 1.0f);
        }

        // Icono y Texto
        const float xPad = (buttonArea.getWidth() < 55.0f) ? 2.0f : 6.0f;
        auto contentArea = buttonArea.reduced(xPad, 2.0f);

        if (iconPath != nullptr && !iconPath->isEmpty())
        {
            if (text.isNotEmpty())
            {
                const float iconBoxSize = juce::jmin(contentArea.getHeight() * 0.7f, 16.0f);
                auto iconArea = contentArea.removeFromLeft(iconBoxSize + 4.0f).withWidth(iconBoxSize);
                g.setColour(textCol);
                g.fillPath(*iconPath, iconPath->getTransformToScaleToFit(iconArea, true));
            }
            else
            {
                // Botón puramente de icono (ej: botones cuadrados de transporte)
                const float iconSize = juce::jmin(contentArea.getWidth() * 0.70f, contentArea.getHeight() * 0.70f);
                auto iconArea = juce::Rectangle<float>(contentArea.getCentreX() - iconSize * 0.5f,
                                                       contentArea.getCentreY() - iconSize * 0.5f,
                                                       iconSize, iconSize);
                g.setColour(textCol);
                g.fillPath(*iconPath, iconPath->getTransformToScaleToFit(iconArea, true));
            }
        }

        if (text.isNotEmpty())
        {
            g.setColour(textCol);
            const float fontSize = (buttonArea.getHeight() <= 28.0f) ? 10.0f : 10.5f;
            g.setFont(getSerigraphyFont(fontSize));
            g.drawText(text.toUpperCase(), contentArea, juce::Justification::centred, true);
        }
    }
} // namespace theme

} // namespace drummatch
