#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class LevelMeter final : public juce::Component,
                         private juce::Timer
{
public:
    LevelMeter();

    void paint(juce::Graphics& g) override;
    void setLevel(float newPeak, float newRms);

private:
    void timerCallback() override;

    float peak = 0.45f;
    float rms  = 0.28f;
    float hold = 0.45f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
