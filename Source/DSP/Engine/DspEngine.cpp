#include "DspEngine.h"

void DspEngine::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
    lastBlockSize = juce::jmax(1, samplesPerBlock);
    ensureChannelState(numChannels);
    reset();
}

void DspEngine::reset()
{
    for (auto& state : channelStates)
        state = {};

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

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    ensureChannelState(numChannels);
    sanitize(buffer);
    updateInputMeters(buffer);

    applyInputGain(buffer, apvts);
    applyAdaptiveNoiseReduction(buffer, apvts);
    applySoftGate(buffer, apvts);
    applyHighPass(buffer, apvts);
    applyPreamp(buffer, apvts);
    applyCoreCompressor(buffer, apvts);
    applyToneEq(buffer, apvts);
    applySaturationStage(buffer, apvts);
    applyHiEndExciter(buffer, apvts);
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

float DspEngine::safeDb(float linear) noexcept
{
    return juce::Decibels::gainToDecibels(juce::jmax(linear, 0.000001f));
}

float DspEngine::makeCoefficient(float timeMs, double sr) noexcept
{
    const float safeTime = juce::jmax(0.1f, timeMs) * 0.001f;
    return std::exp(-1.0f / (safeTime * static_cast<float>(sr)));
}

float DspEngine::onePoleHighPass(float input,
                                 float alpha,
                                 float& previousInput,
                                 float& previousOutput) noexcept
{
    const float output = alpha * (previousOutput + input - previousInput);
    previousInput = input;
    previousOutput = output;
    return output;
}

float DspEngine::softClip(float sample, float drive) noexcept
{
    const float safeDrive = juce::jlimit(1.0f, 12.0f, drive);
    const float normaliser = std::tanh(safeDrive);

    if (normaliser <= 0.0001f)
        return sample;

    return std::tanh(sample * safeDrive) / normaliser;
}


float DspEngine::asymSoftClip(float sample, float drive, float bias) noexcept
{
    const float biased = sample + bias;
    const float shaped = softClip(biased, drive) - softClip(bias, drive);
    return juce::jlimit(-1.2f, 1.2f, shaped);
}

float DspEngine::tapeShape(float sample, float drive) noexcept
{
    const float driven = sample * drive;
    const float cubic = driven - (0.18f * driven * driven * driven);
    return juce::jlimit(-1.2f, 1.2f, cubic / juce::jmax(1.0f, drive * 0.72f));
}

float DspEngine::makeOnePoleCoefficient(float frequency, double sr) noexcept
{
    const float safeFrequency = juce::jlimit(20.0f, 22000.0f, frequency);
    const float safeSampleRate = static_cast<float>(juce::jlimit(8000.0, 384000.0, sr));
    return std::exp(-2.0f * juce::MathConstants<float>::pi * safeFrequency / safeSampleRate);
}

float DspEngine::onePoleLowPass(float input, float coefficient, float& state) noexcept
{
    state = coefficient * state + (1.0f - coefficient) * input;
    return state;
}

float DspEngine::onePoleHighPassFromLowPass(float input, float coefficient, float& state) noexcept
{
    return input - onePoleLowPass(input, coefficient, state);
}

void DspEngine::ensureChannelState(int numChannels)
{
    const auto channelCount = static_cast<size_t>(juce::jmax(0, numChannels));

    if (channelStates.size() != channelCount)
        channelStates.assign(channelCount, {});
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
            const float sample = data[i];
            peak = juce::jmax(peak, std::abs(sample));
            squareSum += static_cast<double>(sample) * static_cast<double>(sample);
            ++count;
        }
    }

    inputPeak.store(juce::jlimit(0.0f, 1.5f, peak));
    inputRms.store(count > 0 ? std::sqrt(static_cast<float>(squareSum / static_cast<double>(count))) : 0.0f);
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
            const float sample = data[i];
            peak = juce::jmax(peak, std::abs(sample));
            squareSum += static_cast<double>(sample) * static_cast<double>(sample);
            ++count;
        }
    }

    outputPeak.store(juce::jlimit(0.0f, 1.5f, peak));
    outputRms.store(count > 0 ? std::sqrt(static_cast<float>(squareSum / static_cast<double>(count))) : 0.0f);
}

void DspEngine::applyInputGain(juce::AudioBuffer<float>& buffer,
                               juce::AudioProcessorValueTreeState& apvts) const
{
    const float gainDb = getFloatParam(apvts, "INPUT_GAIN", 0.0f);
    buffer.applyGain(juce::Decibels::decibelsToGain(gainDb));
}

void DspEngine::applyAdaptiveNoiseReduction(juce::AudioBuffer<float>& buffer,
                                            juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "NOISE_ON", true))
        return;

    const float amount = getFloatParam(apvts, "NOISE_REDUCE", 35.0f) / 100.0f;
    const float detail = getFloatParam(apvts, "NOISE_DETAIL", 55.0f) / 100.0f;
    const float floorDb = getFloatParam(apvts, "NOISE_FLOOR", -58.0f);
    const float attackCoef = makeCoefficient(getFloatParam(apvts, "NOISE_ATTACK", 12.0f), sampleRate);
    const float releaseCoef = makeCoefficient(getFloatParam(apvts, "NOISE_RELEASE", 180.0f), sampleRate);
    const float maxReductionDb = juce::jmap(amount, 0.0f, 1.0f, 0.0f, 24.0f);
    const float softnessDb = juce::jmap(detail, 0.0f, 1.0f, 18.0f, 6.0f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float absSample = std::abs(data[i]);
            const float envTarget = absSample;
            const float envCoef = envTarget > state.noiseEnvelope ? attackCoef : releaseCoef;
            state.noiseEnvelope = envCoef * state.noiseEnvelope + (1.0f - envCoef) * envTarget;

            const float levelDb = safeDb(state.noiseEnvelope);
            const float belowDb = juce::jlimit(0.0f, softnessDb, floorDb - levelDb);
            const float reductionDb = (belowDb / softnessDb) * maxReductionDb;
            const float targetGain = juce::Decibels::decibelsToGain(-reductionDb);
            const float gainCoef = targetGain < state.noiseGain ? attackCoef : releaseCoef;

            state.noiseGain = gainCoef * state.noiseGain + (1.0f - gainCoef) * targetGain;
            data[i] *= state.noiseGain;
        }
    }
}

void DspEngine::applySoftGate(juce::AudioBuffer<float>& buffer,
                              juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "GATE_ON", true))
        return;

    const float thresholdDb = getFloatParam(apvts, "GATE_THRESH", -45.0f);
    const float rangeDb = getFloatParam(apvts, "GATE_RANGE", 30.0f);
    const float attackCoef = makeCoefficient(getFloatParam(apvts, "GATE_ATTACK", 5.0f), sampleRate);
    const float releaseCoef = makeCoefficient(getFloatParam(apvts, "GATE_RELEASE", 120.0f), sampleRate);
    const float hysteresisDb = getFloatParam(apvts, "GATE_HYSTERESIS", 4.0f);
    const float holdMs = getFloatParam(apvts, "GATE_HOLD", 45.0f);
    const int holdSamples = juce::jlimit(0, static_cast<int>(sampleRate),
                                         static_cast<int>((holdMs * 0.001f) * static_cast<float>(sampleRate)));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float absSample = std::abs(data[i]);
            const float envTarget = absSample;
            const float envCoef = envTarget > state.gateEnvelope ? attackCoef : releaseCoef;
            state.gateEnvelope = envCoef * state.gateEnvelope + (1.0f - envCoef) * envTarget;

            const float levelDb = safeDb(state.gateEnvelope);
            const float openDb = thresholdDb + (hysteresisDb * 0.5f);
            const float closeDb = thresholdDb - (hysteresisDb * 0.5f);

            if (levelDb >= openDb)
            {
                state.gateOpen = true;
                state.gateHoldSamples = holdSamples;
            }
            else if (levelDb < closeDb)
            {
                if (state.gateHoldSamples > 0)
                    --state.gateHoldSamples;
                else
                    state.gateOpen = false;
            }

            const float targetGain = state.gateOpen ? 1.0f : juce::Decibels::decibelsToGain(-rangeDb);
            const float gainCoef = targetGain < state.gateGain ? attackCoef : releaseCoef;

            state.gateGain = gainCoef * state.gateGain + (1.0f - gainCoef) * targetGain;
            data[i] *= state.gateGain;
        }
    }
}

void DspEngine::applyHighPass(juce::AudioBuffer<float>& buffer,
                              juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "HPF_ON", true))
        return;

    const float frequency = juce::jlimit(20.0f, 220.0f, getFloatParam(apvts, "HPF_FREQ", 85.0f));
    const float slope = getFloatParam(apvts, "HPF_SLOPE", 12.0f);
    const bool useSecondPole = slope >= 18.0f;
    const float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * frequency);
    const float dt = 1.0f / static_cast<float>(sampleRate);
    const float alpha = rc / (rc + dt);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float y = onePoleHighPass(data[i], alpha, state.hpf1PreviousInput, state.hpf1PreviousOutput);

            if (useSecondPole)
                y = onePoleHighPass(y, alpha, state.hpf2PreviousInput, state.hpf2PreviousOutput);

            data[i] = y;
        }
    }
}

void DspEngine::applyPreamp(juce::AudioBuffer<float>& buffer,
                            juce::AudioProcessorValueTreeState& apvts) const
{
    if (! getBoolParam(apvts, "PREAMP_ON", true))
        return;

    const float driveAmount = getFloatParam(apvts, "PREAMP_DRIVE", 20.0f) / 100.0f;
    const float biasAmount = getFloatParam(apvts, "PREAMP_BIAS", 0.0f) / 100.0f;
    const float mix = getFloatParam(apvts, "PREAMP_MIX", 100.0f) / 100.0f;
    const float output = juce::Decibels::decibelsToGain(getFloatParam(apvts, "PREAMP_OUTPUT", 0.0f));
    const int mode = getChoiceParam(apvts, "PREAMP_MODE", 0);

    const float drive = 1.0f + driveAmount * [mode]() noexcept
    {
        switch (mode)
        {
            case 1: return 4.25f; // Metal
            case 2: return 1.20f; // Nature
            case 3: return 2.65f; // Vintage
            case 4: return 1.05f; // Clean
            default: return 3.10f; // Tube
        }
    }();

    const float bias = juce::jmap(biasAmount, -0.18f, 0.18f);
    const float autoTrim = juce::Decibels::decibelsToGain(-driveAmount * 5.0f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            float wet = dry;

            switch (mode)
            {
                case 1: // Metal: stronger odd harmonics, still bounded.
                    wet = 0.72f * softClip(dry, drive * 1.35f)
                        + 0.28f * softClip(dry * 1.7f, drive * 0.8f);
                    break;

                case 2: // Nature: light transformer-style density.
                    wet = 0.85f * dry + 0.15f * asymSoftClip(dry, drive, bias * 0.35f);
                    break;

                case 3: // Vintage: softer tape-like compression.
                    wet = tapeShape(dry, drive);
                    break;

                case 4: // Clean: subtle peak rounding only.
                    wet = 0.92f * dry + 0.08f * softClip(dry, drive);
                    break;

                default: // Tube: asymmetric 2nd harmonic emphasis.
                    wet = asymSoftClip(dry, drive, bias);
                    break;
            }

            data[i] = ((dry * (1.0f - mix)) + (wet * mix)) * output * autoTrim;
        }
    }
}

void DspEngine::applyCoreCompressor(juce::AudioBuffer<float>& buffer,
                                    juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "COMP_ON", true))
        return;

    const float inputGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "COMP_INPUT", 0.0f));
    const float outputGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "COMP_OUTPUT", 0.0f));
    const float mix = getFloatParam(apvts, "COMP_MIX", 75.0f) / 100.0f;
    const float attackCoef = makeCoefficient(getFloatParam(apvts, "COMP_ATTACK", 12.0f), sampleRate);
    const float releaseCoef = makeCoefficient(getFloatParam(apvts, "COMP_RELEASE", 80.0f), sampleRate);
    constexpr float thresholdDb = -18.0f;
    constexpr float ratio = 4.0f;
    float maxReduction = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float driven = dry * inputGain;
            const float levelDb = safeDb(std::abs(driven));
            const float overDb = juce::jmax(0.0f, levelDb - thresholdDb);
            const float reductionDb = overDb - (overDb / ratio);
            const float targetGain = juce::Decibels::decibelsToGain(-reductionDb);
            const float gainCoef = targetGain < state.compressorGain ? attackCoef : releaseCoef;

            state.compressorGain = gainCoef * state.compressorGain + (1.0f - gainCoef) * targetGain;
            const float compressed = driven * state.compressorGain * outputGain;
            data[i] = dry * (1.0f - mix) + compressed * mix;
            maxReduction = juce::jmax(maxReduction, reductionDb);
        }
    }

    gainReductionDb.store(maxReduction);
}

void DspEngine::applyToneEq(juce::AudioBuffer<float>& buffer,
                            juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "TONE_ON", true))
        return;

    const float lowGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "TONE_LOW", 0.0f));
    const float midGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "TONE_MID", -1.5f));
    const float highGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "TONE_HIGH", 1.5f));
    const float airGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "TONE_AIR", 1.2f));
    const float makeup = juce::Decibels::decibelsToGain(getFloatParam(apvts, "TONE_GAIN", 0.0f));

    const float lowCoeff = makeOnePoleCoefficient(220.0f, sampleRate);
    const float midLowCoeff = makeOnePoleCoefficient(450.0f, sampleRate);
    const float midHighCoeff = makeOnePoleCoefficient(3200.0f, sampleRate);
    const float highCoeff = makeOnePoleCoefficient(8200.0f, sampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float x = data[i];
            const float low = onePoleLowPass(x, lowCoeff, state.toneLowState);
            const float belowMid = onePoleLowPass(x, midLowCoeff, state.toneMidLowState);
            const float belowHigh = onePoleLowPass(x, midHighCoeff, state.toneMidHighState);
            const float high = onePoleHighPassFromLowPass(x, highCoeff, state.toneHighState);
            const float mid = belowHigh - belowMid;
            const float body = x - low - mid - high;
            const float air = high * (airGain - 1.0f) * 0.65f;

            data[i] = ((low * lowGain) + (mid * midGain) + (high * highGain) + body + air) * makeup;
        }
    }
}

void DspEngine::applySaturationStage(juce::AudioBuffer<float>& buffer,
                                     juce::AudioProcessorValueTreeState& apvts) const
{
    if (! getBoolParam(apvts, "SATURATION_ON", true))
        return;

    const float driveAmount = getFloatParam(apvts, "SATURATION_DRIVE", 18.0f) / 100.0f;
    const float mix = getFloatParam(apvts, "SATURATION_MIX", 35.0f) / 100.0f;
    const float drive = 1.0f + driveAmount * 4.0f;
    const float trim = juce::Decibels::decibelsToGain(-driveAmount * 3.0f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float tube = asymSoftClip(dry, drive, 0.035f * driveAmount);
            const float tape = tapeShape(dry, drive * 0.82f);
            const float wet = (tube * 0.62f + tape * 0.38f) * trim;
            data[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

void DspEngine::applyHiEndExciter(juce::AudioBuffer<float>& buffer,
                                  juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "HIEND_ON", true))
        return;

    const float frequency = getFloatParam(apvts, "HIEND_FREQ", 12000.0f);
    const float amount = getFloatParam(apvts, "HIEND_AMOUNT", 22.0f) / 100.0f;
    const float coeff = makeOnePoleCoefficient(frequency, sampleRate);
    const float gain = amount * 0.42f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float high = onePoleHighPassFromLowPass(dry, coeff, state.hiEndState);
            const float sparkle = softClip(high * 2.6f, 2.0f) * gain;
            data[i] = dry + sparkle;
        }
    }
}

void DspEngine::applyLimiterAndOutput(juce::AudioBuffer<float>& buffer,
                                      juce::AudioProcessorValueTreeState& apvts)
{
    const bool limiterOn = getBoolParam(apvts, "LIMITER_ON", true);
    const float outputGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "OUTPUT_GAIN", 0.0f));
    constexpr float ceiling = 0.96f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float sample = data[i] * outputGain;

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
