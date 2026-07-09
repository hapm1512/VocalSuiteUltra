#include "FET1176.h"

void FET1176::prepare(double newSampleRate, int, int numChannels)
{
    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
    states.resize(static_cast<size_t>(juce::jmax(1, numChannels)));
    reset();
}

void FET1176::reset() noexcept
{
    for (auto& state : states)
        state = {};
    gainReductionDb = 0.0f;
}

float FET1176::safeDb(float linear) noexcept
{
    return juce::Decibels::gainToDecibels(juce::jmax(1.0e-7f, std::abs(linear)));
}

float FET1176::makeCoefficient(float timeMs, double sr) noexcept
{
    const float timeSeconds = juce::jmax(0.00001f, timeMs * 0.001f);
    return std::exp(-1.0f / (timeSeconds * static_cast<float>(sr)));
}

float FET1176::onePoleCoefficient(float frequency, double sr) noexcept
{
    const float x = -2.0f * juce::MathConstants<float>::pi * frequency / static_cast<float>(sr);
    return std::exp(x);
}

float FET1176::highPass(float input, float coefficient, float& lowState) noexcept
{
    lowState = coefficient * lowState + (1.0f - coefficient) * input;
    return input - lowState;
}

float FET1176::getFloat(juce::AudioProcessorValueTreeState& apvts, const char* id, float fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(id))
        return value->load();
    return fallback;
}

bool FET1176::getBool(juce::AudioProcessorValueTreeState& apvts, const char* id, bool fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(id))
        return value->load() > 0.5f;
    return fallback;
}

int FET1176::getChoice(juce::AudioProcessorValueTreeState& apvts, const char* id, int fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(id))
        return static_cast<int>(std::round(value->load()));
    return fallback;
}

void FET1176::process(juce::AudioBuffer<float>& buffer,
                      juce::AudioProcessorValueTreeState& apvts) noexcept
{
    if (! getBool(apvts, "COMP_ON", true))
        return;

    if (states.size() < static_cast<size_t>(buffer.getNumChannels()))
        states.resize(static_cast<size_t>(buffer.getNumChannels()));

    const float inputGain = juce::Decibels::decibelsToGain(getFloat(apvts, "COMP_INPUT", 0.0f));
    const float outputGain = juce::Decibels::decibelsToGain(getFloat(apvts, "COMP_OUTPUT", 0.0f));
    const float thresholdDb = getFloat(apvts, "COMP_THRESHOLD", -18.0f);
    const float mix = juce::jlimit(0.0f, 1.0f, getFloat(apvts, "COMP_MIX", 75.0f) / 100.0f);
    const bool allButtons = getBool(apvts, "COMP_ALL_BUTTONS", false);
    const int ratioChoice = getChoice(apvts, "COMP_RATIO", 0);
    const float ratio = allButtons ? 20.0f : (ratioChoice == 1 ? 8.0f : (ratioChoice == 2 ? 12.0f : (ratioChoice == 3 ? 20.0f : 4.0f)));

    const float attackMs = juce::jmap(juce::jlimit(0.0f, 100.0f, getFloat(apvts, "COMP_ATTACK", 12.0f)), 0.0f, 100.0f, 0.80f, 0.05f);
    const float releaseMs = juce::jmap(juce::jlimit(0.0f, 100.0f, getFloat(apvts, "COMP_RELEASE", 80.0f)), 0.0f, 100.0f, 520.0f, 45.0f);
    const float attackCoef = makeCoefficient(attackMs, sampleRate);
    const float scCoeff = onePoleCoefficient(juce::jlimit(20.0f, 400.0f, getFloat(apvts, "COMP_SC_HPF", 90.0f)), sampleRate);
    const float makeup = getBool(apvts, "COMP_AUTO_MAKEUP", true) ? juce::Decibels::decibelsToGain(juce::jlimit(0.0f, 7.0f, (ratio - 1.0f) * 0.33f)) : 1.0f;

    float maxReduction = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = states[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float driven = dry * inputGain;
            const float detector = std::abs(highPass(driven, scCoeff, state.detectorLow));
            const float detectorCoef = detector > state.envelope ? attackCoef : state.programRelease;
            state.envelope = detectorCoef * state.envelope + (1.0f - detectorCoef) * detector;

            const float overDb = juce::jmax(0.0f, safeDb(state.envelope) - thresholdDb);
            const float knee = allButtons ? 1.5f : 4.0f;
            float shapedOver = overDb * juce::jlimit(0.0f, 1.0f, overDb / knee);
            if (allButtons)
                shapedOver *= 1.2f;

            const float reduction = shapedOver - shapedOver / ratio;
            const float releaseCoef = makeCoefficient(juce::jlimit(25.0f, 900.0f, releaseMs * (0.6f + reduction / 10.0f)), sampleRate);
            state.programRelease = 0.996f * state.programRelease + 0.004f * releaseCoef;
            const float targetGain = juce::Decibels::decibelsToGain(-reduction);
            const float gainCoef = targetGain < state.gain ? attackCoef : state.programRelease;
            state.gain = gainCoef * state.gain + (1.0f - gainCoef) * targetGain;

            float wet = driven * state.gain * makeup * outputGain;
            if (allButtons)
                wet = std::tanh(wet * 1.08f) * 0.965f;
            data[i] = dry * (1.0f - mix) + wet * mix;
            maxReduction = juce::jmax(maxReduction, reduction);
        }
    }

    gainReductionDb = maxReduction;
}
