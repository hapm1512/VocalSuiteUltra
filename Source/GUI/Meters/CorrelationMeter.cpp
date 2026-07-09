#include "CorrelationMeter.h"

void CorrelationMeter::setCorrelation(float newCorrelation, float newWidth) noexcept
{
    correlation = juce::jlimit(-1.0f, 1.0f, newCorrelation);
    width = juce::jlimit(0.0f, 200.0f, newWidth);
    repaint();
}

void CorrelationMeter::paint(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced(4.0f);

    g.setColour(juce::Colour(0xff080b12));
    g.fillRoundedRectangle(r, 8.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);

    g.setColour(juce::Colour(0xff9aa7bd));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("CORRELATION", getLocalBounds().withHeight(24), juce::Justification::centred);

    const auto bar = getLocalBounds().reduced(20, 34).removeFromBottom(16).toFloat();

    g.setColour(juce::Colour(0xff132033));
    g.fillRoundedRectangle(bar, 4.0f);

    juce::ColourGradient grad(juce::Colour(0xffff3b3b), bar.getTopLeft(),
                              juce::Colour(0xff35ff5c), bar.getTopRight(), false);

    const float normalised = (correlation + 1.0f) * 0.5f;
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * normalised), 4.0f);

    const auto scope = r.withSizeKeepingCentre(58, 58);
    g.setColour(juce::Colour(0xff34d6ff));
    g.drawEllipse(scope, 1.3f);

    g.drawLine(scope.getCentreX(), scope.getY(), scope.getCentreX(), scope.getBottom());
    g.drawLine(scope.getX(), scope.getCentreY(), scope.getRight(), scope.getCentreY());

    const float dotX = juce::jmap(correlation, -1.0f, 1.0f, scope.getX(), scope.getRight());
    const float dotY = scope.getCentreY() - juce::jlimit(-20.0f, 20.0f, (width - 100.0f) * 0.10f);

    g.setColour(juce::Colour(0xffd8fbff));
    g.fillEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);

    g.setColour(juce::Colour(0xff9aa7bd));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("CORR " + juce::String(correlation, 2), getLocalBounds().withTrimmedTop(80).withHeight(16),
               juce::Justification::centred);
    g.drawText("WIDTH " + juce::String(width, 0) + "%", getLocalBounds().withTrimmedTop(98).withHeight(16),
               juce::Justification::centred);
}
