#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class HeaderBar final : public juce::Component
{
public:
    HeaderBar() = default;

    void paint(juce::Graphics& g) override;

private:
    static void drawHeaderButton(juce::Graphics& g,
                                 juce::Rectangle<int> bounds,
                                 const juce::String& text,
                                 bool active);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};
