#include "LufsMeter.h"

void LufsMeter::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(4.0f);

    g.setColour(juce::Colour(0xff080b12));
    g.fillRoundedRectangle(r, 8.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);

    g.setColour(juce::Colour(0xff9aa7bd));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("LOUDNESS", getLocalBounds().withHeight(24), juce::Justification::centred);

    g.setColour(juce::Colour(0xfff3f7ff));
    g.setFont(juce::FontOptions(23.0f, juce::Font::bold));
    g.drawText("-14.2", getLocalBounds().withTrimmedTop(28).withHeight(34),
               juce::Justification::centred);

    g.setColour(juce::Colour(0xff5ca8ff));
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("LUFS", getLocalBounds().withTrimmedTop(62).withHeight(22),
               juce::Justification::centred);
}
