#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
{
    startTimerHz(30);
}

void SpectrumAnalyzer::timerCallback()
{
    phase += 0.08f;
    repaint();
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(6.0f);

    g.setColour(juce::Colour(0xff070a10));
    g.fillRoundedRectangle(r, 8.0f);

    g.setColour(juce::Colour(0xff263044));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);

    auto graph = r.reduced(16.0f, 18.0f);

    g.setColour(juce::Colour(0x331f8cff));

    for (int i = 0; i < 10; ++i)
    {
        const float x = graph.getX() + graph.getWidth() * (float)i / 9.0f;
        g.drawVerticalLine((int)x, graph.getY(), graph.getBottom());
    }

    for (int i = 0; i < 6; ++i)
    {
        const float y = graph.getY() + graph.getHeight() * (float)i / 5.0f;
        g.drawHorizontalLine((int)y, graph.getX(), graph.getRight());
    }

    spectrumPath.clear();

    for (int i = 0; i < 160; ++i)
    {
        const float p = (float)i / 159.0f;
        const float x = graph.getX() + graph.getWidth() * p;

        const float bass = std::exp(-p * 2.2f);
        const float wave = 0.08f * std::sin(phase + p * 42.0f)
                         + 0.05f * std::sin(phase * 1.7f + p * 91.0f);

        const float amp = juce::jlimit(0.05f, 0.95f, bass * 0.72f + wave + 0.18f);
        const float y = graph.getBottom() - graph.getHeight() * amp;

        if (i == 0)
            spectrumPath.startNewSubPath(x, y);
        else
            spectrumPath.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xffbd5cff).withAlpha(0.28f));

    juce::Path fill = spectrumPath;
    fill.lineTo(graph.getRight(), graph.getBottom());
    fill.lineTo(graph.getX(), graph.getBottom());
    fill.closeSubPath();
    g.fillPath(fill);

    g.setColour(juce::Colour(0xffd36bff));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.6f));

    g.setColour(juce::Colour(0xff8e9ab0));
    g.setFont(juce::FontOptions(10.0f));

    const int labelY = juce::roundToInt(graph.getBottom() + 2.0f);
    auto drawLabel = [&](const juce::String& text, float x, juce::Justification justification)
    {
        g.drawText(text, juce::roundToInt(x), labelY, 40, 14, justification);
    };

    drawLabel("20", graph.getX(), juce::Justification::left);
    drawLabel("100", graph.getX() + graph.getWidth() * 0.18f, juce::Justification::centred);
    drawLabel("1k", graph.getX() + graph.getWidth() * 0.48f, juce::Justification::centred);
    drawLabel("10k", graph.getX() + graph.getWidth() * 0.78f, juce::Justification::centred);
    drawLabel("20k", graph.getRight() - 40.0f, juce::Justification::right);
}
