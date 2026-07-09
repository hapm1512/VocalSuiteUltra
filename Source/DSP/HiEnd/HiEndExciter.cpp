#include "HiEndExciter.h"

void HiEndExciter::prepare(double newSampleRate, int maximumBlockSize, int channels)
{
    juce::ignoreUnused(maximumBlockSize);
    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
    lowPassState.assign(static_cast<size_t>(juce::jmax(0, channels)), 0.0f);
}

void HiEndExciter::reset()
{
    std::fill(lowPassState.begin(), lowPassState.end(), 0.0f);
}

float HiEndExciter::getFloat(juce::AudioProcessorValueTreeState& apvts,
                             const char* id,
                             float fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(id))
        return value->load();

    return fallback;
}

bool HiEndExciter::getBool(juce::AudioProcessorValueTreeState& apvts,
                           const char* id,
                           bool fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(id))
        return value->load() > 0.5f;

    return fallback;
}

float HiEndExciter::softClip(float x, float drive) noexcept
{
    const float safeDrive = juce::jlimit(1.0f, 8.0f, drive);
    const float normaliser = std::tanh(safeDrive);
    return normaliser > 0.0001f ? std::tanh(x * safeDrive) / normaliser : x;
}

float HiEndExciter::makeOnePoleCoefficient(float frequency, double sampleRate) noexcept
{
    const float safeRate = static_cast<float>(juce::jlimit(8000.0, 384000.0, sampleRate));
    const float safeFrequency = juce::jlimit(1000.0f, safeRate * 0.45f, frequency);
    return std::exp(-2.0f * juce::MathConstants<float>::pi * safeFrequency / safeRate);
}

void HiEndExciter::process(juce::AudioBuffer<float>& buffer,
                           juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBool(apvts, "HIEND_ON", true))
        return;

    if (lowPassState.size() != static_cast<size_t>(buffer.getNumChannels()))
        lowPassState.assign(static_cast<size_t>(juce::jmax(0, buffer.getNumChannels())), 0.0f);

    const float frequency = getFloat(apvts, "HIEND_FREQ", 12000.0f);
    const float amount = getFloat(apvts, "HIEND_AMOUNT", 22.0f) / 100.0f;
    const float sparkle = getFloat(apvts, "HIEND_SPARKLE", 35.0f) / 100.0f;
    const float air = getFloat(apvts, "HIEND_AIR", 45.0f) / 100.0f;
    const float mix = getFloat(apvts, "HIEND_MIX", 65.0f) / 100.0f;
    const float coefficient = makeOnePoleCoefficient(frequency, sampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = lowPassState[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            state = coefficient * state + (1.0f - coefficient) * dry;
            const float high = dry - state;
            const float harmonic = softClip((std::abs(high) * 1.8f) - high * 0.25f, 2.6f) * amount * sparkle * 0.42f;
            const float airy = softClip(high * 2.1f, 1.8f) * amount * air * 0.34f;
            const float wet = dry + harmonic + airy;
            data[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}
