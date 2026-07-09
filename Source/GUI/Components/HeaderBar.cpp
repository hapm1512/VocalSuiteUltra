#include "HeaderBar.h"

void HeaderBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(14, 9);
    auto panel = bounds.toFloat();

    g.setColour(juce::Colour(0xee0b0f17));
    g.fillRoundedRectangle(panel, 14.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(panel, 14.0f, 1.0f);

    auto logo = bounds.removeFromLeft(360).reduced(10, 0);

    auto badge = logo.removeFromLeft(48).withSizeKeepingCentre(40, 40).toFloat();
    g.setColour(juce::Colour(0xff101827));
    g.fillEllipse(badge);
    g.setColour(juce::Colour(0xff34d6ff));
    g.drawEllipse(badge, 1.4f);

    juce::Path leaf;
    leaf.startNewSubPath(badge.getCentreX() - 7.0f, badge.getCentreY() + 10.0f);
    leaf.lineTo(badge.getCentreX() + 10.0f, badge.getCentreY() - 7.0f);
    leaf.lineTo(badge.getCentreX() - 8.0f, badge.getCentreY() - 12.0f);
    leaf.closeSubPath();
    g.setColour(juce::Colour(0xffdce8ff));
    g.strokePath(leaf, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colour(0xffe8eefc));
    g.setFont(juce::FontOptions(23.0f, juce::Font::bold));
    g.drawText("VOCAL SUITE", logo.removeFromTop(28), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff7f8ca6));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("PROFESSIONAL  V1.0.0", logo, juce::Justification::centredLeft);

    auto preset = getLocalBounds().withSizeKeepingCentre(400, 42);
    g.setColour(juce::Colour(0xff090c13));
    g.fillRoundedRectangle(preset.toFloat(), 10.0f);
    g.setColour(juce::Colour(0xff4c5a78));
    g.drawRoundedRectangle(preset.toFloat(), 10.0f, 1.0f);
    g.setColour(juce::Colour(0xfff3f7ff));
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("VOCAL - STUDIO PRO", preset, juce::Justification::centred);

    auto right = getLocalBounds().removeFromRight(480).reduced(14, 16);
    drawHeaderButton(g, right.removeFromLeft(56).reduced(4, 0), "SAVE", false);
    drawHeaderButton(g, right.removeFromLeft(56).reduced(4, 0), "DEL", false);
    drawHeaderButton(g, right.removeFromLeft(48).reduced(4, 0), "A", true);
    drawHeaderButton(g, right.removeFromLeft(48).reduced(4, 0), "B", false);
    drawHeaderButton(g, right.removeFromLeft(58).reduced(4, 0), "A/B", false);
    drawHeaderButton(g, right.removeFromLeft(58).reduced(4, 0), "SET", false);
    drawHeaderButton(g, right.removeFromLeft(66).reduced(4, 0), "4X", false);
    drawHeaderButton(g, right.removeFromLeft(58).reduced(4, 0), "PWR", true);
}

void HeaderBar::drawHeaderButton(juce::Graphics& g,
                                 juce::Rectangle<int> bounds,
                                 const juce::String& text,
                                 bool active)
{
    const auto r = bounds.toFloat();

    g.setColour(active ? juce::Colour(0xff102449) : juce::Colour(0xff101522));
    g.fillRoundedRectangle(r, 8.0f);

    g.setColour(active ? juce::Colour(0xff2d8cff) : juce::Colour(0xff344058));
    g.drawRoundedRectangle(r, 8.0f, 1.1f);

    g.setColour(active ? juce::Colour(0xffdbe9ff) : juce::Colour(0xff96a1b6));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(text, bounds, juce::Justification::centred);
}
