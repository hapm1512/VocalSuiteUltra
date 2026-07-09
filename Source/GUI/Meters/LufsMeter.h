#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class LufsMeter final : public juce::Component
{
public:
    LufsMeter() = default;
    ~LufsMeter() override = default;

    void paint(juce::Graphics& g) override;

    void setLoudness(float momentaryLufs,
                     float shortTermLufs,
                     float integratedLufs) noexcept;

private:
    float momentary  = -70.0f;
    float shortTerm  = -70.0f;
    float integrated = -70.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LufsMeter)
};