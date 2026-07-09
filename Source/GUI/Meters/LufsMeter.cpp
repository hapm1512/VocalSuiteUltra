#include "LufsMeter.h"

void LufsMeter::setLoudness(float momentaryLufs,
                            float shortTermLufs,
                            float integratedLufs) noexcept
{
    momentary = juce::jlimit(-70.0f, 6.0f, momentaryLufs);
    shortTerm = juce::jlimit(-70.0f, 6.0f, shortTermLufs);
    integrated = juce::jlimit(-70.0f, 6.0f, integratedLufs);
    repaint();
}

void LufsMeter::paint(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced(4.0f);

    g.setColour(juce::Colour(0xff080b12));
    g.fillRoundedRectangle(r, 8.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);

    g.setColour(juce::Colour(0xff9aa7bd));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("LOUDNESS", getLocalBounds().withHeight(24), juce::Justification::centred);

    g.setColour(juce::Colour(0xfff3f7ff));
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText(juce::String(momentary, 1), getLocalBounds().withTrimmedTop(28).withHeight(32),
               juce::Justification::centred);

    g.setColour(juce::Colour(0xff5ca8ff));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("M  " + juce::String(momentary, 1), getLocalBounds().withTrimmedTop(64).withHeight(18),
               juce::Justification::centred);

    g.setColour(juce::Colour(0xff9aa7bd));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("S  " + juce::String(shortTerm, 1), getLocalBounds().withTrimmedTop(84).withHeight(16),
               juce::Justification::centred);
    g.drawText("I  " + juce::String(integrated, 1), getLocalBounds().withTrimmedTop(100).withHeight(16),
               juce::Justification::centred);
}
