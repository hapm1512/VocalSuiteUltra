#include "LevelMeter.h"

LevelMeter::LevelMeter()
{
    startTimerHz(30);
}

void LevelMeter::setLevel(float newPeak, float newRms)
{
    peak = juce::jlimit(0.0f, 1.0f, newPeak);
    rms  = juce::jlimit(0.0f, 1.0f, newRms);
    hold = juce::jmax(hold, peak);
}

void LevelMeter::timerCallback()
{
    hold *= 0.985f;
    repaint();
}

void LevelMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(6.0f);

    g.setColour(juce::Colour(0xff070a10));
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(juce::Colour(0xff253044));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto meter = bounds.reduced(8.0f, 10.0f);
    const auto channelWidth = meter.getWidth() * 0.45f;
    const auto left = meter.removeFromLeft(channelWidth);
    const auto right = meter.removeFromRight(channelWidth);

    auto drawBar = [&g] (juce::Rectangle<float> barBounds, float value)
    {
        auto filled = barBounds;
        filled.setTop(barBounds.getBottom() - barBounds.getHeight() * value);

        juce::ColourGradient grad(juce::Colour(0xff35ff5c), barBounds.getBottomLeft(),
                                  juce::Colour(0xffffd92e), barBounds.getCentre(), false);
        grad.addColour(0.82, juce::Colour(0xffff4a2f));

        g.setGradientFill(grad);
        g.fillRoundedRectangle(filled, 3.0f);

        g.setColour(juce::Colour(0x6634d6ff));
        g.drawRoundedRectangle(barBounds, 3.0f, 1.0f);
    };

    drawBar(left, peak);
    drawBar(right, rms);

    const auto holdY = bounds.getBottom() - bounds.getHeight() * hold;

    g.setColour(juce::Colour(0xffffffff));
    g.drawLine(bounds.getX() + 8.0f, holdY, bounds.getRight() - 8.0f, holdY, 1.5f);

    g.setColour(juce::Colour(0xff9aa7bd));
    g.setFont(juce::FontOptions(10.0f));

    constexpr int labels[] = { 0, -6, -12, -18, -24, -36, -48, -60 };

    for (auto db : labels)
    {
        const auto normalised = juce::jmap(static_cast<float>(db), -60.0f, 0.0f, 0.0f, 1.0f);
        const auto y = juce::roundToInt(bounds.getBottom() - bounds.getHeight() * normalised) - 6;
        g.drawText(juce::String(db), 2, y, 32, 12, juce::Justification::left);
    }
}
