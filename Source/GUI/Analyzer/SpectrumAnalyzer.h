#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class SpectrumAnalyzer final : public juce::Component,
                               private juce::Timer
{
public:
    SpectrumAnalyzer();

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;

    juce::Path spectrumPath;
    float phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};
