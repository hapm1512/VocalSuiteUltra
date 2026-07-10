#include "HeaderBar.h"

void HeaderBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(14, 9);
    const auto panel = bounds.toFloat();

    g.setColour(juce::Colour(0xee0b0f17));
    g.fillRoundedRectangle(panel, 14.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(panel, 14.0f, 1.0f);

    // Brand area: use the available header width instead of a redundant centre title.
    auto brandArea = bounds.removeFromLeft(650).reduced(10, 0);
    auto badgeArea = brandArea.removeFromLeft(62);
    const auto badge = badgeArea.withSizeKeepingCentre(48, 48).toFloat();

    g.setColour(juce::Colour(0xff101827));
    g.fillEllipse(badge);

    g.setColour(juce::Colour(0xff34d6ff));
    g.drawEllipse(badge, 1.6f);

    juce::Path logoPath;
    logoPath.startNewSubPath(badge.getCentreX() - 8.0f,
                             badge.getCentreY() + 12.0f);
    logoPath.lineTo(badge.getCentreX() + 12.0f,
                    badge.getCentreY() - 9.0f);
    logoPath.lineTo(badge.getCentreX() - 10.0f,
                    badge.getCentreY() - 14.0f);
    logoPath.closeSubPath();

    g.setColour(juce::Colour(0xffdce8ff));
    g.strokePath(logoPath, juce::PathStrokeType(2.2f));

    auto textArea = brandArea.reduced(6, 0);
    auto titleArea = textArea.removeFromTop(34);

    g.setColour(juce::Colour(0xfff2f6ff));
    g.setFont(juce::FontOptions(27.0f, juce::Font::bold));
    g.drawText("VOCAL CHAIN MASTER",
               titleArea,
               juce::Justification::centredLeft,
               true);

    g.setColour(juce::Colour(0xff8d99ae));
    g.setFont(juce::FontOptions(13.5f, juce::Font::plain));
    g.drawText("Professional Edition",
               textArea,
               juce::Justification::centredLeft,
               true);

    // Right-side controls remain grouped and vertically aligned.
    auto right = getLocalBounds().removeFromRight(500).reduced(14, 15);

    drawHeaderButton(g, right.removeFromLeft(58).reduced(4, 0), "SAVE", false);
    drawHeaderButton(g, right.removeFromLeft(58).reduced(4, 0), "DEL", false);
    drawHeaderButton(g, right.removeFromLeft(50).reduced(4, 0), "A", true);
    drawHeaderButton(g, right.removeFromLeft(50).reduced(4, 0), "B", false);
    drawHeaderButton(g, right.removeFromLeft(60).reduced(4, 0), "A/B", false);
    drawHeaderButton(g, right.removeFromLeft(60).reduced(4, 0), "SET", false);
    drawHeaderButton(g, right.removeFromLeft(68).reduced(4, 0), "4X", false);
    drawHeaderButton(g, right.removeFromLeft(62).reduced(4, 0), "PWR", true);
}

void HeaderBar::drawHeaderButton(juce::Graphics& g,
                                 juce::Rectangle<int> bounds,
                                 const juce::String& text,
                                 bool active)
{
    const auto r = bounds.toFloat();

    g.setColour(active ? juce::Colour(0xff102449)
                       : juce::Colour(0xff101522));
    g.fillRoundedRectangle(r, 8.0f);

    g.setColour(active ? juce::Colour(0xff2d8cff)
                       : juce::Colour(0xff344058));
    g.drawRoundedRectangle(r, 8.0f, 1.1f);

    g.setColour(active ? juce::Colour(0xffdbe9ff)
                       : juce::Colour(0xff96a1b6));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(text, bounds, juce::Justification::centred);
}
