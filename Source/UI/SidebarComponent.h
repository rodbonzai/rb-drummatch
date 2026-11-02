#pragma once

#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Library/SampleCatalog.h"
#include "../Library/LibraryScanner.h"
#include "MPC60Theme.h"
#include "IconButton.h"

namespace drummatch
{

class SidebarComponent : public juce::Component,
                         public juce::FileDragAndDropTarget,
                         public SampleCatalog::Listener
{
public:
    SidebarComponent(SampleCatalog& catalog, LibraryScanner& scanner);
    ~SidebarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // SampleCatalog::Listener callbacks
    void librariesChanged() override;
    void scanProgress(const juce::String& folderName, float progress, int foundCount) override;
    void scanCompleted(const juce::String& folderName, int totalFound) override;

private:
    void addFolderClicked();
    void updateFolderListUI();

    SampleCatalog& catalog;
    LibraryScanner& scanner;

    IconButton addFolderButton { "ADD FOLDER", IconType::Folder, theme::ButtonStyle::Cream };
    juce::Label statusLabel;
    double currentProgress = 0.0;

    class FolderRowComponent;
    juce::Viewport viewport;
    std::unique_ptr<juce::Component> folderListContainer;
    std::vector<std::unique_ptr<FolderRowComponent>> folderRows;

    juce::Rectangle<int> dropZoneArea;
    std::unique_ptr<juce::FileChooser> fileChooser;
};

} // namespace drummatch
