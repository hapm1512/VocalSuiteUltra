#include "GlassPanel.h"

GlassPanel::GlassPanel(const juce::String& titleText)
    : title(titleText)
{
}

void GlassPanel::setTitle(const juce::String& newTitle)
{
    title = newTitle;
    repaint();
}

void GlassPanel::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(juce::Colour(0xcc0d111a));
    g.fillRoundedRectangle(r, 12.0f);

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0x4434d6ff), r.getTopLeft(),
        juce::Colour(0x11000000), r.getBottomRight(),
        false));
    g.fillRoundedRectangle(r.reduced(1.0f), 12.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 12.0f, 1.0f);

    g.setColour(juce::Colour(0xffdce8ff));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(title, getLocalBounds().withHeight(42).reduced(14, 0),
               juce::Justification::centredLeft);
}

void GlassPanel::resized()
{
}
