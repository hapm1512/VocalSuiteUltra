#include "DspEngine.h"

void DspEngine::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
    lastBlockSize = juce::jmax(1, samplesPerBlock);
    ensureChannelState(numChannels);
    outputMeter.prepare(sampleRate, lastBlockSize, numChannels);
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
    truePeak.store(0.0f);
    truePeakDb.store(-100.0f);
    outputPeakDb.store(-100.0f);
    outputRmsDb.store(-100.0f);
    outputMeter.reset();
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
    gainReductionDb.store(0.0f);

    applyInputGain(buffer, apvts);
    applyAdaptiveNoiseReduction(buffer, apvts);
    applySoftGate(buffer, apvts);
    applyHighPass(buffer, apvts);
    applyProfessionalVocalEq(buffer, apvts);
    applyProfessionalDeEsser(buffer, apvts);
    applyPreamp(buffer, apvts);
    applyCoreCompressor(buffer, apvts);
    applySaturationStage(buffer, apvts);
    applyHiEndExciter(buffer, apvts);
    applyLimiterAndOutput(buffer, apvts);

    sanitize(buffer);
    updateOutputMeters(buffer);
}

float DspEngine::getInputPeak() const noexcept { return inputPeak.load(); }
float DspEngine::getOutputPeak() const noexcept { return outputPeak.load(); }
float DspEngine::getInputRms() const noexcept { return inputRms.load(); }
float DspEngine::getOutputRms() const noexcept { return outputRms.load(); }
float DspEngine::getGainReductionDb() const noexcept { return gainReductionDb.load(); }
float DspEngine::getTruePeak() const noexcept { return truePeak.load(); }
float DspEngine::getTruePeakDb() const noexcept { return truePeakDb.load(); }
float DspEngine::getOutputPeakDb() const noexcept { return outputPeakDb.load(); }
float DspEngine::getOutputRmsDb() const noexcept { return outputRmsDb.load(); }

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
    const float safeDrive = juce::jlimit(1.0f, 10.0f, drive);
    const float driven = sample * safeDrive;
    const float cubic = driven - (0.16f * driven * driven * driven);
    const float compressed = cubic / (1.0f + std::abs(cubic) * 0.22f);
    return juce::jlimit(-1.2f, 1.2f, compressed / juce::jmax(1.0f, safeDrive * 0.66f));
}

float DspEngine::tubeShape(float sample, float drive, float bias) noexcept
{
    const float safeDrive = juce::jlimit(1.0f, 14.0f, drive);
    const float even = asymSoftClip(sample, safeDrive, juce::jlimit(-0.25f, 0.25f, bias));
    const float odd = softClip(sample, safeDrive * 0.72f);
    return juce::jlimit(-1.25f, 1.25f, even * 0.68f + odd * 0.32f);
}

float DspEngine::transformerShape(float sample, float drive, float lowWeight) noexcept
{
    const float safeDrive = juce::jlimit(1.0f, 8.0f, drive);
    const float shaped = softClip(sample, safeDrive);
    const float lowEmphasis = juce::jlimit(0.0f, 1.0f, lowWeight);
    return juce::jlimit(-1.2f, 1.2f, shaped * (1.0f + lowEmphasis * 0.08f));
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

DspEngine::BiquadCoefficients DspEngine::normalise(float b0, float b1, float b2,
                                                   float a0, float a1, float a2) noexcept
{
    const float safeA0 = std::abs(a0) > 0.000001f ? a0 : 1.0f;
    return { b0 / safeA0, b1 / safeA0, b2 / safeA0, a1 / safeA0, a2 / safeA0 };
}

DspEngine::BiquadCoefficients DspEngine::makeHighPass(float frequency, float q, double sr) noexcept
{
    const float safeSr = static_cast<float>(juce::jlimit(8000.0, 384000.0, sr));
    const float safeFreq = juce::jlimit(10.0f, safeSr * 0.45f, frequency);
    const float safeQ = juce::jlimit(0.25f, 2.0f, q);
    const float omega = juce::MathConstants<float>::twoPi * safeFreq / safeSr;
    const float cosW = std::cos(omega);
    const float sinW = std::sin(omega);
    const float alpha = sinW / (2.0f * safeQ);

    const float b0 = (1.0f + cosW) * 0.5f;
    const float b1 = -(1.0f + cosW);
    const float b2 = (1.0f + cosW) * 0.5f;
    const float a0 = 1.0f + alpha;
    const float a1 = -2.0f * cosW;
    const float a2 = 1.0f - alpha;

    return normalise(b0, b1, b2, a0, a1, a2);
}

DspEngine::BiquadCoefficients DspEngine::makeBandPass(float frequency, float q, double sr) noexcept
{
    const float safeSr = static_cast<float>(juce::jlimit(8000.0, 384000.0, sr));
    const float safeFreq = juce::jlimit(20.0f, safeSr * 0.45f, frequency);
    const float safeQ = juce::jlimit(0.25f, 12.0f, q);
    const float omega = juce::MathConstants<float>::twoPi * safeFreq / safeSr;
    const float cosW = std::cos(omega);
    const float sinW = std::sin(omega);
    const float alpha = sinW / (2.0f * safeQ);

    const float b0 = alpha;
    const float b1 = 0.0f;
    const float b2 = -alpha;
    const float a0 = 1.0f + alpha;
    const float a1 = -2.0f * cosW;
    const float a2 = 1.0f - alpha;

    return normalise(b0, b1, b2, a0, a1, a2);
}

DspEngine::BiquadCoefficients DspEngine::makePeak(float frequency, float q, float gainDb, double sr) noexcept
{
    const float safeSr = static_cast<float>(juce::jlimit(8000.0, 384000.0, sr));
    const float safeFreq = juce::jlimit(10.0f, safeSr * 0.45f, frequency);
    const float safeQ = juce::jlimit(0.15f, 18.0f, q);
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float omega = juce::MathConstants<float>::twoPi * safeFreq / safeSr;
    const float cosW = std::cos(omega);
    const float sinW = std::sin(omega);
    const float alpha = sinW / (2.0f * safeQ);

    const float b0 = 1.0f + alpha * a;
    const float b1 = -2.0f * cosW;
    const float b2 = 1.0f - alpha * a;
    const float a0 = 1.0f + alpha / a;
    const float a1 = -2.0f * cosW;
    const float a2 = 1.0f - alpha / a;

    return normalise(b0, b1, b2, a0, a1, a2);
}

DspEngine::BiquadCoefficients DspEngine::makeLowShelf(float frequency, float gainDb, double sr) noexcept
{
    const float safeSr = static_cast<float>(juce::jlimit(8000.0, 384000.0, sr));
    const float safeFreq = juce::jlimit(10.0f, safeSr * 0.45f, frequency);
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float omega = juce::MathConstants<float>::twoPi * safeFreq / safeSr;
    const float cosW = std::cos(omega);
    const float sinW = std::sin(omega);
    const float sqrtA = std::sqrt(a);
    const float alpha = sinW * std::sqrt(0.5f);
    const float twoSqrtAAlpha = 2.0f * sqrtA * alpha;

    const float b0 = a * ((a + 1.0f) - (a - 1.0f) * cosW + twoSqrtAAlpha);
    const float b1 = 2.0f * a * ((a - 1.0f) - (a + 1.0f) * cosW);
    const float b2 = a * ((a + 1.0f) - (a - 1.0f) * cosW - twoSqrtAAlpha);
    const float a0 = (a + 1.0f) + (a - 1.0f) * cosW + twoSqrtAAlpha;
    const float a1 = -2.0f * ((a - 1.0f) + (a + 1.0f) * cosW);
    const float a2 = (a + 1.0f) + (a - 1.0f) * cosW - twoSqrtAAlpha;

    return normalise(b0, b1, b2, a0, a1, a2);
}

DspEngine::BiquadCoefficients DspEngine::makeHighShelf(float frequency, float gainDb, double sr) noexcept
{
    const float safeSr = static_cast<float>(juce::jlimit(8000.0, 384000.0, sr));
    const float safeFreq = juce::jlimit(10.0f, safeSr * 0.45f, frequency);
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float omega = juce::MathConstants<float>::twoPi * safeFreq / safeSr;
    const float cosW = std::cos(omega);
    const float sinW = std::sin(omega);
    const float sqrtA = std::sqrt(a);
    const float alpha = sinW * std::sqrt(0.5f);
    const float twoSqrtAAlpha = 2.0f * sqrtA * alpha;

    const float b0 = a * ((a + 1.0f) + (a - 1.0f) * cosW + twoSqrtAAlpha);
    const float b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosW);
    const float b2 = a * ((a + 1.0f) + (a - 1.0f) * cosW - twoSqrtAAlpha);
    const float a0 = (a + 1.0f) - (a - 1.0f) * cosW + twoSqrtAAlpha;
    const float a1 = 2.0f * ((a - 1.0f) - (a + 1.0f) * cosW);
    const float a2 = (a + 1.0f) - (a - 1.0f) * cosW - twoSqrtAAlpha;

    return normalise(b0, b1, b2, a0, a1, a2);
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
    const auto frame = outputMeter.analyse(buffer, gainReductionDb.load());

    outputPeak.store(frame.peak);
    outputRms.store(frame.rms);
    gainReductionDb.store(frame.gainReductionDb);
    truePeak.store(frame.truePeak);
    truePeakDb.store(frame.truePeakDb);
    outputPeakDb.store(frame.peakDb);
    outputRmsDb.store(frame.rmsDb);
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
            const float envCoef = absSample > state.noiseEnvelope ? attackCoef : releaseCoef;
            state.noiseEnvelope = envCoef * state.noiseEnvelope + (1.0f - envCoef) * absSample;

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
            const float envCoef = absSample > state.gateEnvelope ? attackCoef : releaseCoef;
            state.gateEnvelope = envCoef * state.gateEnvelope + (1.0f - envCoef) * absSample;

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
    const float slope = getFloatParam(apvts, "HPF_SLOPE", 24.0f);
    const int stages = slope >= 42.0f ? 4 : (slope >= 30.0f ? 3 : (slope >= 18.0f ? 2 : 1));
    const auto hpf = makeHighPass(frequency, 0.7071f, sampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float y = state.hpf1.process(data[i], hpf);

            if (stages > 1)
                y = state.hpf2.process(y, hpf);
            if (stages > 2)
                y = state.hpf3.process(y, hpf);
            if (stages > 3)
                y = state.hpf4.process(y, hpf);

            data[i] = y;
        }
    }
}

void DspEngine::applyProfessionalVocalEq(juce::AudioBuffer<float>& buffer,
                                         juce::AudioProcessorValueTreeState& apvts)
{
    const bool surgicalOn = getBoolParam(apvts, "EQ_ON", true);
    const bool toneOn = getBoolParam(apvts, "TONE_ON", true);

    if (! surgicalOn && ! toneOn)
        return;

    const float eqMix = getFloatParam(apvts, "EQ_MIX", 100.0f) / 100.0f;
    const float makeup = juce::Decibels::decibelsToGain(getFloatParam(apvts, "TONE_GAIN", 0.0f));

    const float mudFreq = getFloatParam(apvts, "EQ_MUD_FREQ", getFloatParam(apvts, "EQ_FREQ", 320.0f));
    const float mudGain = getFloatParam(apvts, "EQ_MUD_GAIN", getFloatParam(apvts, "EQ_GAIN", -2.5f));
    const float mudQ = getFloatParam(apvts, "EQ_MUD_Q", getFloatParam(apvts, "EQ_Q", 4.0f));

    const float harshFreq = getFloatParam(apvts, "EQ_HARSH_FREQ", 3200.0f);
    const float harshGain = getFloatParam(apvts, "EQ_HARSH_GAIN", -2.0f);
    const float harshQ = getFloatParam(apvts, "EQ_HARSH_Q", 3.2f);

    const float notchFreq = getFloatParam(apvts, "EQ_NOTCH_FREQ", 2400.0f);
    const float notchGain = getFloatParam(apvts, "EQ_NOTCH_GAIN", 0.0f);
    const float notchQ = getFloatParam(apvts, "EQ_NOTCH_Q", 8.0f);

    const float lowGain = getFloatParam(apvts, "TONE_LOW", 0.0f);
    const float midGain = getFloatParam(apvts, "TONE_MID", -1.5f);
    const float highGain = getFloatParam(apvts, "TONE_HIGH", 1.5f);
    const float airGain = getFloatParam(apvts, "TONE_AIR", 1.2f);

    const float lowFreq = getFloatParam(apvts, "TONE_LOW_FREQ", 160.0f);
    const float midFreq = getFloatParam(apvts, "TONE_MID_FREQ", 900.0f);
    const float highFreq = getFloatParam(apvts, "TONE_HIGH_FREQ", 6500.0f);
    const float airFreq = getFloatParam(apvts, "AIR_FREQ", 12000.0f);

    const auto mud = makePeak(mudFreq, mudQ, mudGain, sampleRate);
    const auto harsh = makePeak(harshFreq, harshQ, harshGain, sampleRate);
    const auto notch = makePeak(notchFreq, notchQ, notchGain, sampleRate);
    const auto lowShelf = makeLowShelf(lowFreq, lowGain, sampleRate);
    const auto midPeak = makePeak(midFreq, 0.85f, midGain, sampleRate);
    const auto highShelf = makeHighShelf(highFreq, highGain, sampleRate);
    const auto airShelf = makeHighShelf(airFreq, airGain, sampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            float wet = dry;

            if (surgicalOn)
            {
                wet = state.mudCut.process(wet, mud);
                wet = state.harshCut.process(wet, harsh);
                wet = state.surgicalNotch.process(wet, notch);
            }

            if (toneOn)
            {
                wet = state.lowShelf.process(wet, lowShelf);
                wet = state.midPeak.process(wet, midPeak);
                wet = state.highShelf.process(wet, highShelf);
                wet = state.airShelf.process(wet, airShelf);
                wet *= makeup;
            }

            data[i] = dry * (1.0f - eqMix) + wet * eqMix;
        }
    }
}

void DspEngine::applyProfessionalDeEsser(juce::AudioBuffer<float>& buffer,
                                        juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "DEESSER_ON", true))
        return;

    const int mode = getChoiceParam(apvts, "DEESSER_MODE", 1);
    const bool listen = getBoolParam(apvts, "DEESSER_LISTEN", false);
    const float frequency = getFloatParam(apvts, "DEESSER_FREQ", 6500.0f);
    const float thresholdDb = getFloatParam(apvts, "DEESSER_THRESH", -28.0f);
    const float reduceDb = getFloatParam(apvts, "DEESSER_REDUCE", 7.0f);
    const float amount = getFloatParam(apvts, "DEESSER_AMOUNT", 70.0f) / 100.0f;
    const float attackCoef = makeCoefficient(getFloatParam(apvts, "DEESSER_ATTACK", 2.5f), sampleRate);
    const float releaseCoef = makeCoefficient(getFloatParam(apvts, "DEESSER_RELEASE", 85.0f), sampleRate);
    const float q = mode == 0 ? 0.65f : 4.0f;
    const auto band = makeBandPass(frequency, q, sampleRate);
    float maxReduction = gainReductionDb.load();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float sibilance = state.deEsserBand.process(dry, band);
            const float detector = std::abs(sibilance);
            const float envCoef = detector > state.deEsserEnvelope ? attackCoef : releaseCoef;
            state.deEsserEnvelope = envCoef * state.deEsserEnvelope + (1.0f - envCoef) * detector;

            const float overDb = juce::jmax(0.0f, safeDb(state.deEsserEnvelope) - thresholdDb);
            const float reductionDb = juce::jlimit(0.0f, reduceDb, overDb * 0.72f) * amount;
            const float targetGain = juce::Decibels::decibelsToGain(-reductionDb);
            const float gainCoef = targetGain < state.deEsserGain ? attackCoef : releaseCoef;
            state.deEsserGain = gainCoef * state.deEsserGain + (1.0f - gainCoef) * targetGain;

            const float reducedBand = sibilance * state.deEsserGain;
            const float processed = dry + (reducedBand - sibilance);
            data[i] = listen ? sibilance : processed;
            maxReduction = juce::jmax(maxReduction, reductionDb);
        }
    }

    gainReductionDb.store(maxReduction);
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
            case 1: return 4.25f;
            case 2: return 1.20f;
            case 3: return 2.65f;
            case 4: return 1.05f;
            default: return 3.10f;
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
                case 1:
                    wet = 0.72f * softClip(dry, drive * 1.35f) + 0.28f * softClip(dry * 1.7f, drive * 0.8f);
                    break;
                case 2:
                    wet = 0.85f * dry + 0.15f * asymSoftClip(dry, drive, bias * 0.35f);
                    break;
                case 3:
                    wet = tapeShape(dry, drive);
                    break;
                case 4:
                    wet = 0.92f * dry + 0.08f * softClip(dry, drive);
                    break;
                default:
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
    const float thresholdDb = getFloatParam(apvts, "COMP_THRESHOLD", -18.0f);
    const int ratioChoice = getChoiceParam(apvts, "COMP_RATIO", 0);
    const float ratio = ratioChoice == 1 ? 8.0f : (ratioChoice == 2 ? 12.0f : (ratioChoice == 3 ? 20.0f : 4.0f));
    const bool autoMakeup = getBoolParam(apvts, "COMP_AUTO_MAKEUP", true);

    const float attackMs = getFloatParam(apvts, "COMP_ATTACK", 12.0f);
    const float releaseMs = getFloatParam(apvts, "COMP_RELEASE", 80.0f);

    // 1176-style control mapping: higher knob value means faster timing.
    const float attackTimeMs = juce::jlimit(0.08f, 25.0f, attackMs);
    const float releaseTimeMs = juce::jlimit(12.0f, 500.0f, releaseMs);
    const float attackCoef = makeCoefficient(attackTimeMs, sampleRate);
    const float releaseCoef = makeCoefficient(releaseTimeMs, sampleRate);
    const float makeupGain = autoMakeup ? juce::Decibels::decibelsToGain((ratio - 1.0f) * 0.55f) : 1.0f;

    float maxReduction = gainReductionDb.load();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float driven = dry * inputGain;
            const float rectified = std::abs(driven);
            const float detectorCoef = rectified > state.compressorEnvelope ? attackCoef : releaseCoef;
            state.compressorEnvelope = detectorCoef * state.compressorEnvelope + (1.0f - detectorCoef) * rectified;

            const float levelDb = safeDb(state.compressorEnvelope);
            const float overDb = juce::jmax(0.0f, levelDb - thresholdDb);
            const float kneeDb = 3.0f;
            const float kneeAmount = juce::jlimit(0.0f, 1.0f, overDb / kneeDb);
            const float effectiveOverDb = overDb * kneeAmount;
            const float reductionDb = effectiveOverDb - (effectiveOverDb / ratio);
            const float targetGain = juce::Decibels::decibelsToGain(-reductionDb);
            const float gainCoef = targetGain < state.compressorGain ? attackCoef : releaseCoef;

            state.compressorGain = gainCoef * state.compressorGain + (1.0f - gainCoef) * targetGain;

            const float compressed = driven * state.compressorGain * makeupGain * outputGain;
            data[i] = dry * (1.0f - mix) + compressed * mix;
            maxReduction = juce::jmax(maxReduction, reductionDb);
        }
    }

    gainReductionDb.store(maxReduction);
}

void DspEngine::applySaturationStage(juce::AudioBuffer<float>& buffer,
                                     juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "SATURATION_ON", true))
        return;

    const float driveAmount = getFloatParam(apvts, "SATURATION_DRIVE", 18.0f) / 100.0f;
    const float mix = getFloatParam(apvts, "SATURATION_MIX", 35.0f) / 100.0f;
    const int mode = getChoiceParam(apvts, "SATURATION_MODE", 0);
    const float tubeAmount = getFloatParam(apvts, "SATURATION_TUBE", 55.0f) / 100.0f;
    const float tapeAmount = getFloatParam(apvts, "SATURATION_TAPE", 35.0f) / 100.0f;
    const float transformerAmount = getFloatParam(apvts, "SATURATION_TRANSFORMER", 25.0f) / 100.0f;
    const float biasAmount = getFloatParam(apvts, "SATURATION_BIAS", 8.0f) / 100.0f;
    const float outputGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "SATURATION_OUTPUT", 0.0f));

    const float drive = 1.0f + driveAmount * 6.0f;
    const float trim = juce::Decibels::decibelsToGain(-driveAmount * 4.2f);
    const float bias = juce::jlimit(-0.18f, 0.18f, biasAmount * 0.18f);
    const float tapeLowCoeff = makeOnePoleCoefficient(120.0f, sampleRate);
    const float tapeHighCoeff = makeOnePoleCoefficient(8500.0f, sampleRate);
    const float transformerCoeff = makeOnePoleCoefficient(180.0f, sampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float tapeLow = onePoleLowPass(dry, tapeLowCoeff, state.tapeLowState);
            const float tapeHigh = onePoleHighPassFromLowPass(dry, tapeHighCoeff, state.tapeHighState);
            const float transformerLow = onePoleLowPass(dry, transformerCoeff, state.transformerLowState);

            const float tube = tubeShape(dry, drive * 1.12f, bias) * tubeAmount;
            const float tapeInput = dry + tapeLow * (0.10f + tapeAmount * 0.10f) - tapeHigh * (0.06f * tapeAmount);
            const float tape = tapeShape(tapeInput, drive * 0.82f) * tapeAmount;
            const float transformerInput = dry + transformerLow * (0.14f * transformerAmount);
            const float transformer = transformerShape(transformerInput, drive * 0.58f, transformerAmount) * transformerAmount;

            float wet = 0.0f;

            switch (mode)
            {
                case 1: wet = tube + tape * 0.25f + transformer * 0.25f; break;
                case 2: wet = tube * 0.25f + tape + transformer * 0.20f; break;
                case 3: wet = tube * 0.25f + tape * 0.20f + transformer; break;
                default: wet = tube + tape + transformer; break;
            }

            const float normalise = 1.0f / juce::jmax(0.35f, tubeAmount + tapeAmount + transformerAmount);
            const float coloured = wet * normalise * trim * outputGain;
            const float smoothed = coloured * 0.985f + state.saturationMemory * 0.015f;
            state.saturationMemory = smoothed;
            data[i] = dry * (1.0f - mix) + smoothed * mix;
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
    const float sparkleAmount = getFloatParam(apvts, "HIEND_SPARKLE", 35.0f) / 100.0f;
    const float airAmount = getFloatParam(apvts, "HIEND_AIR", 45.0f) / 100.0f;
    const float mix = getFloatParam(apvts, "HIEND_MIX", 65.0f) / 100.0f;
    const float coeff = makeOnePoleCoefficient(frequency, sampleRate);
    const float harmonicGain = amount * (0.18f + sparkleAmount * 0.36f);
    const float airGain = amount * (0.10f + airAmount * 0.24f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float high = onePoleHighPassFromLowPass(dry, coeff, state.hiEndState);
            const float rectified = std::abs(high) * 2.0f - high * 0.35f;
            const float harmonics = softClip(rectified, 2.6f) * harmonicGain;
            const float airy = softClip(high * 2.2f, 1.8f) * airGain;
            const float wet = dry + harmonics + airy;
            data[i] = dry * (1.0f - mix) + wet * mix;
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
