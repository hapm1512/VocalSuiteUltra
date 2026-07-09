#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class FooterBar final : public juce::Component
{
public:
    FooterBar() = default;

    void paint(juce::Graphics& g) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterBar)
};
