#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Persistence/Database.h"
#include "../Library/SampleCatalog.h"
#include "../Library/LibraryScanner.h"
#include "../Audio/AudioEngine.h"
#include "../Analysis/MatchRanking.h"
#include "TopBarComponent.h"
#include "SidebarComponent.h"
#include "PadGridComponent.h"
#include "PadDetailComponent.h"
#include "SampleListComponent.h"
#include "WaveformComponent.h"

namespace drummatch
{

class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void triggerAcousticMatching(int offset = 0);
    void selectCandidateForPad(int padIndex, int candidateOffset);
    void triggerPadByIndex(int index);
    void syncReferencePad();

    Database database;
    SampleCatalog catalog { database };
    LibraryScanner scanner { catalog };
    AudioEngine audioEngine;
    MatchRanking rankingEngine { database, audioEngine.getFormatManager() };

    TopBarComponent topBar { catalog };
    SidebarComponent sidebar { catalog, scanner };
    PadGridComponent padGrid;
    PadDetailComponent padDetail;
    SampleListComponent sampleList { catalog, audioEngine };
    WaveformComponent waveformBar { audioEngine };

    int varietyCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace drummatch
