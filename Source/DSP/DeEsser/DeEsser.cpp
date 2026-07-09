#include "DeEsser.h"

void DeEsser::prepare(double newSampleRate, int maxBlockSize, int numChannels)
{
    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, maxBlockSize));
    spec.numChannels = 1;

    states.resize(static_cast<size_t>(juce::jmax(1, numChannels)));
    updateFilter(6500.0f, 4.0f);
    reset();
}

void DeEsser::reset()
{
    gainReductionDb = 0.0f;

    for (auto& state : states)
    {
        state.envelope = 0.0f;
        state.gain = 1.0f;
        state.band.reset();
    }
}

void DeEsser::process(juce::AudioBuffer<float>& buffer,
                      juce::AudioProcessorValueTreeState& apvts)
{
    if (const auto* on = apvts.getRawParameterValue("DEESSER_ON"))
        if (on->load() <= 0.5f)
            return;

    auto getFloat = [&apvts](const char* id, float fallback)
    {
        if (const auto* value = apvts.getRawParameterValue(id))
            return value->load();
        return fallback;
    };

    auto getBool = [&apvts](const char* id, bool fallback)
    {
        if (const auto* value = apvts.getRawParameterValue(id))
            return value->load() > 0.5f;
        return fallback;
    };

    auto getChoice = [&apvts](const char* id, int fallback)
    {
        if (const auto* value = apvts.getRawParameterValue(id))
            return static_cast<int>(std::round(value->load()));
        return fallback;
    };

    const int mode = getChoice("DEESSER_MODE", 1);
    const bool listen = getBool("DEESSER_LISTEN", false);
    const float frequency = juce::jlimit(4000.0f, 10000.0f, getFloat("DEESSER_FREQ", 6500.0f));
    const float thresholdDb = juce::jlimit(-60.0f, 0.0f, getFloat("DEESSER_THRESH", -28.0f));
    const float rangeDb = juce::jlimit(0.0f, 24.0f, getFloat("DEESSER_REDUCE", 7.0f));
    const float amount = juce::jlimit(0.0f, 1.0f, getFloat("DEESSER_AMOUNT", 70.0f) / 100.0f);
    const float attack = coeff(getFloat("DEESSER_ATTACK", 2.5f), sampleRate);
    const float release = coeff(getFloat("DEESSER_RELEASE", 85.0f), sampleRate);
    const float q = mode == 0 ? 0.8f : 5.25f;

    updateFilter(frequency, q);
    gainReductionDb = 0.0f;

    const int channels = juce::jmin(buffer.getNumChannels(), static_cast<int>(states.size()));

    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = states[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float sibilance = state.band.processSample(dry);
            const float detector = std::abs(sibilance);
            const float envCoef = detector > state.envelope ? attack : release;
            state.envelope = envCoef * state.envelope + (1.0f - envCoef) * detector;

            const float overDb = safeDb(state.envelope) - thresholdDb;
            const float reductionDb = juce::jlimit(0.0f, rangeDb, juce::jmax(0.0f, overDb) * 0.85f) * amount;
            const float target = juce::Decibels::decibelsToGain(-reductionDb);
            const float gainCoef = target < state.gain ? attack : release;
            state.gain = gainCoef * state.gain + (1.0f - gainCoef) * target;

            const float reducedBand = sibilance * state.gain;
            const float split = dry + (reducedBand - sibilance);
            const float wide = dry * (0.72f + 0.28f * state.gain);

            data[i] = listen ? reducedBand : (mode == 0 ? wide : split);
            gainReductionDb = juce::jmax(gainReductionDb, reductionDb);
        }
    }
}

float DeEsser::coeff(float timeMs, double sr) noexcept
{
    const float safeTime = juce::jmax(0.1f, timeMs) * 0.001f;
    return std::exp(-1.0f / (safeTime * static_cast<float>(sr)));
}

float DeEsser::safeDb(float value) noexcept
{
    return juce::Decibels::gainToDecibels(juce::jmax(value, 0.000001f));
}

void DeEsser::updateFilter(float frequency, float q)
{
    const auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, frequency, q);

    for (auto& state : states)
        state.band.coefficients = coeffs;
}
