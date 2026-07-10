#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class HeaderBar final : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void()> savePreset;
        std::function<void()> deletePreset;
        std::function<void()> selectA;
        std::function<void()> selectB;
        std::function<void()> toggleAB;
        std::function<void(juce::Component*)> openSettings;
        std::function<void()> toggleOversampling;
        std::function<void()> togglePower;
    };

    HeaderBar();

    void setCallbacks(Callbacks newCallbacks);
    void setABState(bool useA);
    void setOversamplingActive(bool active);
    void setPowerActive(bool active);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Callbacks callbacks;

    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DEL" };
    juce::TextButton aButton { "A" };
    juce::TextButton bButton { "B" };
    juce::TextButton compareButton { "A/B" };
    juce::TextButton settingsButton { "SET" };
    juce::TextButton oversamplingButton { "4X" };
    juce::TextButton powerButton { "PWR" };

    bool aIsActive = true;
    bool oversamplingIsActive = false;
    bool powerIsActive = true;

    void configureButton(juce::TextButton& button);
    void refreshButtonColours();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};
