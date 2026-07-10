#include "SpectrumAnalyzer.h"

namespace
{
    constexpr float minFrequency = 20.0f;
    constexpr float maxFrequency = 20000.0f;
    constexpr float spectrumMinDb = -100.0f;
    constexpr float spectrumMaxDb = 0.0f;
    constexpr float eqMinDb = -18.0f;
    constexpr float eqMaxDb = 18.0f;
}

SpectrumAnalyzer::SpectrumAnalyzer()
{
    displayMagnitudes.fill(spectrumMinDb);
    peakHold.fill(spectrumMinDb);
    eqResponseDb.fill(0.0f);
    startTimerHz(60);
}

void SpectrumAnalyzer::setSampleProvider(SampleProvider provider)
{
    sampleProvider = std::move(provider);
}

void SpectrumAnalyzer::setEqParameterSource(juce::AudioProcessorValueTreeState& state,
                                            SampleRateProvider provider)
{
    parameterState = &state;
    sampleRateProvider = std::move(provider);
    updateEqResponse();
    repaint();
}

void SpectrumAnalyzer::timerCallback()
{
    updateSpectrum();
    updateEqResponse();
    repaint();
}

float SpectrumAnalyzer::readParameter(const char* parameterId, float fallback) const noexcept
{
    if (parameterState == nullptr)
        return fallback;

    if (const auto* value = parameterState->getRawParameterValue(parameterId))
        return value->load();

    return fallback;
}

bool SpectrumAnalyzer::readBoolParameter(const char* parameterId, bool fallback) const noexcept
{
    return readParameter(parameterId, fallback ? 1.0f : 0.0f) >= 0.5f;
}

void SpectrumAnalyzer::updateSpectrum()
{
    bool received = false;

    if (sampleProvider)
        received = sampleProvider(audioSnapshot);

    if (! received)
    {
        for (auto& value : displayMagnitudes)
            value = juce::jmax(spectrumMinDb, value - 1.0f);

        for (auto& value : peakHold)
            value = juce::jmax(spectrumMinDb, value - 0.35f);

        hasSignal = false;
        return;
    }

    hasSignal = true;
    fftData.fill(0.0f);

    for (int i = 0; i < fftSize; ++i)
        fftData[static_cast<size_t>(i)] = audioSnapshot[static_cast<size_t>(i)];

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const auto actualSampleRate = sampleRateProvider ? sampleRateProvider() : 48000.0;
    const float safeSampleRate = static_cast<float>(juce::jlimit(8000.0, 384000.0, actualSampleRate));
    const float binHz = safeSampleRate / static_cast<float>(fftSize);

    for (size_t i = 0; i < displayMagnitudes.size(); ++i)
    {
        const float p = static_cast<float>(i) / static_cast<float>(displayMagnitudes.size() - 1);
        const float frequency = minFrequency * std::pow(maxFrequency / minFrequency, p);
        const float bin = frequency / binHz;
        const int index = juce::jlimit(1, (fftSize / 2) - 2, juce::roundToInt(bin));

        const float mag0 = fftData[static_cast<size_t>(index - 1)];
        const float mag1 = fftData[static_cast<size_t>(index)];
        const float mag2 = fftData[static_cast<size_t>(index + 1)];
        const float magnitude = (mag0 * 0.25f + mag1 * 0.5f + mag2 * 0.25f) / static_cast<float>(fftSize);

        const float db = juce::jlimit(spectrumMinDb, spectrumMaxDb,
                                     juce::Decibels::gainToDecibels(magnitude * 10.0f + 0.000001f));
        displayMagnitudes[i] = displayMagnitudes[i] * 0.72f + db * 0.28f;
        peakHold[i] = juce::jmax(peakHold[i] - 0.18f, displayMagnitudes[i]);
    }
}

void SpectrumAnalyzer::updateEqResponse()
{
    eqEnabled = readBoolParameter("EQ_ON", true)
             || readBoolParameter("TONE_ON", true)
             || readBoolParameter("HPF_ON", true);

    if (parameterState == nullptr)
    {
        eqResponseDb.fill(0.0f);
        return;
    }

    const double requestedSampleRate = sampleRateProvider ? sampleRateProvider() : 48000.0;
    const double sampleRate = juce::jlimit(8000.0, 384000.0, requestedSampleRate > 0.0 ? requestedSampleRate : 48000.0);

    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    std::array<typename Coefficients::Ptr, 7> filters {};
    size_t filterCount = 0;

    if (readBoolParameter("HPF_ON", true))
    {
        const float frequency = juce::jlimit(20.0f, 220.0f, readParameter("HPF_FREQ", 85.0f));
        filters[filterCount++] = Coefficients::makeHighPass(sampleRate, frequency, 0.70710678f);
    }

    if (readBoolParameter("EQ_ON", true))
    {
        const float frequency = juce::jlimit(20.0f, 20000.0f, readParameter("EQ_FREQ", 320.0f));
        const float gainDb = readParameter("EQ_GAIN", -2.5f);
        const float q = juce::jlimit(0.2f, 12.0f, readParameter("EQ_Q", 4.0f));
        filters[filterCount++] = Coefficients::makePeakFilter(sampleRate, frequency, q,
                                                              juce::Decibels::decibelsToGain(gainDb));

        const float mudFrequency = readParameter("EQ_MUD_FREQ", 320.0f);
        const float mudGain = readParameter("EQ_MUD_GAIN", -2.5f);
        const float mudQ = juce::jlimit(0.4f, 10.0f, readParameter("EQ_MUD_Q", 3.5f));
        filters[filterCount++] = Coefficients::makePeakFilter(sampleRate, mudFrequency, mudQ,
                                                              juce::Decibels::decibelsToGain(mudGain));

        const float harshFrequency = readParameter("EQ_HARSH_FREQ", 3200.0f);
        const float harshGain = readParameter("EQ_HARSH_GAIN", -2.0f);
        const float harshQ = juce::jlimit(0.4f, 10.0f, readParameter("EQ_HARSH_Q", 3.2f));
        filters[filterCount++] = Coefficients::makePeakFilter(sampleRate, harshFrequency, harshQ,
                                                              juce::Decibels::decibelsToGain(harshGain));
    }

    if (readBoolParameter("TONE_ON", true))
    {
        const float lowFrequency = readParameter("TONE_LOW_FREQ", 160.0f);
        const float midFrequency = readParameter("TONE_MID_FREQ", 900.0f);
        const float highFrequency = readParameter("TONE_HIGH_FREQ", 6500.0f);
        const float airFrequency = readParameter("AIR_FREQ", 12000.0f);

        filters[filterCount++] = Coefficients::makeLowShelf(sampleRate, lowFrequency, 0.70710678f,
                                                            juce::Decibels::decibelsToGain(readParameter("TONE_LOW", 0.0f)));
        filters[filterCount++] = Coefficients::makePeakFilter(sampleRate, midFrequency, 0.85f,
                                                              juce::Decibels::decibelsToGain(readParameter("TONE_MID", -1.5f)));
        filters[filterCount++] = Coefficients::makeHighShelf(sampleRate, highFrequency, 0.70710678f,
                                                             juce::Decibels::decibelsToGain(readParameter("TONE_HIGH", 1.5f)));

        // The fixed-size array is full here; blend AIR into the visible high shelf.
        const float airGain = readParameter("TONE_AIR", 1.2f);
        const float airStart = juce::jlimit(9000.0f, 20000.0f, airFrequency);
        for (size_t i = 0; i < eqResponseDb.size(); ++i)
        {
            const float p = static_cast<float>(i) / static_cast<float>(eqResponseDb.size() - 1);
            const float frequency = minFrequency * std::pow(maxFrequency / minFrequency, p);
            const float airWeight = juce::jlimit(0.0f, 1.0f,
                                                (std::log(frequency) - std::log(airStart))
                                                / (std::log(maxFrequency) - std::log(airStart)));
            eqResponseDb[i] = airGain * airWeight;
        }
    }
    else
    {
        eqResponseDb.fill(0.0f);
    }

    for (size_t i = 0; i < eqResponseDb.size(); ++i)
    {
        const float p = static_cast<float>(i) / static_cast<float>(eqResponseDb.size() - 1);
        const double frequency = static_cast<double>(minFrequency * std::pow(maxFrequency / minFrequency, p));
        double magnitude = 1.0;

        for (size_t filterIndex = 0; filterIndex < filterCount; ++filterIndex)
        {
            if (filters[filterIndex] != nullptr)
                magnitude *= filters[filterIndex]->getMagnitudeForFrequency(frequency, sampleRate);
        }

        const float filterDb = juce::Decibels::gainToDecibels(static_cast<float>(magnitude), eqMinDb);
        eqResponseDb[i] = juce::jlimit(eqMinDb, eqMaxDb, eqResponseDb[i] + filterDb);
    }
}

float SpectrumAnalyzer::frequencyToX(float frequency, juce::Rectangle<float> graph) const noexcept
{
    const float norm = std::log(juce::jlimit(minFrequency, maxFrequency, frequency) / minFrequency)
                     / std::log(maxFrequency / minFrequency);
    return graph.getX() + graph.getWidth() * norm;
}

float SpectrumAnalyzer::magnitudeToY(float magnitudeDb, juce::Rectangle<float> graph) const noexcept
{
    const float norm = juce::jmap(juce::jlimit(spectrumMinDb, spectrumMaxDb, magnitudeDb),
                                  spectrumMinDb, spectrumMaxDb, 0.0f, 1.0f);
    return graph.getBottom() - graph.getHeight() * norm;
}

float SpectrumAnalyzer::eqGainToY(float gainDb, juce::Rectangle<float> graph) const noexcept
{
    const float norm = juce::jmap(juce::jlimit(eqMinDb, eqMaxDb, gainDb),
                                  eqMinDb, eqMaxDb, 0.0f, 1.0f);
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

    const float frequencies[] = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
                                  1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
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

    g.setColour(juce::Colour(0x355ce8ff));
    const auto zeroY = juce::roundToInt(eqGainToY(0.0f, graph));
    g.drawHorizontalLine(zeroY, graph.getX(), graph.getRight());

    spectrumPath.clear();
    peakPath.clear();
    eqPath.clear();

    for (size_t i = 0; i < displayMagnitudes.size(); ++i)
    {
        const float p = static_cast<float>(i) / static_cast<float>(displayMagnitudes.size() - 1);
        const float frequency = minFrequency * std::pow(maxFrequency / minFrequency, p);
        const float x = frequencyToX(frequency, graph);
        const float y = magnitudeToY(displayMagnitudes[i], graph);
        const float peakY = magnitudeToY(peakHold[i], graph);
        const float eqY = eqGainToY(eqResponseDb[i], graph);

        if (i == 0)
        {
            spectrumPath.startNewSubPath(x, y);
            peakPath.startNewSubPath(x, peakY);
            eqPath.startNewSubPath(x, eqY);
        }
        else
        {
            spectrumPath.lineTo(x, y);
            peakPath.lineTo(x, peakY);
            eqPath.lineTo(x, eqY);
        }
    }

    juce::Path fill = spectrumPath;
    fill.lineTo(graph.getRight(), graph.getBottom());
    fill.lineTo(graph.getX(), graph.getBottom());
    fill.closeSubPath();

    g.setColour(juce::Colour(0xffbd5cff).withAlpha(hasSignal ? 0.24f : 0.08f));
    g.fillPath(fill);

    g.setColour(juce::Colour(0xffa65cff).withAlpha(hasSignal ? 0.78f : 0.30f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.3f));

    g.setColour(juce::Colour(0x66ffffff));
    g.strokePath(peakPath, juce::PathStrokeType(0.7f));

    g.setColour(eqEnabled ? juce::Colour(0xff38e8ff) : juce::Colour(0xff52606f));
    g.strokePath(eqPath, juce::PathStrokeType(2.4f,
                 juce::PathStrokeType::curved,
                 juce::PathStrokeType::rounded));

    const struct NodeInfo
    {
        const char* frequencyId;
        const char* gainId;
        float fallbackFrequency;
        float fallbackGain;
        juce::Colour colour;
    } nodes[] =
    {
        { "EQ_FREQ",       "EQ_GAIN",       320.0f,  -2.5f, juce::Colour(0xffffd744) },
        { "EQ_MUD_FREQ",   "EQ_MUD_GAIN",   320.0f,  -2.5f, juce::Colour(0xffff9850) },
        { "EQ_HARSH_FREQ", "EQ_HARSH_GAIN", 3200.0f, -2.0f, juce::Colour(0xffff72c8) },
        { "TONE_HIGH_FREQ", "TONE_HIGH",    6500.0f,  1.5f, juce::Colour(0xff80ee66) },
        { "AIR_FREQ",       "TONE_AIR",    12000.0f,  1.2f, juce::Colour(0xff8db7ff) }
    };

    if (eqEnabled)
    {
        for (const auto& node : nodes)
        {
            const float x = frequencyToX(readParameter(node.frequencyId, node.fallbackFrequency), graph);
            const float y = eqGainToY(readParameter(node.gainId, node.fallbackGain), graph);
            g.setColour(node.colour.withAlpha(0.28f));
            g.fillEllipse(x - 7.0f, y - 7.0f, 14.0f, 14.0f);
            g.setColour(node.colour);
            g.fillEllipse(x - 3.5f, y - 3.5f, 7.0f, 7.0f);
        }
    }

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
