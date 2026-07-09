#include "LedButton.h"

LedButton::LedButton(const juce::String& text)
    : juce::Button(text)
{
    setClickingTogglesState(true);
    setToggleState(true, juce::dontSendNotification);
}

void LedButton::paintButton(juce::Graphics& g,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);

    const bool on = getToggleState();

    auto base = on ? juce::Colour(0xff123d24)
                   : juce::Colour(0xff2a1720);

    auto edge = on ? juce::Colour(0xff43ff88)
                   : juce::Colour(0xffff5577);

    if (shouldDrawButtonAsHighlighted)
        base = base.brighter(0.25f);

    if (shouldDrawButtonAsDown)
        base = base.darker(0.25f);

    g.setColour(base);
    g.fillRoundedRectangle(r, 6.0f);

    g.setColour(edge);
    g.drawRoundedRectangle(r, 6.0f, 1.2f);

    g.setColour(on ? juce::Colour(0xffcaffda)
                   : juce::Colour(0xffffc0c9));

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(on ? "ON" : "OFF", getLocalBounds(), juce::Justification::centred);
}
