#include "SidebarComponent.h"

namespace drummatch
{

class SidebarComponent::FolderRowComponent : public juce::Component,
                                             public juce::SettableTooltipClient
{
public:
    FolderRowComponent(const LibraryFolder& folder, SampleCatalog& cat, LibraryScanner& scan)
        : folderData(folder), catalog(cat), scanner(scan),
          scanButton("SCAN", IconType::None, theme::ButtonStyle::Cream),
          removeButton("", IconType::Cross, theme::ButtonStyle::Danger)
    {
        scanButton.onClick = [this]()
        {
            scanner.scanFolderAsync(folderData);
        };
        addAndMakeVisible(scanButton);

        removeButton.setTooltip("Remove folder from KitMatch");
        removeButton.onClick = [this]()
        {
            catalog.removeLibraryFolder(folderData.id);
        };
        addAndMakeVisible(removeButton);

        setTooltip(folderData.name + "\n" + folderData.directory.getFullPathName());
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        // Clic en la zona de checkbox cuadrada estilo LCD
        if (checkboxArea.contains(e.getPosition()))
        {
            folderData.isEnabled = !folderData.isEnabled;
            catalog.setFolderEnabled(folderData.id, folderData.isEnabled);
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Tarjeta crema secundaria (#D3CFBC)
        g.setColour(theme::panelSecondary);
        g.fillRoundedRectangle(bounds, 3.0f);

        g.setColour(theme::textSecondary.withAlpha(0.35f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        // Checkbox cuadrado estilo LCD verde
        checkboxArea = juce::Rectangle<int>(8, (getHeight() - 16) / 2, 16, 16);
        g.setColour(theme::lcdFrame);
        g.drawRect(checkboxArea.toFloat(), 1.0f);

        if (folderData.isEnabled)
        {
            g.setColour(theme::lcdBg);
            g.fillRect(checkboxArea.reduced(2));

            // Marca de verificación oscura
            g.setColour(theme::lcdText);
            juce::Path check;
            check.startNewSubPath(checkboxArea.getX() + 3.0f, checkboxArea.getY() + 7.5f);
            check.lineTo(checkboxArea.getX() + 6.5f, checkboxArea.getY() + 11.5f);
            check.lineTo(checkboxArea.getX() + 12.5f, checkboxArea.getY() + 3.5f);
            g.strokePath(check, juce::PathStrokeType(1.6f));
        }
        else
        {
            g.setColour(theme::chassis.darker(0.15f));
            g.fillRect(checkboxArea.reduced(2));
        }

        // Zona de texto: amplio espacio para el nombre del drumkit
        const int textX = 30;
        const int buttonMargin = 86; // Espacio para SCAN (50px) y moreButton (24px)
        const int textW = juce::jmax(50, getWidth() - textX - buttonMargin);

        // Nombre del kit en negrita con ajuste multilínea (hasta 2 líneas sin truncar)
        const auto nameArea = juce::Rectangle<int>(textX, 6, textW, 28);
        g.setColour(theme::textMain);
        g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::bold)));
        g.drawFittedText(folderData.name, nameArea, juce::Justification::topLeft, 2, 0.95f);

        // Subtítulo en tipografía mono: "N samples · indexed" (visible al 100%)
        const auto subArea = juce::Rectangle<int>(textX, 35, textW, 16);
        g.setColour(theme::textSecondary);
        g.setFont(theme::getMonoFont(9.5f));
        g.drawText(juce::String(folderData.sampleCount) + " samples - indexed", subArea, juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        const int btnH = 26;
        const int btnY = (getHeight() - btnH) / 2;
        const int removeW = 24;
        const int scanW = 50;
        const int rightMargin = 6;
        const int gap = 4;

        removeButton.setBounds(getWidth() - rightMargin - removeW, btnY, removeW, btnH);
        scanButton.setBounds(getWidth() - rightMargin - removeW - gap - scanW, btnY, scanW, btnH);
    }

private:
    LibraryFolder folderData;
    SampleCatalog& catalog;
    LibraryScanner& scanner;

    IconButton scanButton;
    IconButton removeButton;
    juce::Rectangle<int> checkboxArea;
};

// ==============================================================================

SidebarComponent::SidebarComponent(SampleCatalog& cat, LibraryScanner& scan)
    : catalog(cat), scanner(scan)
{
    catalog.addListener(this);

    addFolderButton.onClick = [this]() { addFolderClicked(); };
    addAndMakeVisible(addFolderButton);

    statusLabel.setText("READY", juce::dontSendNotification);
    statusLabel.setFont(theme::getMonoFont(10.0f));
    statusLabel.setColour(juce::Label::textColourId, theme::textSecondary);
    addAndMakeVisible(statusLabel);

    folderListContainer = std::make_unique<juce::Component>();
    viewport.setViewedComponent(folderListContainer.get(), false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    updateFolderListUI();
}

SidebarComponent::~SidebarComponent()
{
    catalog.removeListener(this);
}

bool SidebarComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        if (juce::File(f).isDirectory())
            return true;
    }
    return false;
}

void SidebarComponent::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& path : files)
    {
        juce::File dir(path);
        if (dir.isDirectory())
        {
            if (catalog.addLibraryFolder(dir))
            {
                const auto folders = catalog.getLibraryFolders();
                for (const auto& f : folders)
                {
                    if (f.directory == dir)
                    {
                        scanner.scanFolderAsync(f);
                        break;
                    }
                }
            }
        }
    }
}

void SidebarComponent::addFolderClicked()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Drumkit Folder",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*",
        true
    );

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        const auto result = fc.getResult();
        if (result.isDirectory())
        {
            if (catalog.addLibraryFolder(result))
            {
                const auto folders = catalog.getLibraryFolders();
                for (const auto& f : folders)
                {
                    if (f.directory == result)
                    {
                        scanner.scanFolderAsync(f);
                        break;
                    }
                }
            }
        }
    });
}

void SidebarComponent::updateFolderListUI()
{
    folderRows.clear();
    const auto folders = catalog.getLibraryFolders();

    const int rowHeight = 56;
    const int gap = 6;
    const int totalHeight = static_cast<int>(folders.size()) * (rowHeight + gap);

    folderListContainer->setBounds(0, 0, viewport.getWidth() > 0 ? viewport.getWidth() - 8 : 240, totalHeight);

    int y = 0;
    for (const auto& f : folders)
    {
        auto row = std::make_unique<FolderRowComponent>(f, catalog, scanner);
        row->setBounds(0, y, folderListContainer->getWidth(), rowHeight);
        folderListContainer->addAndMakeVisible(*row);
        folderRows.push_back(std::move(row));
        y += rowHeight + gap;
    }
}

void SidebarComponent::librariesChanged()
{
    updateFolderListUI();
    statusLabel.setText("INDEXED: " + juce::String(catalog.getTotalSampleCount()) + " SAMPLES", juce::dontSendNotification);
}

void SidebarComponent::scanProgress(const juce::String& folderName, float progress, int foundCount)
{
    currentProgress = progress;
    statusLabel.setText("SCANNING " + folderName + ": " + juce::String(foundCount), juce::dontSendNotification);
    repaint();
}

void SidebarComponent::scanCompleted(const juce::String& folderName, int totalFound)
{
    currentProgress = 0.0;
    statusLabel.setText(folderName.toUpperCase() + ": " + juce::String(totalFound) + " SAMPLES", juce::dontSendNotification);
    repaint();
}

void SidebarComponent::paint(juce::Graphics& g)
{
    // Placa gris verdosa (#A3A69B) con marco biselado
    g.fillAll(theme::panelStandard);

    g.setColour(theme::chassis.brighter(0.1f));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);

    // Serigrafía "DRUMKITS"
    g.setColour(theme::textSecondary);
    g.setFont(theme::getSerigraphyFont(11.0f));
    g.drawText("DRUMKITS", 14, 10, 160, 20, juce::Justification::centredLeft, false);

    // Barra de progreso fina estilo LCD mientras escanea
    if (currentProgress > 0.0)
    {
        auto progArea = juce::Rectangle<float>(14.0f, 68.0f, static_cast<float>(getWidth() - 28), 5.0f);
        theme::drawLCDBox(g, progArea, 1.0f);
        g.setColour(theme::lcdText);
        g.fillRect(progArea.reduced(1.0f).withWidth(progArea.reduced(1.0f).getWidth() * static_cast<float>(currentProgress)));
    }

    // Zona de Drag & Drop con borde punteado "DROP A FOLDER HERE"
    if (!dropZoneArea.isEmpty())
    {
        const auto r = dropZoneArea.toFloat().reduced(1.0f);

        g.setColour(theme::panelSecondary.withAlpha(0.4f));
        g.fillRoundedRectangle(r, 3.0f);

        g.setColour(theme::textSecondary.withAlpha(0.6f));
        const float dashLengths[2] = { 4.0f, 4.0f };
        g.drawDashedLine(juce::Line<float>(r.getX(), r.getY(), r.getRight(), r.getY()), dashLengths, 2, 1.0f);
        g.drawDashedLine(juce::Line<float>(r.getRight(), r.getY(), r.getRight(), r.getBottom()), dashLengths, 2, 1.0f);
        g.drawDashedLine(juce::Line<float>(r.getRight(), r.getBottom(), r.getX(), r.getBottom()), dashLengths, 2, 1.0f);
        g.drawDashedLine(juce::Line<float>(r.getX(), r.getBottom(), r.getX(), r.getY()), dashLengths, 2, 1.0f);

        g.setFont(theme::getSerigraphyFont(9.5f));
        g.drawFittedText("DROP A FOLDER HERE\nto add another kit", dropZoneArea, juce::Justification::centred, 2);
    }
}

void SidebarComponent::resized()
{
    auto area = getLocalBounds().reduced(12, 10);
    area.removeFromTop(24); // Título serigrafía

    addFolderButton.setBounds(area.removeFromTop(34));
    area.removeFromTop(10);

    statusLabel.setBounds(area.removeFromTop(18));
    area.removeFromTop(8);

    // Zona de drop en la parte inferior (60px)
    dropZoneArea = area.removeFromBottom(60);
    area.removeFromBottom(10);

    viewport.setBounds(area);
    if (folderListContainer != nullptr)
        folderListContainer->setSize(viewport.getWidth() - 8, folderListContainer->getHeight());
}

} // namespace drummatch
