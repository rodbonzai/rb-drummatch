#include "WaveformComponent.h"

namespace drummatch
{

// ==============================================================================
// TransportButton (Réplica exacta de MPC 60 / media_1793570756120_ea788ede.png)
// ==============================================================================

WaveformComponent::TransportButton::TransportButton(Type t)
    : juce::Button(""), type(t)
{
}

void WaveformComponent::TransportButton::setType(Type t)
{
    if (type != t)
    {
        type = t;
        repaint();
    }
}

void WaveformComponent::TransportButton::paintButton(juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto fullBounds = getLocalBounds().toFloat();
    const float yOffset = isDown ? 2.0f : 0.0f;
    const auto faceBounds = fullBounds.withTrimmedBottom(isDown ? 0.0f : 3.0f).translated(0.0f, yOffset);

    if (type == Type::Play || type == Type::Pause)
    {
        // 1. Sombra inferior sólida oscura (rojo oscuro/negro)
        if (!isDown)
        {
            g.setColour(juce::Colour(0xff4A0D0D));
            g.fillRoundedRectangle(fullBounds.withTrimmedTop(fullBounds.getHeight() - 4.0f), 4.0f);
        }

        // 2. Degradado rojo REC (#C23634 -> #9E2222)
        const auto topCol = isHighlighted ? juce::Colour(0xffD14240) : juce::Colour(0xffC23634);
        const auto botCol = isHighlighted ? juce::Colour(0xffAA2624) : juce::Colour(0xff9E2222);

        juce::ColourGradient grad(topCol, faceBounds.getX(), faceBounds.getY(),
                                  botCol, faceBounds.getX(), faceBounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(faceBounds, 4.0f);

        // 3. Borde fino oscuro (#681313)
        g.setColour(juce::Colour(0xff681313));
        g.drawRoundedRectangle(faceBounds, 4.0f, 1.5f);

        // 4. Brillo superior de 1 px
        if (!isDown)
        {
            g.setColour(juce::Colour(0xffE05A58));
            g.drawLine(faceBounds.getX() + 3.0f, faceBounds.getY() + 1.0f,
                       faceBounds.getRight() - 3.0f, faceBounds.getY() + 1.0f, 1.0f);
        }

        // 5. Icono blanco prominente
        g.setColour(juce::Colours::white);
        if (type == Type::Play)
        {
            // Gran triángulo blanco nítido
            const float triW = 19.0f;
            const float triH = 21.0f;
            const float cx = faceBounds.getCentreX() + 1.5f; // centro óptico
            const float cy = faceBounds.getCentreY();

            juce::Path tri;
            tri.addTriangle(cx - triW * 0.45f, cy - triH * 0.5f,
                            cx - triW * 0.45f, cy + triH * 0.5f,
                            cx + triW * 0.55f, cy);
            g.fillPath(tri);
        }
        else // Pause
        {
            const float barW = 5.0f;
            const float barH = 19.0f;
            const float gap = 5.0f;
            const float x1 = faceBounds.getCentreX() - barW - gap * 0.5f;
            const float x2 = faceBounds.getCentreX() + gap * 0.5f;
            const float y = faceBounds.getCentreY() - barH * 0.5f;

            g.fillRoundedRectangle(x1, y, barW, barH, 1.0f);
            g.fillRoundedRectangle(x2, y, barW, barH, 1.0f);
        }
    }
    else // Type::Stop
    {
        // 1. Sombra inferior sólida crema oscuro (#8C8675)
        if (!isDown)
        {
            g.setColour(juce::Colour(0xff8C8675));
            g.fillRoundedRectangle(fullBounds.withTrimmedTop(fullBounds.getHeight() - 4.0f), 4.0f);
        }

        // 2. Cuerpo crema con relieve (#F3EDD6 -> #DDD5B8)
        const auto topCol = isHighlighted ? juce::Colour(0xffFAF5E2) : juce::Colour(0xffF3EDD6);
        const auto botCol = isHighlighted ? juce::Colour(0xffE5DCBD) : juce::Colour(0xffDDD5B8);

        juce::ColourGradient grad(topCol, faceBounds.getX(), faceBounds.getY(),
                                  botCol, faceBounds.getX(), faceBounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(faceBounds, 4.0f);

        // 3. Borde fino crema (#9C988B)
        g.setColour(juce::Colour(0xff9C988B));
        g.drawRoundedRectangle(faceBounds, 4.0f, 1.5f);

        // 4. Brillo superior de 1 px
        if (!isDown)
        {
            g.setColour(juce::Colour(0xffFFFDF5));
            g.drawLine(faceBounds.getX() + 3.0f, faceBounds.getY() + 1.0f,
                       faceBounds.getRight() - 3.0f, faceBounds.getY() + 1.0f, 1.0f);
        }

        // 5. Icono de stop: Cuadrado negro carbón con esquinas redondeadas (~21x21 px)
        const float sqSize = 21.0f;
        const auto sqRect = juce::Rectangle<float>(faceBounds.getCentreX() - sqSize * 0.5f,
                                                   faceBounds.getCentreY() - sqSize * 0.5f,
                                                   sqSize, sqSize);

        g.setColour(juce::Colour(0xff1E1E1C));
        g.fillRoundedRectangle(sqRect, 3.5f);
        g.setColour(juce::Colour(0xff111110));
        g.drawRoundedRectangle(sqRect, 3.5f, 1.0f);
    }
}

// ==============================================================================
// MPC60KnobLookAndFeel (Knob Rotatorio AKAI MPC 60)
// ==============================================================================

void WaveformComponent::MPC60KnobLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 5.0f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. Pista de arco / ranura exterior
    const float arcRadius = radius + 3.0f;
    juce::Path bgArc;
    bgArc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff2A2A28));
    g.strokePath(bgArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Arco de nivel activo (rojo primario MPC)
    if (sliderPosProportional > 0.01f)
    {
        juce::Path valArc;
        valArc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f, rotaryStartAngle, currentAngle, true);
        g.setColour(theme::redPrimary);
        g.strokePath(valArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Puntos de referencia / ticks (solo mínimo y máximo, estilo hardware MPC 60)
    const float tickRadius = arcRadius + 3.0f;
    const float angles[] = { rotaryStartAngle, rotaryEndAngle };
    g.setColour(theme::textSecondary.withAlpha(0.65f));
    for (float a : angles)
    {
        const float tx = cx + tickRadius * std::sin(a);
        const float ty = cy - tickRadius * std::cos(a);
        g.fillEllipse(tx - 1.25f, ty - 1.25f, 2.5f, 2.5f);
    }

    // 2. Sombra sólida inferior del knob físico
    g.setColour(juce::Colour(0x60000000));
    g.fillEllipse(cx - radius, cy - radius + 2.5f, radius * 2.0f, radius * 2.0f);

    // 3. Falda exterior oscura estriada del knob
    juce::ColourGradient skirtGrad(juce::Colour(0xff4A4A48), cx, cy - radius,
                                  juce::Colour(0xff2E2E2D), cx, cy + radius, false);
    g.setGradientFill(skirtGrad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colour(0xff1C1C1C));
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // 4. Cara superior cilíndrica del knob (crema MPC 60)
    const float capRadius = radius - 3.5f;
    juce::ColourGradient capGrad(theme::btnCreamTop, cx, cy - capRadius,
                                theme::btnCreamBottom, cx, cy + capRadius, false);
    g.setGradientFill(capGrad);
    g.fillEllipse(cx - capRadius, cy - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // Borde fino de la tapa
    g.setColour(theme::btnCreamBorder);
    g.drawEllipse(cx - capRadius, cy - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.0f);

    // Brillo sutil superior (1px)
    g.setColour(juce::Colour(0x75ffffff));
    juce::Path highlightArc;
    highlightArc.addCentredArc(cx, cy, capRadius - 0.75f, capRadius - 0.75f, 0.0f,
                               -juce::MathConstants<float>::pi * 0.45f,
                                juce::MathConstants<float>::pi * 0.45f, true);
    g.strokePath(highlightArc, juce::PathStrokeType(1.0f));

    // 5. Aguja indicadora roja incrustada (sin círculo central)
    const float innerR = 2.0f;
    const float outerR = capRadius - 1.5f;
    const float sinA = std::sin(currentAngle);
    const float cosA = std::cos(currentAngle);

    const float x1 = cx + innerR * sinA;
    const float y1 = cy - innerR * cosA;
    const float x2 = cx + outerR * sinA;
    const float y2 = cy - outerR * cosA;

    g.setColour(theme::redPrimary);
    g.drawLine(x1, y1, x2, y2, 2.5f);
}

// ==============================================================================
// WaveformComponent
// ==============================================================================

WaveformComponent::WaveformComponent(AudioEngine& engine)
    : audioEngine(engine),
      thumbnail(512, engine.getFormatManager(), thumbnailCache)
{
    audioEngine.addListener(this);
    thumbnail.addChangeListener(this);

    // Botón PLAY rojo (44 x 44 px cuadrado)
    playButton.onClick = [this]()
    {
        if (audioEngine.isPlaying())
            audioEngine.stop();
        else
            audioEngine.play();
    };
    addAndMakeVisible(playButton);

    // Botón STOP crema (44 x 44 px cuadrado)
    stopButton.onClick = [this]() { audioEngine.stop(); };
    addAndMakeVisible(stopButton);

    // Etiquetas de información del sample
    sampleNameLabel.setText("NO SAMPLE LOADED", juce::dontSendNotification);
    sampleNameLabel.setFont(theme::getSerigraphyFont(13.0f));
    sampleNameLabel.setColour(juce::Label::textColourId, theme::textMain);
    addAndMakeVisible(sampleNameLabel);

    formatBadgeLabel.setText("---", juce::dontSendNotification);
    formatBadgeLabel.setFont(theme::getMonoFont(11.0f));
    formatBadgeLabel.setColour(juce::Label::textColourId, theme::textSecondary);
    addAndMakeVisible(formatBadgeLabel);

    // Serigrafía "VOL"
    volumeLabel.setText("VOL", juce::dontSendNotification);
    volumeLabel.setFont(theme::getSerigraphyFont(10.0f));
    volumeLabel.setColour(juce::Label::textColourId, theme::textSecondary);
    volumeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(volumeLabel);

    // Knob rotatorio de volumen estilo MPC 60
    volumeSlider.setLookAndFeel(&knobLookAndFeel);
    volumeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f,
                                     true);
    volumeSlider.setRange(0.0, 1.2, 0.01);
    volumeSlider.setValue(0.9, juce::dontSendNotification);
    volumeSlider.onValueChange = [this]()
    {
        audioEngine.setGain(static_cast<float>(volumeSlider.getValue()));
    };
    addAndMakeVisible(volumeSlider);

    // Readout de tiempo en monoespaciada
    timeLabel.setText("0.00s / 0.00s", juce::dontSendNotification);
    timeLabel.setFont(theme::getMonoFont(11.0f));
    timeLabel.setColour(juce::Label::textColourId, theme::textMain);
    timeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeLabel);

    startTimerHz(30);
}

WaveformComponent::~WaveformComponent()
{
    stopTimer();
    audioEngine.removeListener(this);
    thumbnail.removeChangeListener(this);
    volumeSlider.setLookAndFeel(nullptr);
}

void WaveformComponent::loadFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    currentFile = file;
    sampleNameLabel.setText(file.getFileName().toUpperCase(), juce::dontSendNotification);

    if (auto* reader = audioEngine.getFormatManager().createReaderFor(file))
    {
        const juce::String formatInfo = file.getFileExtension().toUpperCase().replace(".", "") + " " +
                                        juce::String(reader->sampleRate / 1000.0, 1) + "kHz " +
                                        juce::String(reader->bitsPerSample) + "-bit " +
                                        (reader->numChannels == 2 ? "STEREO" : "MONO");
        formatBadgeLabel.setText(formatInfo, juce::dontSendNotification);
        delete reader;
    }

    thumbnail.setSource(new juce::FileInputSource(file));
    repaint();
}

void WaveformComponent::playbackStateChanged(bool isPlaying)
{
    playButton.setType(isPlaying ? TransportButton::Type::Pause : TransportButton::Type::Play);
    repaint();
}

void WaveformComponent::sampleLoaded(const juce::File& file)
{
    loadFile(file);
}

void WaveformComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &thumbnail)
        repaint();
}

void WaveformComponent::timerCallback()
{
    const double pos = audioEngine.getCurrentPosition();
    const double len = audioEngine.getLengthInSeconds();

    timeLabel.setText(juce::String(pos, 2) + "s / " + juce::String(len, 2) + "s", juce::dontSendNotification);

    if (audioEngine.isPlaying())
        repaint();
}

void WaveformComponent::mouseDown(const juce::MouseEvent& e)
{
    if (waveArea.contains(e.getPosition()) && audioEngine.getLengthInSeconds() > 0.0)
    {
        const double proportion = juce::jlimit(0.0, 1.0,
            static_cast<double>(e.x - waveArea.getX()) / static_cast<double>(waveArea.getWidth()));
        const double seekTime = proportion * audioEngine.getLengthInSeconds();
        audioEngine.setPosition(seekTime);
        repaint();
    }
}

void WaveformComponent::paint(juce::Graphics& g)
{
    // Fondo de Chasis (#BAB7AA)
    g.fillAll(theme::chassis);

    // Borde superior de separación del chasis
    g.setColour(theme::textSecondary.withAlpha(0.35f));
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

    // Gran pantalla LCD verde para la forma de onda
    if (!lcdArea.isEmpty())
    {
        theme::drawLCDBox(g, lcdArea.toFloat(), 2.0f);

        // Limitar estrictamente todo el dibujo de onda y playhead al interior de waveArea
        juce::Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(waveArea);

        // Línea central verde oscuro
        g.setColour(theme::lcdText.withAlpha(0.25f));
        g.drawHorizontalLine(waveArea.getCentreY(), static_cast<float>(waveArea.getX()), static_cast<float>(waveArea.getRight()));

        if (thumbnail.getNumChannels() > 0)
        {
            // Forma de onda en verde oscuro LCD (#123A10)
            g.setColour(theme::lcdText);
            thumbnail.drawChannels(g, waveArea.reduced(2, 4), 0.0, thumbnail.getTotalLength(), 1.0f);

            // Playhead vertical (solo mientras se reproduce y estrictamente contenido)
            const double len = audioEngine.getLengthInSeconds();
            if (len > 0.0 && audioEngine.isPlaying())
            {
                const double currentPos = juce::jlimit(0.0, len, audioEngine.getCurrentPosition());
                const float playheadX = static_cast<float>(waveArea.getX()) +
                    static_cast<float>(currentPos / len) * static_cast<float>(waveArea.getWidth());

                g.setColour(theme::lcdText);
                g.drawLine(playheadX, static_cast<float>(waveArea.getY() + 1),
                           playheadX, static_cast<float>(waveArea.getBottom() - 1), 2.0f);
            }
        }
        else
        {
            g.setColour(theme::lcdPlaceholder);
            g.setFont(theme::getMonoFont(11.0f));
            g.drawText("NO SAMPLE LOADED  |  CLICK A SAMPLE TO PREVIEW WAVEFORM",
                       waveArea, juce::Justification::centred, false);
        }
    }
}

void WaveformComponent::resized()
{
    auto area = getLocalBounds().reduced(14, 10);

    // 1. Zona izquierda: Botones 44 x 44 px + Info de audio (según media_1793570756120_ea788ede.png)
    playButton.setBounds(area.getX(), area.getY() + 6, 44, 44);
    stopButton.setBounds(area.getX() + 50, area.getY() + 6, 44, 44);

    const int infoX = area.getX() + 104;
    const int infoW = 190;
    sampleNameLabel.setBounds(infoX, area.getY() + 8, infoW, 20);
    formatBadgeLabel.setBounds(infoX, area.getY() + 30, infoW, 18);

    // 2. Zona derecha: Knob rotatorio de volumen y tiempo
    const int knobSectionW = 96;
    const int rightX = area.getRight() - knobSectionW;

    volumeLabel.setBounds(rightX, area.getY() + 2, knobSectionW, 14);
    volumeSlider.setBounds(rightX + (knobSectionW - 46) / 2, area.getY() + 18, 46, 46);
    timeLabel.setBounds(rightX, area.getY() + 66, knobSectionW, 16);

    // 3. Zona central: Pantalla LCD con forma de onda
    const int centerLeft = infoX + infoW + 12;
    const int centerRight = rightX - 12;
    const int centerW = juce::jmax(100, centerRight - centerLeft);

    lcdArea = juce::Rectangle<int>(centerLeft, area.getY(), centerW, area.getHeight());
    waveArea = lcdArea.reduced(4, 2);
}

} // namespace drummatch
