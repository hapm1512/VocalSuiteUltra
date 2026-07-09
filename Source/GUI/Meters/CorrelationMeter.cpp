#include "CorrelationMeter.h"

void CorrelationMeter::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(4.0f);

    g.setColour(juce::Colour(0xff080b12));
    g.fillRoundedRectangle(r, 8.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);

    g.setColour(juce::Colour(0xff9aa7bd));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("CORRELATION", getLocalBounds().withHeight(24), juce::Justification::centred);

    auto bar = getLocalBounds().reduced(20, 34).removeFromBottom(16).toFloat();

    g.setColour(juce::Colour(0xff132033));
    g.fillRoundedRectangle(bar, 4.0f);

    juce::ColourGradient grad(
        juce::Colour(0xffff3b3b), bar.getTopLeft(),
        juce::Colour(0xff35ff5c), bar.getTopRight(),
        false);

    g.setGradientFill(grad);
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * 0.82f), 4.0f);

    g.setColour(juce::Colour(0xff34d6ff));
    g.drawEllipse(r.withSizeKeepingCentre(58, 58), 1.3f);

    g.drawLine(r.getCentreX(), r.getCentreY() - 30, r.getCentreX(), r.getCentreY() + 30);
    g.drawLine(r.getCentreX() - 30, r.getCentreY(), r.getCentreX() + 30, r.getCentreY());
}
