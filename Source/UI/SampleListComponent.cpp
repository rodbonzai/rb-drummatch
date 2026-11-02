#include "SampleListComponent.h"

namespace drummatch
{

enum Columns
{
    ColName = 1,
    ColCategory,
    ColWave
};

// ==============================================================================
// ChipButton
// ==============================================================================

void SampleListComponent::ChipButton::paintButton(juce::Graphics& g, bool isHighlighted, bool isDown)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);

    if (active)
    {
        // Activo: Fondo crema MPC y texto oscuro en negrita
        g.setColour(theme::btnCreamTop);
        g.fillRoundedRectangle(b, 3.0f);
        g.setColour(theme::btnCreamBorder);
        g.drawRoundedRectangle(b, 3.0f, 1.0f);

        g.setColour(theme::textMain);
        g.setFont(theme::getSerigraphyFont(10.0f));
        g.drawText(getButtonText(), b, juce::Justification::centred, false);
    }
    else
    {
        // Inactivo: Azul oscuro y texto gris azulado
        const auto bg = isHighlighted ? theme::blueRowLight.brighter(0.1f) : theme::blueRowDark.darker(0.1f);
        g.setColour(bg);
        g.fillRoundedRectangle(b, 3.0f);
        g.setColour(theme::blueBadgeBorder.withAlpha(0.5f));
        g.drawRoundedRectangle(b, 3.0f, 1.0f);

        g.setColour(isHighlighted ? theme::blueTextMain : theme::blueTextSec);
        g.setFont(theme::getSerigraphyFont(10.0f));
        g.drawText(getButtonText(), b, juce::Justification::centred, false);
    }
}

// ==============================================================================
// SampleListComponent
// ==============================================================================

SampleListComponent::SampleListComponent(SampleCatalog& cat, AudioEngine& engine)
    : catalog(cat), audioEngine(engine)
{
    catalog.addListener(this);

    // Buscador estilo LCD verde
    searchBox.setTextToShowWhenEmpty("SEARCH...", theme::lcdPlaceholder);
    searchBox.setColour(juce::TextEditor::backgroundColourId, theme::lcdBg);
    searchBox.setColour(juce::TextEditor::textColourId, theme::lcdText);
    searchBox.setColour(juce::TextEditor::highlightColourId, theme::lcdText.withAlpha(0.2f));
    searchBox.setColour(juce::TextEditor::highlightedTextColourId, theme::lcdText);
    searchBox.setColour(juce::TextEditor::outlineColourId, theme::lcdFrame);
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, theme::lcdFrame);
    searchBox.setFont(theme::getMonoFont(11.5f));
    searchBox.onTextChange = [this]() { refreshData(); };
    addAndMakeVisible(searchBox);

    // Filtros de categoría: ALL, KICK, SNARE, HAT, PERC
    const struct { const char* label; CategoryFilter filter; } filters[] = {
        { "ALL",   CategoryFilter::All },
        { "KICK",  CategoryFilter::Kick },
        { "SNARE", CategoryFilter::Snare },
        { "HAT",   CategoryFilter::Hat },
        { "PERC",  CategoryFilter::Perc }
    };

    for (const auto& item : filters)
    {
        auto chip = std::make_unique<ChipButton>(item.label, item.filter);
        chip->setActive(item.filter == CategoryFilter::All);
        chip->onClick = [this, f = item.filter]()
        {
            currentFilter = f;
            for (auto& c : filterChips)
                c->setActive(c->getFilterType() == currentFilter);
            refreshData();
        };
        addAndMakeVisible(*chip);
        filterChips.push_back(std::move(chip));
    }

    // Configuración de la tabla compacta (28 px por fila)
    auto& header = table.getHeader();
    header.setColour(juce::TableHeaderComponent::backgroundColourId, theme::blueRowDark);
    header.setColour(juce::TableHeaderComponent::textColourId, theme::blueTextSec);
    header.setColour(juce::TableHeaderComponent::outlineColourId, theme::bluePanelBg.darker(0.3f));

    header.addColumn("NAME", ColName, 175, 120, 300);
    header.addColumn("CAT",  ColCategory, 74, 60, 100);
    header.addColumn("WAVE", ColWave, 55, 45, 80);

    table.setRowHeight(28);
    table.setModel(this);
    table.setColour(juce::ListBox::backgroundColourId, theme::bluePanelBg);
    table.setColour(juce::ListBox::outlineColourId, juce::Colour(0x00000000));
    table.onMouseDragCallback = [this](const juce::MouseEvent& e) { handleTableMouseDrag(e); };
    addAndMakeVisible(table);

    // Botones de acción inferiores
    playButton.onClick = [this]() { playSelectedSample(); };
    addAndMakeVisible(playButton);

    stopButton.onClick = [this]() { audioEngine.stop(); };
    addAndMakeVisible(stopButton);

    setRefButton.onClick = [this]() { setSelectionAsReference(); };
    addAndMakeVisible(setRefButton);

    refreshData();
}

SampleListComponent::~SampleListComponent()
{
    catalog.removeListener(this);
}

juce::String SampleListComponent::getCategoryShortBadge(InstrumentCategory cat)
{
    switch (cat)
    {
        case InstrumentCategory::Kick:           return "KICK";
        case InstrumentCategory::GhostKick:      return "G-KICK";
        case InstrumentCategory::Snare:          return "SNARE";
        case InstrumentCategory::GhostSnare:     return "G-SNARE";
        case InstrumentCategory::CrossStick:     return "X-STICK";
        case InstrumentCategory::Rim:            return "RIM";
        case InstrumentCategory::Clap:           return "CLAP";
        case InstrumentCategory::ClosedHat:      return "CL HAT";
        case InstrumentCategory::ClosedHatHard:  return "CL HAT";
        case InstrumentCategory::OpenHat:        return "OP HAT";
        case InstrumentCategory::TomLow:         return "TOM LO";
        case InstrumentCategory::TomMid:         return "TOM MID";
        case InstrumentCategory::TomHigh:        return "TOM HI";
        case InstrumentCategory::CrashCymbal:    return "CRASH";
        case InstrumentCategory::RideCymbal:     return "RIDE";
        case InstrumentCategory::Percussion01:   return "PERC 1";
        case InstrumentCategory::Percussion02:   return "PERC 2";
        case InstrumentCategory::Other:          return "PERC";
        default:                                 return "PERC";
    }
}

void SampleListComponent::drawMiniWaveform(juce::Graphics& g, const juce::Rectangle<float>& bounds, const Sample& sample)
{
    // Fondo oscuro de la mini forma de onda
    g.setColour(juce::Colour(0x44000000));
    g.fillRoundedRectangle(bounds, 2.0f);

    const uint32_t seed = juce::DefaultHashFunctions::generateHash(sample.id, 9999);
    g.setColour(theme::blueTextSec.withAlpha(0.85f));

    const int numBars = 8;
    const float barW = 2.0f;
    const float spacing = (bounds.getWidth() - numBars * barW) / static_cast<float>(numBars - 1);

    for (int b = 0; b < numBars; ++b)
    {
        float hFrac = 0.30f + 0.65f * std::abs(std::sin(static_cast<float>(seed + b * 57) * 0.17f));
        if (b == 0 || b == 1)
            hFrac = juce::jmax(hFrac, 0.75f); // transient inicial

        const float barH = juce::jlimit(2.5f, bounds.getHeight() - 2.0f, bounds.getHeight() * hFrac);
        const float bx = bounds.getX() + static_cast<float>(b) * (barW + spacing);
        const float by = bounds.getCentreY() - barH * 0.5f;

        g.fillRect(bx, by, barW, barH);
    }
}

void SampleListComponent::handleTableMouseDrag(const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() > 6 && selectedRow >= 0 && selectedRow < static_cast<int>(displaySamples.size()))
    {
        if (auto* ddc = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            if (!ddc->isDragAndDropActive())
            {
                const auto& sample = displaySamples[static_cast<size_t>(selectedRow)];

                juce::Image dragImage(juce::Image::ARGB, 160, 26, true);
                juce::Graphics g(dragImage);
                g.fillAll(theme::blueRowDark);
                g.setColour(theme::btnCreamTop);
                g.drawRect(0, 0, 160, 26, 2);
                g.setFont(theme::getMonoFont(11.0f).withStyle(juce::Font::bold));
                g.drawText(sample.getFileName().toUpperCase(), 6, 0, 148, 26, juce::Justification::centredLeft, true);

                ddc->startDragging("drummatch_sample:" + sample.id, this, juce::ScaledImage(dragImage, 1.0), true);
            }
        }
    }
}

void SampleListComponent::refreshData()
{
    auto all = catalog.getSamples(true, searchBox.getText());
    displaySamples.clear();
    displaySamples.reserve(all.size());

    for (const auto& s : all)
    {
        bool matches = true;
        switch (currentFilter)
        {
            case CategoryFilter::Kick:
                matches = (s.category == InstrumentCategory::Kick || s.category == InstrumentCategory::GhostKick);
                break;
            case CategoryFilter::Snare:
                matches = (s.category == InstrumentCategory::Snare || s.category == InstrumentCategory::GhostSnare ||
                           s.category == InstrumentCategory::CrossStick || s.category == InstrumentCategory::Rim ||
                           s.category == InstrumentCategory::Clap);
                break;
            case CategoryFilter::Hat:
                matches = (s.category == InstrumentCategory::ClosedHat || s.category == InstrumentCategory::ClosedHatHard ||
                           s.category == InstrumentCategory::OpenHat);
                break;
            case CategoryFilter::Perc:
                matches = (s.category == InstrumentCategory::TomLow || s.category == InstrumentCategory::TomMid ||
                           s.category == InstrumentCategory::TomHigh || s.category == InstrumentCategory::CrashCymbal ||
                           s.category == InstrumentCategory::RideCymbal || s.category == InstrumentCategory::Percussion01 ||
                           s.category == InstrumentCategory::Percussion02 || s.category == InstrumentCategory::Other);
                break;
            case CategoryFilter::All:
            default:
                matches = true;
                break;
        }

        if (matches)
            displaySamples.push_back(s);
    }

    std::sort(displaySamples.begin(), displaySamples.end(), [](const Sample& a, const Sample& b)
    {
        return a.getFileName().compareNatural(b.getFileName()) < 0;
    });

    table.updateContent();
    repaint();
}

int SampleListComponent::getNumRows()
{
    return static_cast<int>(displaySamples.size());
}

void SampleListComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    const bool isRef = (rowNumber >= 0 && rowNumber < static_cast<int>(displaySamples.size())) &&
                       displaySamples[static_cast<size_t>(rowNumber)].isReference;

    if (rowIsSelected)
    {
        // Fila seleccionada: oscurecida con barra lateral crema de 4 px
        g.fillAll(juce::Colour(0x55000000));
        g.setColour(theme::btnCreamTop);
        g.fillRect(0, 0, 4, height);
    }
    else
    {
        // Filas alternadas en azul DATA ENTRY
        if (rowNumber % 2 == 0)
            g.fillAll(theme::blueRowDark);
        else
            g.fillAll(theme::blueRowLight);

        // Fila de referencia: barra lateral crema de 4 px
        if (isRef)
        {
            g.setColour(theme::btnCreamTop);
            g.fillRect(0, 0, 4, height);
        }
    }
}

void SampleListComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(displaySamples.size()))
        return;

    const auto& sample = displaySamples[static_cast<size_t>(rowNumber)];
    const auto area = juce::Rectangle<int>(8, 0, width - 12, height);

    switch (columnId)
    {
        case ColName:
        {
            const auto font = sample.isReference ? theme::getMonoFont(11.0f).withStyle(juce::Font::bold)
                                                 : theme::getMonoFont(11.0f);
            g.setFont(font);
            g.setColour(sample.isReference ? theme::btnCreamTop : theme::blueTextMain);
            g.drawText(sample.getFileName().toUpperCase(), area, juce::Justification::centredLeft, true);
            break;
        }
        case ColCategory:
        {
            // Etiqueta pequeña con borde: [ CL HAT ]
            const float badgeW = juce::jmin(static_cast<float>(width - 8), 58.0f);
            const float badgeH = 18.0f;
            const auto badgeArea = juce::Rectangle<float>(static_cast<float>(area.getX()),
                                                         static_cast<float>(area.getCentreY()) - badgeH * 0.5f,
                                                         badgeW, badgeH);

            g.setColour(theme::bluePanelBg.darker(0.3f));
            g.fillRoundedRectangle(badgeArea, 2.0f);
            g.setColour(theme::blueBadgeBorder);
            g.drawRoundedRectangle(badgeArea, 2.0f, 1.0f);

            g.setFont(theme::getMonoFont(9.5f).withStyle(juce::Font::bold));
            g.setColour(theme::blueTextSec);
            g.drawText(getCategoryShortBadge(sample.category), badgeArea, juce::Justification::centred, false);
            break;
        }
        case ColWave:
        {
            // Mini waveform de 36 x 12 px
            const float waveW = 36.0f;
            const float waveH = 12.0f;
            const auto waveArea = juce::Rectangle<float>(static_cast<float>(area.getX() + 2),
                                                         static_cast<float>(area.getCentreY()) - waveH * 0.5f,
                                                         waveW, waveH);
            drawMiniWaveform(g, waveArea, sample);
            break;
        }
        default:
            break;
    }
}

void SampleListComponent::cellClicked(int rowNumber, int, const juce::MouseEvent&)
{
    selectedRow = rowNumber;
    playSelectedSample();
}

void SampleListComponent::cellDoubleClicked(int rowNumber, int, const juce::MouseEvent&)
{
    selectedRow = rowNumber;
    playSelectedSample();
}

void SampleListComponent::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(displaySamples.size()) && lastRowSelected != selectedRow)
    {
        selectedRow = lastRowSelected;
        playSelectedSample();
    }
}

void SampleListComponent::playSelectedSample()
{
    if (selectedRow >= 0 && selectedRow < static_cast<int>(displaySamples.size()))
    {
        const auto& s = displaySamples[static_cast<size_t>(selectedRow)];
        audioEngine.playSample(s.file);

        if (onSamplePlayed)
            onSamplePlayed(s);
    }
}

void SampleListComponent::setSelectionAsReference()
{
    if (selectedRow >= 0 && selectedRow < static_cast<int>(displaySamples.size()))
    {
        const auto& s = displaySamples[static_cast<size_t>(selectedRow)];
        catalog.setReferenceSample(s);

        if (onReferenceSampleSelected)
            onReferenceSampleSelected(s);

        refreshData();
    }
}

void SampleListComponent::samplesUpdated()
{
    refreshData();
}

void SampleListComponent::referenceSampleChanged(const std::optional<Sample>&)
{
    refreshData();
}

void SampleListComponent::paint(juce::Graphics& g)
{
    // Fondo azul DATA ENTRY (#2F5683)
    g.fillAll(theme::bluePanelBg);

    // Borde exterior
    g.setColour(theme::blueBadgeBorder.withAlpha(0.35f));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);

    // Encabezado "SAMPLES"
    g.setColour(theme::blueTextMain);
    g.setFont(theme::getSerigraphyFont(11.0f));
    g.drawText("SAMPLES", 12, 10, 100, 20, juce::Justification::centredLeft, false);

    // Contador en monoespaciada a la derecha
    g.setColour(theme::blueTextSec);
    g.setFont(theme::getMonoFont(11.0f));
    g.drawText(juce::String(displaySamples.size()), getWidth() - 112, 10, 100, 20, juce::Justification::centredRight, false);

    // Marco negro alrededor del buscador LCD
    if (!searchBoxLcdArea.isEmpty())
    {
        g.setColour(theme::lcdFrame);
        g.drawRect(searchBoxLcdArea, 2);
    }
}

void SampleListComponent::resized()
{
    auto area = getLocalBounds().reduced(12, 10);

    // Encabezado (título y contador dibujados en paint)
    area.removeFromTop(24);

    // Buscador estilo LCD verde
    auto searchArea = area.removeFromTop(28);
    searchBoxLcdArea = searchArea;
    searchBox.setBounds(searchArea.reduced(2));

    area.removeFromTop(8);

    // Chips de filtro: ALL, KICK, SNARE, HAT, PERC
    auto chipRow = area.removeFromTop(24);
    const int numChips = static_cast<int>(filterChips.size());
    const int chipGap = 4;
    const int chipW = (chipRow.getWidth() - (numChips - 1) * chipGap) / numChips;

    for (int i = 0; i < numChips; ++i)
    {
        filterChips[static_cast<size_t>(i)]->setBounds(chipRow.removeFromLeft(chipW));
        if (i < numChips - 1)
            chipRow.removeFromLeft(chipGap);
    }

    area.removeFromTop(8);

    // Barra de control inferior: PLAY, STOP y SET AS REF
    auto bottomBar = area.removeFromBottom(32);
    playButton.setBounds(bottomBar.removeFromLeft(64));
    bottomBar.removeFromLeft(6);
    stopButton.setBounds(bottomBar.removeFromLeft(64));
    bottomBar.removeFromLeft(8);
    setRefButton.setBounds(bottomBar); // ocupa el resto del ancho

    area.removeFromBottom(8);

    // Tabla de samples
    table.setBounds(area);
}

} // namespace drummatch
