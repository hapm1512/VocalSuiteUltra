#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class ProKnob final : public juce::Slider
{
public:
    ProKnob();

    void setDragSensitivity(int pixelsForFullRange);
    void paint(juce::Graphics& g) override;

private:
    juce::Colour ringColour { 0xff34d6ff };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProKnob)
};
