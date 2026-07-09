#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class CorrelationMeter final : public juce::Component
{
public:
    void paint(juce::Graphics& g) override;
};
