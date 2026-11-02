#include <juce_gui_basics/juce_gui_basics.h>
#include "UI/MainComponent.h"

namespace drummatch
{

class DrumMatchApplication : public juce::JUCEApplication
{
public:
    DrumMatchApplication() = default;

    const juce::String getApplicationName() override       { return "KitMatch"; }
    const juce::String getApplicationVersion() override    { return "1.0.0-beta.1"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name,
                             juce::Colour(0xff7E7C74),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

            setResizable(true, true);
            setResizeLimits(960, 600, 3840, 2160);
            centreWithSize(getWidth(), getHeight());

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace drummatch

START_JUCE_APPLICATION(drummatch::DrumMatchApplication)

