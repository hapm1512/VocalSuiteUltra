#include "DspEngine.h"

namespace
{
    float safeDb(float linear) noexcept
    {
        return juce::Decibels::gainToDecibels(juce::jmax(linear, 0.000001f));
    }

    float smootherCoefficient(float timeMs, double sampleRate) noexcept
    {
        const auto safeTime = juce::jmax(0.1f, timeMs) * 0.001f;
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    float softClip(float x, float drive) noexcept
    {
        const auto driven = x * drive;
        const auto normaliser = std::tanh(drive);
        return normaliser > 0.0001f ? std::tanh(driven) / normaliser : x;
    }
}

void DspEngine::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused(samplesPerBlock);

    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
    ensureChannelState(numChannels);
    reset();
}

void DspEngine::reset()
{
    for (auto& state : hpfStates)
        state = {};

    std::fill(gateGain.begin(), gateGain.end(), 1.0f);
    std::fill(compressorGain.begin(), compressorGain.end(), 1.0f);

    inputPeak.store(0.0f);
    outputPeak.store(0.0f);
    inputRms.store(0.0f);
    outputRms.store(0.0f);
    gainReductionDb.store(0.0f);
}

void DspEngine::process(juce::AudioBuffer<float>& buffer,
                        juce::AudioProcessorValueTreeState& apvts)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    ensureChannelState(numChannels);
    sanitize(buffer);
    updateInputMeters(buffer);

    applyInputGain(buffer, apvts);
    applyNoiseAndGate(buffer, apvts);
    applyHighPass(buffer, apvts);
    applyPreamp(buffer, apvts);
    applyCoreCompressor(buffer, apvts);
    applyLimiterAndOutput(buffer, apvts);

    sanitize(buffer);
    updateOutputMeters(buffer);
}

float DspEngine::getInputPeak() const noexcept
{
    return inputPeak.load();
}

float DspEngine::getOutputPeak() const noexcept
{
    return outputPeak.load();
}

float DspEngine::getInputRms() const noexcept
{
    return inputRms.load();
}

float DspEngine::getOutputRms() const noexcept
{
    return outputRms.load();
}

float DspEngine::getGainReductionDb() const noexcept
{
    return gainReductionDb.load();
}

float DspEngine::getFloatParam(juce::AudioProcessorValueTreeState& apvts,
                               const char* parameterId,
                               float fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(parameterId))
        return value->load();

    return fallback;
}

bool DspEngine::getBoolParam(juce::AudioProcessorValueTreeState& apvts,
                             const char* parameterId,
                             bool fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(parameterId))
        return value->load() > 0.5f;

    return fallback;
}

int DspEngine::getChoiceParam(juce::AudioProcessorValueTreeState& apvts,
                              const char* parameterId,
                              int fallback) noexcept
{
    if (const auto* value = apvts.getRawParameterValue(parameterId))
        return static_cast<int>(std::round(value->load()));

    return fallback;
}

void DspEngine::ensureChannelState(int numChannels)
{
    const auto channelCount = static_cast<size_t>(juce::jmax(0, numChannels));

    if (hpfStates.size() != channelCount)
        hpfStates.assign(channelCount, {});

    if (gateGain.size() != channelCount)
        gateGain.assign(channelCount, 1.0f);

    if (compressorGain.size() != channelCount)
        compressorGain.assign(channelCount, 1.0f);
}

void DspEngine::updateInputMeters(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    double squareSum = 0.0;
    int count = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto sample = data[i];
            peak = juce::jmax(peak, std::abs(sample));
            squareSum += static_cast<double>(sample) * static_cast<double>(sample);
            ++count;
        }
    }

    inputPeak.store(juce::jlimit(0.0f, 1.5f, peak));
    inputRms.store(count > 0 ? std::sqrt(static_cast<float>(squareSum / count)) : 0.0f);
}

void DspEngine::updateOutputMeters(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    double squareSum = 0.0;
    int count = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto sample = data[i];
            peak = juce::jmax(peak, std::abs(sample));
            squareSum += static_cast<double>(sample) * static_cast<double>(sample);
            ++count;
        }
    }

    outputPeak.store(juce::jlimit(0.0f, 1.5f, peak));
    outputRms.store(count > 0 ? std::sqrt(static_cast<float>(squareSum / count)) : 0.0f);
}

void DspEngine::applyInputGain(juce::AudioBuffer<float>& buffer,
                               juce::AudioProcessorValueTreeState& apvts) const
{
    const auto gainDb = getFloatParam(apvts, "INPUT_GAIN", 0.0f);
    buffer.applyGain(juce::Decibels::decibelsToGain(gainDb));
}

void DspEngine::applyNoiseAndGate(juce::AudioBuffer<float>& buffer,
                                  juce::AudioProcessorValueTreeState& apvts)
{
    const auto noiseOn = getBoolParam(apvts, "NOISE_ON", true);
    const auto gateOn = getBoolParam(apvts, "GATE_ON", true);

    if (! noiseOn && ! gateOn)
        return;

    const auto noiseAmount = getFloatParam(apvts, "NOISE_REDUCE", 0.0f) / 100.0f;
    const auto thresholdDb = getFloatParam(apvts, "GATE_THRESH", -45.0f);
    const auto rangeDb = getFloatParam(apvts, "GATE_RANGE", 30.0f);
    const auto attack = smootherCoefficient(getFloatParam(apvts, "GATE_ATTACK", 5.0f), sampleRate);
    const auto release = smootherCoefficient(getFloatParam(apvts, "GATE_RELEASE", 120.0f), sampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto currentGain = gateGain[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto levelDb = safeDb(std::abs(data[i]));
            const auto noiseRange = noiseOn ? noiseAmount * 12.0f : 0.0f;
            const auto gateRange = gateOn && levelDb < thresholdDb ? rangeDb : 0.0f;
            const auto totalReduction = juce::jlimit(0.0f, 80.0f, noiseRange + gateRange);
            const auto targetGain = juce::Decibels::decibelsToGain(-totalReduction);
            const auto coefficient = targetGain < currentGain ? attack : release;

            currentGain = coefficient * currentGain + (1.0f - coefficient) * targetGain;
            data[i] *= currentGain;
        }

        gateGain[static_cast<size_t>(ch)] = currentGain;
    }
}

void DspEngine::applyHighPass(juce::AudioBuffer<float>& buffer,
                              juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "HPF_ON", true))
        return;

    const auto frequency = juce::jlimit(20.0f, 220.0f, getFloatParam(apvts, "HPF_FREQ", 85.0f));
    const auto rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * frequency);
    const auto dt = 1.0f / static_cast<float>(sampleRate);
    const auto alpha = rc / (rc + dt);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto& state = hpfStates[static_cast<size_t>(ch)];
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto x = data[i];
            const auto y = alpha * (state.previousOutput + x - state.previousInput);
            state.previousInput = x;
            state.previousOutput = y;
            data[i] = y;
        }
    }
}

void DspEngine::applyPreamp(juce::AudioBuffer<float>& buffer,
                            juce::AudioProcessorValueTreeState& apvts) const
{
    if (! getBoolParam(apvts, "PREAMP_ON", true))
        return;

    const auto drive = getFloatParam(apvts, "PREAMP_DRIVE", 20.0f) / 100.0f;
    const auto output = juce::Decibels::decibelsToGain(getFloatParam(apvts, "PREAMP_OUTPUT", 0.0f));
    const auto mode = getChoiceParam(apvts, "PREAMP_MODE", 0);

    const auto modeDrive = [mode]() noexcept
    {
        switch (mode)
        {
            case 1: return 3.2f; // Metal
            case 2: return 1.35f; // Nature
            case 3: return 2.15f; // Vintage
            default: return 2.45f; // Tube
        }
    }();

    const auto amount = 1.0f + drive * modeDrive;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = softClip(data[i], amount) * output;
    }
}

void DspEngine::applyCoreCompressor(juce::AudioBuffer<float>& buffer,
                                    juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "COMP_ON", true))
        return;

    const auto inputGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "COMP_INPUT", 0.0f));
    const auto outputGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "COMP_OUTPUT", 0.0f));
    const auto mix = getFloatParam(apvts, "COMP_MIX", 75.0f) / 100.0f;
    const auto attack = smootherCoefficient(getFloatParam(apvts, "COMP_ATTACK", 12.0f), sampleRate);
    const auto release = smootherCoefficient(getFloatParam(apvts, "COMP_RELEASE", 80.0f), sampleRate);
    constexpr auto thresholdDb = -18.0f;
    constexpr auto ratio = 4.0f;
    float maxReduction = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto currentGain = compressorGain[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto dry = data[i];
            const auto driven = dry * inputGain;
            const auto levelDb = safeDb(std::abs(driven));
            const auto overDb = juce::jmax(0.0f, levelDb - thresholdDb);
            const auto reductionDb = overDb - (overDb / ratio);
            const auto targetGain = juce::Decibels::decibelsToGain(-reductionDb);
            const auto coefficient = targetGain < currentGain ? attack : release;

            currentGain = coefficient * currentGain + (1.0f - coefficient) * targetGain;
            const auto compressed = driven * currentGain * outputGain;
            data[i] = dry * (1.0f - mix) + compressed * mix;
            maxReduction = juce::jmax(maxReduction, reductionDb);
        }

        compressorGain[static_cast<size_t>(ch)] = currentGain;
    }

    gainReductionDb.store(maxReduction);
}

void DspEngine::applyLimiterAndOutput(juce::AudioBuffer<float>& buffer,
                                      juce::AudioProcessorValueTreeState& apvts)
{
    const auto limiterOn = getBoolParam(apvts, "LIMITER_ON", true);
    const auto outputGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "OUTPUT_GAIN", 0.0f));
    constexpr float ceiling = 0.96f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            auto sample = data[i] * outputGain;

            if (limiterOn)
                sample = juce::jlimit(-ceiling, ceiling, sample);

            data[i] = sample;
        }
    }
}

void DspEngine::sanitize(juce::AudioBuffer<float>& buffer) noexcept
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (! std::isfinite(data[i]))
                data[i] = 0.0f;
        }
    }
}
