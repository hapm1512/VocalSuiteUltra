#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
{
    displayMagnitudes.fill(-100.0f);
    peakHold.fill(-100.0f);
    startTimerHz(60);
}

void SpectrumAnalyzer::setSampleProvider(SampleProvider provider)
{
    sampleProvider = std::move(provider);
}

void SpectrumAnalyzer::timerCallback()
{
    updateSpectrum();
    repaint();
}

void SpectrumAnalyzer::updateSpectrum()
{
    bool received = false;

    if (sampleProvider)
        received = sampleProvider(audioSnapshot);

    if (! received)
    {
        for (auto& value : displayMagnitudes)
            value = juce::jmax(-100.0f, value - 1.0f);

        for (auto& value : peakHold)
            value = juce::jmax(-100.0f, value - 0.35f);

        hasSignal = false;
        return;
    }

    hasSignal = true;
    fftData.fill(0.0f);

    for (int i = 0; i < fftSize; ++i)
        fftData[static_cast<size_t>(i)] = audioSnapshot[static_cast<size_t>(i)];

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    constexpr float minDb = -100.0f;
    constexpr float maxDb = 0.0f;
    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 20000.0f;
    constexpr float assumedSampleRate = 48000.0f;
    constexpr float binHz = assumedSampleRate / static_cast<float>(fftSize);

    for (size_t i = 0; i < displayMagnitudes.size(); ++i)
    {
        const float p = static_cast<float>(i) / static_cast<float>(displayMagnitudes.size() - 1);
        const float frequency = minFreq * std::pow(maxFreq / minFreq, p);
        const float bin = frequency / binHz;
        const int index = juce::jlimit(1, (fftSize / 2) - 2, juce::roundToInt(bin));

        const float mag0 = fftData[static_cast<size_t>(index - 1)];
        const float mag1 = fftData[static_cast<size_t>(index)];
        const float mag2 = fftData[static_cast<size_t>(index + 1)];
        const float magnitude = (mag0 * 0.25f + mag1 * 0.5f + mag2 * 0.25f) / static_cast<float>(fftSize);

        const float db = juce::jlimit(minDb, maxDb, juce::Decibels::gainToDecibels(magnitude * 10.0f + 0.000001f));
        displayMagnitudes[i] = displayMagnitudes[i] * 0.72f + db * 0.28f;
        peakHold[i] = juce::jmax(peakHold[i] - 0.18f, displayMagnitudes[i]);
    }
}

float SpectrumAnalyzer::frequencyToX(float frequency, juce::Rectangle<float> graph) const noexcept
{
    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 20000.0f;
    const float norm = std::log(juce::jlimit(minFreq, maxFreq, frequency) / minFreq) / std::log(maxFreq / minFreq);
    return graph.getX() + graph.getWidth() * norm;
}

float SpectrumAnalyzer::magnitudeToY(float magnitudeDb, juce::Rectangle<float> graph) const noexcept
{
    constexpr float minDb = -100.0f;
    constexpr float maxDb = 0.0f;
    const float norm = juce::jmap(juce::jlimit(minDb, maxDb, magnitudeDb), minDb, maxDb, 0.0f, 1.0f);
    return graph.getBottom() - graph.getHeight() * norm;
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(6.0f);

    g.setColour(juce::Colour(0xff070a10));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(juce::Colour(0xff263044));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    const auto graph = bounds.reduced(16.0f, 18.0f);

    g.setColour(juce::Colour(0x331f8cff));

    const float frequencies[] = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    for (const auto frequency : frequencies)
    {
        const auto x = juce::roundToInt(frequencyToX(frequency, graph));
        g.drawVerticalLine(x, graph.getY(), graph.getBottom());
    }

    for (float db = -100.0f; db <= 0.0f; db += 20.0f)
    {
        const auto y = juce::roundToInt(magnitudeToY(db, graph));
        g.drawHorizontalLine(y, graph.getX(), graph.getRight());
    }

    spectrumPath.clear();
    peakPath.clear();

    for (size_t i = 0; i < displayMagnitudes.size(); ++i)
    {
        const float p = static_cast<float>(i) / static_cast<float>(displayMagnitudes.size() - 1);
        const float frequency = 20.0f * std::pow(1000.0f, p);
        const float x = frequencyToX(frequency, graph);
        const float y = magnitudeToY(displayMagnitudes[i], graph);
        const float peakY = magnitudeToY(peakHold[i], graph);

        if (i == 0)
        {
            spectrumPath.startNewSubPath(x, y);
            peakPath.startNewSubPath(x, peakY);
        }
        else
        {
            spectrumPath.lineTo(x, y);
            peakPath.lineTo(x, peakY);
        }
    }

    juce::Path fill = spectrumPath;
    fill.lineTo(graph.getRight(), graph.getBottom());
    fill.lineTo(graph.getX(), graph.getBottom());
    fill.closeSubPath();

    g.setColour(juce::Colour(0xffbd5cff).withAlpha(hasSignal ? 0.30f : 0.12f));
    g.fillPath(fill);

    g.setColour(juce::Colour(0xffd36bff));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.7f));

    g.setColour(juce::Colour(0x99ffffff));
    g.strokePath(peakPath, juce::PathStrokeType(0.8f));

    g.setColour(juce::Colour(0xff8e9ab0));
    g.setFont(juce::FontOptions(10.0f));

    const auto labelY = juce::roundToInt(graph.getBottom() + 2.0f);
    auto drawLabel = [&g, labelY] (const juce::String& text, float x, juce::Justification justification)
    {
        g.drawText(text, juce::roundToInt(x), labelY, 42, 14, justification);
    };

    drawLabel("20", frequencyToX(20.0f, graph), juce::Justification::left);
    drawLabel("100", frequencyToX(100.0f, graph) - 20.0f, juce::Justification::centred);
    drawLabel("1k", frequencyToX(1000.0f, graph) - 20.0f, juce::Justification::centred);
    drawLabel("10k", frequencyToX(10000.0f, graph) - 20.0f, juce::Justification::centred);
    drawLabel("20k", frequencyToX(20000.0f, graph) - 42.0f, juce::Justification::right);
}
