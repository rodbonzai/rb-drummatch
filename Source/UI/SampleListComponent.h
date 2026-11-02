#pragma once

#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Library/SampleCatalog.h"
#include "../Audio/AudioEngine.h"
#include "../Domain/Sample.h"
#include "IconButton.h"
#include "MPC60Theme.h"

namespace drummatch
{

enum class CategoryFilter
{
    All,
    Kick,
    Snare,
    Hat,
    Perc
};

class SampleListComponent : public juce::Component,
                            public juce::TableListBoxModel,
                            public SampleCatalog::Listener
{
public:
    SampleListComponent(SampleCatalog& catalog, AudioEngine& audioEngine);
    ~SampleListComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // juce::TableListBoxModel callbacks
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;
    void selectedRowsChanged(int lastRowSelected) override;

    // SampleCatalog::Listener callbacks
    void samplesUpdated() override;
    void referenceSampleChanged(const std::optional<Sample>& ref) override;

    std::function<void(const Sample&)> onReferenceSampleSelected;
    std::function<void(const Sample&)> onSamplePlayed;

    bool hasSearchFocus() const { return searchBox.hasKeyboardFocus(true); }

private:
    void refreshData();
    void playSelectedSample();
    void setSelectionAsReference();
    void handleTableMouseDrag(const juce::MouseEvent& e);
    void drawMiniWaveform(juce::Graphics& g, const juce::Rectangle<float>& bounds, const Sample& sample);
    static juce::String getCategoryShortBadge(InstrumentCategory cat);

    SampleCatalog& catalog;
    AudioEngine& audioEngine;

    std::vector<Sample> displaySamples;
    int selectedRow = -1;
    CategoryFilter currentFilter = CategoryFilter::All;

    juce::TextEditor searchBox;
    juce::Rectangle<int> searchBoxLcdArea;

    class ChipButton : public juce::Button
    {
    public:
        ChipButton(const juce::String& text, CategoryFilter filter)
            : juce::Button(text), filterType(filter) {}

        CategoryFilter getFilterType() const { return filterType; }
        bool isActive() const { return active; }
        void setActive(bool shouldBeActive)
        {
            active = shouldBeActive;
            repaint();
        }

        void paintButton(juce::Graphics& g, bool isHighlighted, bool isDown) override;

    private:
        CategoryFilter filterType;
        bool active = false;
    };

    std::vector<std::unique_ptr<ChipButton>> filterChips;

    class DraggableTable : public juce::TableListBox
    {
    public:
        std::function<void(const juce::MouseEvent&)> onMouseDragCallback;
        void mouseDrag(const juce::MouseEvent& e) override
        {
            juce::TableListBox::mouseDrag(e);
            if (onMouseDragCallback)
                onMouseDragCallback(e);
        }
    };

    DraggableTable table;

    IconButton playButton   { "PLAY", IconType::Play, theme::ButtonStyle::Cream };
    IconButton stopButton   { "STOP", IconType::Stop, theme::ButtonStyle::Cream };
    IconButton setRefButton { "SET AS REF", IconType::None, theme::ButtonStyle::Primary };
};

} // namespace drummatch
