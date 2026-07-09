#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class CorrelationMeter final : public juce::Component
{
public:
    CorrelationMeter() = default;
    ~CorrelationMeter() override = default;

    void paint(juce::Graphics& g) override;

    void setCorrelation(float newCorrelation,
                        float newWidth) noexcept;

private:
    float correlation = 1.0f;
    float width = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CorrelationMeter)
};