#include "ProLookAndFeel.h"

ProLookAndFeel::ProLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce8ff));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
}
