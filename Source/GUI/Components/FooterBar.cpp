#include "FooterBar.h"

void FooterBar::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().reduced(14, 6).toFloat();

    g.setColour(juce::Colour(0xee0a0d14));
    g.fillRoundedRectangle(r, 10.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 10.0f, 1.0f);

    auto text = getLocalBounds().reduced(28, 0);

    g.setColour(juce::Colour(0xff7f8ca6));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("QUALITY: HIGH     48 kHz     LATENCY: 0.0 ms     OVERSAMPLE: 4X",
               text.removeFromLeft(540), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff43ff88));
    g.fillEllipse((float)text.getRight() - 18.0f, (float)text.getCentreY() - 4.0f, 8.0f, 8.0f);

    g.setColour(juce::Colour(0xff7f8ca6));
    g.drawText("CPU: 0.0%     COPYRIGHT HAI PHAM", text, juce::Justification::centredRight);
}
