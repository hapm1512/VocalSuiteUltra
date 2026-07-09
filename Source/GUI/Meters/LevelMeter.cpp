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
    auto r = getLocalBounds().toFloat().reduced(6.0f);

    g.setColour(juce::Colour(0xff070a10));
    g.fillRoundedRectangle(r, 6.0f);

    g.setColour(juce::Colour(0xff253044));
    g.drawRoundedRectangle(r, 6.0f, 1.0f);

    auto meter = r.reduced(8.0f, 10.0f);
    auto left  = meter.removeFromLeft(meter.getWidth() * 0.45f);
    auto right = meter.removeFromRight(meter.getWidth());

    auto drawBar = [&](juce::Rectangle<float> b, float v)
    {
        auto filled = b;
        filled.setTop(b.getBottom() - b.getHeight() * v);

        juce::ColourGradient grad(
            juce::Colour(0xff35ff5c), b.getBottomLeft(),
            juce::Colour(0xffffd92e), b.getCentre(),
            false);

        grad.addColour(0.82, juce::Colour(0xffff4a2f));

        g.setGradientFill(grad);
        g.fillRoundedRectangle(filled, 3.0f);

        g.setColour(juce::Colour(0x6634d6ff));
        g.drawRoundedRectangle(b, 3.0f, 1.0f);
    };

    drawBar(left, peak);
    drawBar(right, rms);

    const float y = r.getBottom() - r.getHeight() * hold;

    g.setColour(juce::Colour(0xffffffff));
    g.drawLine(r.getX() + 8.0f, y, r.getRight() - 8.0f, y, 1.5f);

    g.setColour(juce::Colour(0xff9aa7bd));
    g.setFont(juce::FontOptions(10.0f));

    const int labels[] = { 0, -6, -12, -18, -24, -36, -48, -60 };

    for (auto db : labels)
    {
        float norm = juce::jmap((float)db, -60.0f, 0.0f, 0.0f, 1.0f);
        float yy = r.getBottom() - r.getHeight() * norm;
        g.drawText(juce::String(db), 2, (int)yy - 6, 32, 12, juce::Justification::left);
    }
}
