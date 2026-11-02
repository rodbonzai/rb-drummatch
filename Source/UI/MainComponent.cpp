#include "MainComponent.h"
#include "MPC60Theme.h"

namespace drummatch
{

MainComponent::MainComponent()
{
    // Habilitar captura de eventos de teclado físico
    setWantsKeyboardFocus(true);

    // 1. Abrir base de datos SQLite y cargar estado persistente
    database.open(Database::getDefaultDatabaseFile());
    catalog.loadFromDatabase();

    // 2. Cargar layout de pads persistido
    const auto savedLayout = database.loadPadLayout("default");
    padGrid.setPadDefinitions(savedLayout);

    // 3. Montar componentes en la interfaz
    addAndMakeVisible(topBar);
    addAndMakeVisible(sidebar);
    addAndMakeVisible(padGrid);
    addAndMakeVisible(padDetail);
    addAndMakeVisible(sampleList);
    addAndMakeVisible(waveformBar);

    // 4. Interacción Pad Grid
    padGrid.onPadSelected = [this](const PadDefinition& def)
    {
        padDetail.setSelectedPad(def);

        if (def.assignedSample.has_value() && def.assignedSample->file.existsAsFile())
        {
            audioEngine.playPadSample(def.assignedSample->file, def.index);
            waveformBar.loadFile(def.assignedSample->file);
        }
    };

    padGrid.getSampleCategoryCallback = [this](const juce::String& sampleId) -> InstrumentCategory
    {
        if (auto s = catalog.findSampleById(sampleId))
            return s->category;
        return InstrumentCategory::Other;
    };

    padGrid.onSampleDroppedOnPad = [this](const juce::String& sampleId, int padIndex)
    {
        if (auto s = catalog.findSampleById(sampleId))
        {
            padGrid.assignSampleToPad(padIndex, *s);
            database.savePadLayout("default", padGrid.getPadDefinitions());

            padGrid.selectPad(padIndex);
            audioEngine.playPadSample(s->file, padIndex);
            waveformBar.loadFile(s->file);

            if (auto pad = padGrid.getPadDefinition(padIndex))
                padDetail.setSelectedPad(*pad);

            syncReferencePad();
        }
    };

    // Botones de acción del header del grid: Auto-Match Kit y Variar Combinación
    padGrid.onAutoMatchRequested = [this]()
    {
        varietyCounter = 0;
        triggerAcousticMatching(0);
    };

    padGrid.onShuffleMatchRequested = [this]()
    {
        varietyCounter++;
        triggerAcousticMatching(varietyCounter);
    };

    // 5. Interacción Pad Detail (Editor, Alternativas y Layout)
    padDetail.onPadUpdated = [this](const PadDefinition& updatedDef)
    {
        padGrid.updatePadDefinition(updatedDef);
        database.savePadLayout("default", padGrid.getPadDefinitions());
    };

    padDetail.onPlaySampleRequested = [this](int padIndex)
    {
        if (auto pad = padGrid.getPadDefinition(padIndex))
        {
            if (pad->assignedSample.has_value() && pad->assignedSample->file.existsAsFile())
            {
                audioEngine.playPadSample(pad->assignedSample->file, padIndex);
                waveformBar.loadFile(pad->assignedSample->file);
            }
        }
    };

    padDetail.onClearSampleRequested = [this](int padIndex)
    {
        audioEngine.stopPadVoice(padIndex);

        if (auto pad = padGrid.getPadDefinition(padIndex))
        {
            if (pad->isReference)
                catalog.clearReferenceSample();
        }

        padGrid.clearPadSample(padIndex);
        database.savePadLayout("default", padGrid.getPadDefinitions());

        if (auto pad = padGrid.getPadDefinition(padIndex))
            padDetail.setSelectedPad(*pad);

        syncReferencePad();
    };

    padDetail.onResetLayoutRequested = [this]()
    {
        const auto defaultLayout = PadDefinition::getDefaultLayout();
        padGrid.setPadDefinitions(defaultLayout);
        database.savePadLayout("default", defaultLayout);
        padDetail.clearSelection();
        syncReferencePad();
    };

    padDetail.onNextCandidateRequested = [this](int padIndex)
    {
        selectCandidateForPad(padIndex, 1);
    };

    padDetail.onPrevCandidateRequested = [this](int padIndex)
    {
        selectCandidateForPad(padIndex, -1);
    };

    // 6. Interacción Sample List y Reference Sample
    sampleList.onSamplePlayed = [this](const Sample& sample)
    {
        waveformBar.loadFile(sample.file);
    };

    sampleList.onReferenceSampleSelected = [this](const Sample& sample)
    {
        // Si el usuario tiene un pad seleccionado manualmente, asignar la referencia a ese pad.
        // Si no, asignarlo al pad que mejor coincida con su categoría.
        int targetPad = padGrid.getSelectedPadIndex();
        if (targetPad < 0)
            targetPad = padGrid.findPadIndexForCategory(sample.category);

        padGrid.assignSampleToPad(targetPad, sample);
        database.savePadLayout("default", padGrid.getPadDefinitions());

        audioEngine.playSample(sample.file);
        waveformBar.loadFile(sample.file);

        // Resaltar pad de referencia y sincronizar
        syncReferencePad();

        // Disparar búsqueda de compatibilidad acústica completa
        varietyCounter = 0;
        triggerAcousticMatching(0);
    };

    // 7. Botones de acción en la barra superior: MATCH KIT y CLEAR ALL
    topBar.onMatchRequested = [this]()
    {
        varietyCounter = 0;
        triggerAcousticMatching(0);
    };

    topBar.onClearRequested = [this]()
    {
        padGrid.clearAllPads();
        catalog.clearReferenceSample();
        database.savePadLayout("default", padGrid.getPadDefinitions());
        padDetail.clearSelection();
        audioEngine.stop();
        syncReferencePad();
    };

    // 8. Callback al completar el ranking acústico: reemplaza y asigna samples a todos los pads
    rankingEngine.onRankingCompleted = [this](const std::vector<MatchCandidate>& bestMatches)
    {
        for (const auto& match : bestMatches)
        {
            if (auto pad = padGrid.getPadDefinition(match.padIndex))
            {
                auto updatedDef = *pad;

                // El sample de referencia activo NUNCA debe ser reemplazado por el matching
                if (updatedDef.isReference)
                {
                    if (auto ref = catalog.getReferenceSample())
                    {
                        updatedDef.assignedSample = *ref;
                        updatedDef.matchScore = 100.0f;
                        updatedDef.matchReason = "Active Reference Sample";
                        updatedDef.alternativeIndex = 0;
                        updatedDef.totalAlternatives = 1;
                        padGrid.updatePadDefinition(updatedDef);
                        continue;
                    }
                }

                updatedDef.assignedSample = match.sample;
                updatedDef.matchScore = match.score;
                updatedDef.matchReason = match.reason;
                updatedDef.alternativeIndex = 0;
                updatedDef.totalAlternatives = rankingEngine.getTotalCandidatesForPad(match.padIndex);
                padGrid.updatePadDefinition(updatedDef);
            }
        }

        database.savePadLayout("default", padGrid.getPadDefinitions());

        if (padGrid.getSelectedPadIndex() >= 0)
        {
            if (auto pad = padGrid.getPadDefinition(padGrid.getSelectedPadIndex()))
                padDetail.setSelectedPad(*pad);
        }

        syncReferencePad();
    };

    // Sincronizar estado inicial del pad de referencia
    syncReferencePad();

    // Tamaño de ventana MPC 60 (1200 x 760)
    setSize(1200, 760);
}

void MainComponent::triggerAcousticMatching(int offset)
{
    auto ref = catalog.getReferenceSample();
    if (!ref.has_value())
    {
        const auto samples = catalog.getSamples(true);
        if (!samples.empty())
        {
            catalog.setReferenceSample(samples.front());
            ref = catalog.getReferenceSample();
            padGrid.assignSampleToCategory(ref->category, *ref);
            syncReferencePad();
        }
        else
        {
            return;
        }
    }

    const auto candidates = catalog.getSamples(true);
    const auto currentLayout = padGrid.getPadDefinitions();

    rankingEngine.rankCandidatesAsync(*ref, candidates, currentLayout, offset);
}

void MainComponent::selectCandidateForPad(int padIndex, int candidateOffset)
{
    auto pad = padGrid.getPadDefinition(padIndex);
    if (!pad.has_value() || pad->isReference || pad->totalAlternatives <= 1)
        return;

    const int newIndex = juce::jlimit(0, pad->totalAlternatives - 1, pad->alternativeIndex + candidateOffset);
    if (newIndex == pad->alternativeIndex)
        return;

    if (auto cand = rankingEngine.getCandidateAt(padIndex, newIndex))
    {
        auto updated = *pad;
        updated.assignedSample = cand->sample;
        updated.matchScore = cand->score;
        updated.matchReason = cand->reason;
        updated.alternativeIndex = newIndex;

        padGrid.updatePadDefinition(updated);
        padDetail.setSelectedPad(updated);

        database.savePadLayout("default", padGrid.getPadDefinitions());

        audioEngine.playPadSample(cand->sample.file, padIndex);
        waveformBar.loadFile(cand->sample.file);
    }
}

void MainComponent::triggerPadByIndex(int index)
{
    padGrid.triggerPad(index);
}

void MainComponent::syncReferencePad()
{
    juce::String refId = "";
    if (auto ref = catalog.getReferenceSample())
        refId = ref->id;

    const juce::String padId = padGrid.setReferenceSampleId(refId);
    topBar.setReferencePadId(padId);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Si el usuario está escribiendo en el buscador de samples, no capturar teclas
    if (sampleList.hasSearchFocus())
        return false;

    const auto kc = key.getKeyCode();
    const juce::juce_wchar ch = key.getTextCharacter();

    // =========================================================================
    // Mapeo 4x4 Teclado Físico (convención MPC 60 / live performance)
    // Fila 1 (A01 - A04): Z, X, C, V
    // Fila 2 (A05 - A08): A, S, D, F
    // Fila 3 (A09 - A12): Q, W, E, R
    // Fila 4 (A13 - A16): 1, 2, 3, 4
    // =========================================================================
    if (ch == 'z' || ch == 'Z') { triggerPadByIndex(0);  return true; }
    if (ch == 'x' || ch == 'X') { triggerPadByIndex(1);  return true; }
    if (ch == 'c' || ch == 'C') { triggerPadByIndex(2);  return true; }
    if (ch == 'v' || ch == 'V') { triggerPadByIndex(3);  return true; }

    if (ch == 'a' || ch == 'A') { triggerPadByIndex(4);  return true; }
    if (ch == 's' || ch == 'S') { triggerPadByIndex(5);  return true; }
    if (ch == 'd' || ch == 'D') { triggerPadByIndex(6);  return true; }
    if (ch == 'f' || ch == 'F') { triggerPadByIndex(7);  return true; }

    if (ch == 'q' || ch == 'Q') { triggerPadByIndex(8);  return true; }
    if (ch == 'w' || ch == 'W') { triggerPadByIndex(9);  return true; }
    if (ch == 'e' || ch == 'E') { triggerPadByIndex(10); return true; }
    if (ch == 'r' || ch == 'R') { triggerPadByIndex(11); return true; }

    if (ch == '1')               { triggerPadByIndex(12); return true; }
    if (ch == '2')               { triggerPadByIndex(13); return true; }
    if (ch == '3')               { triggerPadByIndex(14); return true; }
    if (ch == '4')               { triggerPadByIndex(15); return true; }

    // Soporte Numpad
    if (kc == juce::KeyPress::numberPad1) { triggerPadByIndex(4);  return true; }
    if (kc == juce::KeyPress::numberPad2) { triggerPadByIndex(5);  return true; }
    if (kc == juce::KeyPress::numberPad3) { triggerPadByIndex(6);  return true; }
    if (kc == juce::KeyPress::numberPad4) { triggerPadByIndex(8);  return true; }
    if (kc == juce::KeyPress::numberPad5) { triggerPadByIndex(9);  return true; }
    if (kc == juce::KeyPress::numberPad6) { triggerPadByIndex(10); return true; }
    if (kc == juce::KeyPress::numberPad7) { triggerPadByIndex(12); return true; }
    if (kc == juce::KeyPress::numberPad8) { triggerPadByIndex(13); return true; }
    if (kc == juce::KeyPress::numberPad9) { triggerPadByIndex(14); return true; }

    // Barra espaciadora: toggle Play / Stop
    if (kc == juce::KeyPress::spaceKey)
    {
        if (audioEngine.isPlaying())
            audioEngine.stop();
        else
            audioEngine.play();
        return true;
    }

    return false;
}

void MainComponent::paint(juce::Graphics& g)
{
    // Fondo exterior (#7E7C74)
    g.fillAll(theme::bgExterior);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // 1. Barra superior (64 px, chasis)
    topBar.setBounds(area.removeFromTop(64));

    // 3. Barra inferior (104 px: transporte + waveform + volumen)
    waveformBar.setBounds(area.removeFromBottom(104));

    // 2. Área principal en 3 columnas con 14 px de separación
    area.reduce(14, 14);

    // Izquierda (280 px): DRUMKITS
    sidebar.setBounds(area.removeFromLeft(280));
    area.removeFromLeft(14);

    // Derecha (330 px): SAMPLES, con el panel azul
    sampleList.setBounds(area.removeFromRight(330));
    area.removeFromRight(14);

    // Centro (flexible): PAD GRID + INSPECTOR
    const int detailHeight = 74;
    padDetail.setBounds(area.removeFromBottom(detailHeight));
    padGrid.setBounds(area);
}

} // namespace drummatch
