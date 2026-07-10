#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class PresetBrowser final : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void(int)> loadFactoryPreset;
        std::function<void()> loadUserPreset;
    };

    PresetBrowser();

    void setCallbacks(Callbacks newCallbacks);
    void setFactoryPresetNames(const juce::StringArray& names);
    void setUserPresetAvailable(bool available);
    void setSelectedFactoryPreset(int index);
    void selectUserPreset();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Callbacks callbacks;
    juce::OwnedArray<juce::TextButton> factoryButtons;
    juce::TextButton userButton { "USER - CURRENT" };
    int selectedFactory = 0;
    bool userAvailable = false;
    bool userSelected = false;

    void rebuildFactoryButtons(const juce::StringArray& names);
    void refreshSelection();
    static void configurePresetButton(juce::TextButton& button);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
