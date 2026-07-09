#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "LedButton.h"
#include "ProKnob.h"

class ModulePanel final : public juce::Component
{
public:
    ModulePanel();

    void configure(const juce::String& newTitle,
                   const juce::StringArray& knobNames,
                   juce::Colour newAccent,
                   const juce::String& newModeText = {});

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::String title;
    juce::String modeText;
    juce::Colour accent { 0xff34d6ff };

    LedButton powerButton { "ON" };
    juce::OwnedArray<ProKnob> knobs;
    juce::StringArray labels;

    void drawMiniDisplay(juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePanel)
};
