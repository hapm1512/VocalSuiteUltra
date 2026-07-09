#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class ProLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ProLookAndFeel();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProLookAndFeel)
};
