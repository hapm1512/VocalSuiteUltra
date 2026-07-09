#include "Saturation.h"

void Saturation::prepare(double newSampleRate, int maximumBlockSize, int channels)
{
    juce::ignoreUnused(maximumBlockSize, channels);
    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
}

void Saturation::reset()
{
}

float Saturation::getFloat(juce::AudioProcessorValueTreeState& apvts,
                           const char* id,
                           float fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(id))
        return value->load();

    return fallback;
}

bool Saturation::getBool(juce::AudioProcessorValueTreeState& apvts,
                         const char* id,
                         bool fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(id))
        return value->load() > 0.5f;

    return fallback;
}

int Saturation::getChoice(juce::AudioProcessorValueTreeState& apvts,
                          const char* id,
                          int fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(id))
        return static_cast<int>(std::round(value->load()));

    return fallback;
}

float Saturation::softClip(float x, float drive) noexcept
{
    const float safeDrive = juce::jlimit(1.0f, 14.0f, drive);
    const float normaliser = std::tanh(safeDrive);
    return normaliser > 0.0001f ? std::tanh(x * safeDrive) / normaliser : x;
}

float Saturation::tube(float x, float drive, float bias) noexcept
{
    const float biased = x + bias;
    const float shaped = softClip(biased, drive) - softClip(bias, drive);
    return juce::jlimit(-1.2f, 1.2f, shaped);
}

float Saturation::tape(float x, float drive) noexcept
{
    const float safeDrive = juce::jlimit(1.0f, 10.0f, drive);
    const float driven = x * safeDrive;
    const float cubic = driven - (0.16f * driven * driven * driven);
    return juce::jlimit(-1.2f, 1.2f, cubic / juce::jmax(1.0f, safeDrive * 0.66f));
}

void Saturation::process(juce::AudioBuffer<float>& buffer,
                         juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBool(apvts, "SATURATION_ON", true))
        return;

    const float driveAmount = getFloat(apvts, "SATURATION_DRIVE", 18.0f) / 100.0f;
    const float mix = getFloat(apvts, "SATURATION_MIX", 35.0f) / 100.0f;
    const float tubeAmount = getFloat(apvts, "SATURATION_TUBE", 55.0f) / 100.0f;
    const float tapeAmount = getFloat(apvts, "SATURATION_TAPE", 35.0f) / 100.0f;
    const float transformerAmount = getFloat(apvts, "SATURATION_TRANSFORMER", 25.0f) / 100.0f;
    const float bias = getFloat(apvts, "SATURATION_BIAS", 8.0f) * 0.0018f;
    const float output = juce::Decibels::decibelsToGain(getFloat(apvts, "SATURATION_OUTPUT", 0.0f));
    const int mode = getChoice(apvts, "SATURATION_MODE", 0);
    const float drive = 1.0f + driveAmount * 6.0f;
    const float trim = juce::Decibels::decibelsToGain(-driveAmount * 4.2f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float tubeWet = tube(dry, drive * 1.12f, bias) * tubeAmount;
            const float tapeWet = tape(dry, drive * 0.82f) * tapeAmount;
            const float transformerWet = softClip(dry, drive * 0.58f) * transformerAmount;

            float wet = 0.0f;

            switch (mode)
            {
                case 1: wet = tubeWet + tapeWet * 0.25f + transformerWet * 0.25f; break;
                case 2: wet = tubeWet * 0.25f + tapeWet + transformerWet * 0.20f; break;
                case 3: wet = tubeWet * 0.25f + tapeWet * 0.20f + transformerWet; break;
                default: wet = tubeWet + tapeWet + transformerWet; break;
            }

            const float normalise = 1.0f / juce::jmax(0.35f, tubeAmount + tapeAmount + transformerAmount);
            const float coloured = wet * normalise * trim * output;
            data[i] = dry * (1.0f - mix) + coloured * mix;
        }
    }
}
