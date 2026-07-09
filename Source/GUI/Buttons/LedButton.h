#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class LedButton final : public juce::Button
{
public:
    explicit LedButton(const juce::String& text);

    void paintButton(juce::Graphics& g,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LedButton)
};
