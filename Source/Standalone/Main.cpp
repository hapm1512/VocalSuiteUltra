#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

namespace
{
class StandaloneContent final : public juce::Component
{
public:
    StandaloneContent()
        : processor (std::make_unique<VocalSuiteUltraProAudioProcessor>())
    {
        editor.reset (processor->createEditor());
        addAndMakeVisible (*editor);

        player.setProcessor (processor.get());

        auto error = deviceManager.initialise (2, 2, nullptr, true);

        // Some systems have no active input device. Keep output available
        // without showing JUCE's default warning/settings strip.
        if (error.isNotEmpty())
            error = deviceManager.initialise (0, 2, nullptr, true);

        deviceManager.addAudioCallback (&player);

        setSize (editor->getWidth(), editor->getHeight());
    }

    ~StandaloneContent() override
    {
        deviceManager.removeAudioCallback (&player);
        player.setProcessor (nullptr);
        deviceManager.closeAudioDevice();
        editor.reset();
        processor.reset();
    }

    void resized() override
    {
        if (editor != nullptr)
            editor->setBounds (getLocalBounds());
    }

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer player;
    std::unique_ptr<VocalSuiteUltraProAudioProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandaloneContent)
};

class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow()
        : juce::DocumentWindow ("VocalSuiteUltraPro",
                                juce::Colour (0xff080b12),
                                juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        setContentOwned (new StandaloneContent(), true);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

class VocalSuiteStandaloneApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "VocalSuiteUltraPro"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override              { return false; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>();
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String&) override {}

private:
    std::unique_ptr<MainWindow> mainWindow;
};
} // namespace

START_JUCE_APPLICATION (VocalSuiteStandaloneApplication)
