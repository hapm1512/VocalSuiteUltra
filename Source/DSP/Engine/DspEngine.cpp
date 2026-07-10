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
    lufsMomentary.store(-70.0f);
    lufsShortTerm.store(-70.0f);
    lufsIntegrated.store(-70.0f);
    stereoCorrelation.store(1.0f);
    stereoWidth.store(0.0f);
    outputMeter.reset();
    analyzerFifo.fill(0.0f);
    analyzerWriteIndex.store(0);
    analyzerReady.store(false);
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

    // Gain-staged vocal chain: colour the clean signal first, then shape and control it.
    // This prevents the EQ and dynamics stages from feeding an unnecessarily hot preamp.
    applyPreamp(buffer, apvts);
    applyProfessionalVocalEq(buffer, apvts);
    applyProfessionalDeEsser(buffer, apvts);
    applyCoreCompressor(buffer, apvts);
    applySaturationStage(buffer, apvts);
    applyHiEndExciter(buffer, apvts);
    applyLimiterAndOutput(buffer, apvts);

    sanitize(buffer);
    pushAnalyzerSamples(buffer);
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
float DspEngine::getLufsMomentary() const noexcept { return lufsMomentary.load(); }
float DspEngine::getLufsShortTerm() const noexcept { return lufsShortTerm.load(); }
float DspEngine::getLufsIntegrated() const noexcept { return lufsIntegrated.load(); }
float DspEngine::getStereoCorrelation() const noexcept { return stereoCorrelation.load(); }
float DspEngine::getStereoWidth() const noexcept { return stereoWidth.load(); }

bool DspEngine::copyAnalyzerBuffer(DspEngine::AnalyzerBuffer& destination) const noexcept
{
    if (! analyzerReady.load())
        return false;

    const int write = juce::jlimit(0, analyzerFifoSize - 1, analyzerWriteIndex.load());

    for (int i = 0; i < analyzerFifoSize; ++i)
    {
        const int index = (write + i) & (analyzerFifoSize - 1);
        destination[static_cast<size_t>(i)] = analyzerFifo[static_cast<size_t>(index)];
    }

    return true;
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

float DspEngine::softCeiling(float sample, float ceiling, float softness) noexcept
{
    const float safeCeiling = juce::jlimit(0.05f, 1.0f, ceiling);
    const float safeSoftness = juce::jlimit(0.0f, 1.0f, softness);

    if (std::abs(sample) <= safeCeiling)
        return sample;

    const float sign = sample < 0.0f ? -1.0f : 1.0f;
    const float over = std::abs(sample) - safeCeiling;
    const float knee = 0.025f + safeSoftness * 0.35f;
    const float rounded = safeCeiling + knee * std::tanh(over / knee);
    return sign * juce::jmin(1.0f, rounded);
}

float DspEngine::detectTruePeakProxy(float current, float previous) noexcept
{
    const float mid = 0.5f * (current + previous);
    const float quarterA = previous * 0.75f + current * 0.25f;
    const float quarterB = previous * 0.25f + current * 0.75f;
    return juce::jmax(std::abs(current), std::abs(mid), std::abs(quarterA), std::abs(quarterB));
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

void DspEngine::pushAnalyzerSamples(const juce::AudioBuffer<float>& buffer) noexcept
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    int write = analyzerWriteIndex.load();

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getReadPointer(ch)[i];

        mono /= static_cast<float>(numChannels);
        analyzerFifo[static_cast<size_t>(write)] = juce::jlimit(-1.0f, 1.0f, mono);
        write = (write + 1) & (analyzerFifoSize - 1);
    }

    analyzerWriteIndex.store(write);
    analyzerReady.store(true);
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

    // Lightweight loudness approximation for real-time UI metering.
    // This is intentionally allocation-free and safe for the audio thread.
    const float momentary = juce::jlimit(-70.0f, 6.0f, frame.rmsDb - 3.0f);
    const float shortTerm = lufsShortTerm.load() * 0.985f + momentary * 0.015f;
    const float integrated = lufsIntegrated.load() * 0.998f + momentary * 0.002f;

    lufsMomentary.store(momentary);
    lufsShortTerm.store(juce::jlimit(-70.0f, 6.0f, shortTerm));
    lufsIntegrated.store(juce::jlimit(-70.0f, 6.0f, integrated));

    float correlation = 1.0f;

    if (buffer.getNumChannels() >= 2 && buffer.getNumSamples() > 0)
    {
        const auto* left = buffer.getReadPointer(0);
        const auto* right = buffer.getReadPointer(1);

        double cross = 0.0;
        double leftEnergy = 0.0;
        double rightEnergy = 0.0;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const double l = static_cast<double>(left[i]);
            const double r = static_cast<double>(right[i]);
            cross += l * r;
            leftEnergy += l * l;
            rightEnergy += r * r;
        }

        const double denom = std::sqrt(leftEnergy * rightEnergy);
        if (denom > 1.0e-12)
            correlation = static_cast<float>(cross / denom);
    }

    correlation = juce::jlimit(-1.0f, 1.0f, correlation);
    stereoCorrelation.store(correlation);
    stereoWidth.store(juce::jlimit(0.0f, 200.0f, (1.0f - correlation) * 100.0f));
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
    const bool dynamicOn = getBoolParam(apvts, "DYN_EQ_ON", true);

    if (! surgicalOn && ! toneOn && ! dynamicOn)
        return;

    const float eqMix = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "EQ_MIX", 100.0f) / 100.0f);
    const float makeupDb = getFloatParam(apvts, "TONE_GAIN", 0.0f);

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

    // Commercial EQ protection: prevents high boost from making vocal thin,
    // and prevents low boost from turning boxy/muddy.
    const float highProtect = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "EQ_HIGH_PROTECT", 65.0f) / 100.0f);
    const float lowProtect = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "EQ_LOW_PROTECT", 55.0f) / 100.0f);
    const float tiltDb = getFloatParam(apvts, "EQ_TILT", 0.0f);
    const float analogQ = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "EQ_ANALOG_Q", 60.0f) / 100.0f);
    const float autoGainAmount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "EQ_AUTO_GAIN", 70.0f) / 100.0f);

    const float protectedLowGain = lowGain - juce::jmax(0.0f, lowGain) * lowProtect * 0.30f;
    const float protectedHighGain = highGain - juce::jmax(0.0f, highGain) * highProtect * 0.18f;
    const float protectedAirGain = airGain - juce::jmax(0.0f, airGain) * highProtect * 0.12f;
    const float analogMidQ = juce::jmap(analogQ, 0.70f, 1.25f);

    const float totalBoost = juce::jmax(0.0f, protectedLowGain)
                           + juce::jmax(0.0f, midGain)
                           + juce::jmax(0.0f, protectedHighGain)
                           + juce::jmax(0.0f, protectedAirGain)
                           + juce::jmax(0.0f, tiltDb);

    const float autoGainDb = -0.18f * totalBoost * autoGainAmount;
    const float makeup = juce::Decibels::decibelsToGain(makeupDb + autoGainDb);

    const auto mud = makePeak(mudFreq, mudQ, mudGain, sampleRate);
    const auto harsh = makePeak(harshFreq, harshQ, harshGain, sampleRate);
    const auto notch = makePeak(notchFreq, notchQ, notchGain, sampleRate);
    const auto lowShelf = makeLowShelf(lowFreq, protectedLowGain - (tiltDb * 0.35f), sampleRate);
    const auto midPeak = makePeak(midFreq, analogMidQ, midGain, sampleRate);
    const auto highShelf = makeHighShelf(highFreq, protectedHighGain + (tiltDb * 0.35f), sampleRate);
    const auto airShelf = makeHighShelf(airFreq, protectedAirGain, sampleRate);

    std::array<float, 4> dynFreq
    {
        getFloatParam(apvts, "DYN_EQ1_FREQ", 260.0f),
        getFloatParam(apvts, "DYN_EQ2_FREQ", 520.0f),
        getFloatParam(apvts, "DYN_EQ3_FREQ", 2800.0f),
        getFloatParam(apvts, "DYN_EQ4_FREQ", 6200.0f)
    };

    std::array<float, 4> dynQ
    {
        getFloatParam(apvts, "DYN_EQ1_Q", 1.6f),
        getFloatParam(apvts, "DYN_EQ2_Q", 1.4f),
        getFloatParam(apvts, "DYN_EQ3_Q", 2.2f),
        getFloatParam(apvts, "DYN_EQ4_Q", 2.0f)
    };

    std::array<float, 4> dynThreshold
    {
        getFloatParam(apvts, "DYN_EQ1_THRESH", -38.0f),
        getFloatParam(apvts, "DYN_EQ2_THRESH", -36.0f),
        getFloatParam(apvts, "DYN_EQ3_THRESH", -34.0f),
        getFloatParam(apvts, "DYN_EQ4_THRESH", -36.0f)
    };

    std::array<float, 4> dynAmount
    {
        getFloatParam(apvts, "DYN_EQ1_AMOUNT", 2.2f),
        getFloatParam(apvts, "DYN_EQ2_AMOUNT", 1.8f),
        getFloatParam(apvts, "DYN_EQ3_AMOUNT", 2.4f),
        getFloatParam(apvts, "DYN_EQ4_AMOUNT", 1.6f)
    };

    const float dynAttack = makeCoefficient(getFloatParam(apvts, "DYN_EQ_ATTACK", 8.0f), sampleRate);
    const float dynRelease = makeCoefficient(getFloatParam(apvts, "DYN_EQ_RELEASE", 160.0f), sampleRate);
    const float dynMix = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "DYN_EQ_MIX", 100.0f) / 100.0f);

    std::array<BiquadCoefficients, 4> detector;
    for (size_t band = 0; band < detector.size(); ++band)
        detector[band] = makeBandPass(dynFreq[band], dynQ[band], sampleRate);

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

            if (dynamicOn)
            {
                float dynWet = wet;

                for (size_t band = 0; band < 4; ++band)
                {
                    const float detected = std::abs(state.dynamicDetector[band].process(wet, detector[band]));
                    const float envCoef = detected > state.dynamicEnvelope[band] ? dynAttack : dynRelease;
                    state.dynamicEnvelope[band] = envCoef * state.dynamicEnvelope[band] + (1.0f - envCoef) * detected;

                    const float overDb = juce::jmax(0.0f, safeDb(state.dynamicEnvelope[band]) - dynThreshold[band]);
                    const float reductionDb = -juce::jlimit(0.0f, dynAmount[band], overDb * 0.33f);

                    if (std::abs(reductionDb) > 0.001f)
                    {
                        const auto dynEq = makePeak(dynFreq[band], dynQ[band], reductionDb, sampleRate);
                        dynWet = state.dynamicBell[band].process(dynWet, dynEq);
                    }
                }

                wet = wet * (1.0f - dynMix) + dynWet * dynMix;
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

    const int mode = getChoiceParam(apvts, "DEESSER_MODE", 1); // 0 = wide, 1 = split
    const bool listen = getBoolParam(apvts, "DEESSER_LISTEN", false);
    const float frequency = juce::jlimit(4000.0f, 10000.0f, getFloatParam(apvts, "DEESSER_FREQ", 6500.0f));
    const float thresholdDb = juce::jlimit(-60.0f, 0.0f, getFloatParam(apvts, "DEESSER_THRESH", -28.0f));
    const float rangeDb = juce::jlimit(0.0f, 24.0f, getFloatParam(apvts, "DEESSER_REDUCE", 7.0f));
    const float amount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "DEESSER_AMOUNT", 70.0f) / 100.0f);

    const float attackMs = juce::jlimit(0.1f, 25.0f, getFloatParam(apvts, "DEESSER_ATTACK", 2.5f));
    const float releaseMs = juce::jlimit(15.0f, 250.0f, getFloatParam(apvts, "DEESSER_RELEASE", 85.0f));
    const float attackCoef = makeCoefficient(attackMs, sampleRate);
    const float releaseCoef = makeCoefficient(releaseMs, sampleRate);

    // Wide mode uses a broad band detector. Split mode focuses tightly on sibilance.
    const float q = mode == 0 ? 0.80f : 5.25f;
    const auto sibilanceBand = makeBandPass(frequency, q, sampleRate);

    // Soft knee makes reduction enter gradually and avoids lisp/click artifacts.
    constexpr float softKneeDb = 8.0f;
    float maxReduction = gainReductionDb.load();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float sibilance = state.deEsserBand.process(dry, sibilanceBand);

            // Hybrid peak/RMS detector: catches fast S peaks without overreacting to air.
            const float absSib = std::abs(sibilance);
            const float rmsLike = std::sqrt(0.85f * state.deEsserEnvelope * state.deEsserEnvelope
                                            + 0.15f * absSib * absSib);
            const float detector = juce::jmax(absSib * 0.70f, rmsLike);
            const float envCoef = detector > state.deEsserEnvelope ? attackCoef : releaseCoef;
            state.deEsserEnvelope = envCoef * state.deEsserEnvelope + (1.0f - envCoef) * detector;

            const float detectorDb = safeDb(state.deEsserEnvelope);
            const float overDb = detectorDb - thresholdDb;

            float reductionDb = 0.0f;

            if (overDb > -softKneeDb)
            {
                const float kneePosition = juce::jlimit(0.0f, 1.0f, (overDb + softKneeDb) / softKneeDb);
                const float knee = kneePosition * kneePosition * (3.0f - 2.0f * kneePosition);
                const float rawReduction = juce::jmax(0.0f, overDb) * 0.85f + knee * 0.35f;
                reductionDb = juce::jlimit(0.0f, rangeDb, rawReduction) * amount;
            }

            const float targetGain = juce::Decibels::decibelsToGain(-reductionDb);
            const float gainCoef = targetGain < state.deEsserGain ? attackCoef : releaseCoef;
            state.deEsserGain = gainCoef * state.deEsserGain + (1.0f - gainCoef) * targetGain;

            const float reducedBand = sibilance * state.deEsserGain;

            // Split mode reduces only the sibilance band. Wide mode gently ducks the whole signal.
            const float splitProcessed = dry + (reducedBand - sibilance);
            const float wideProcessed = dry * (0.72f + 0.28f * state.deEsserGain);
            const float processed = mode == 0 ? wideProcessed : splitProcessed;

            // Listen mode outputs only detected sibilance after reduction for tuning.
            data[i] = listen ? reducedBand : processed;
            maxReduction = juce::jmax(maxReduction, reductionDb);
        }
    }

    gainReductionDb.store(maxReduction);
}

void DspEngine::applyPreamp(juce::AudioBuffer<float>& buffer,
                            juce::AudioProcessorValueTreeState& apvts)
{
    if (! getBoolParam(apvts, "PREAMP_ON", true))
        return;

    const float driveAmount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_DRIVE", 20.0f) / 100.0f);
    const float biasAmount = juce::jlimit(-1.0f, 1.0f, getFloatParam(apvts, "PREAMP_BIAS", 0.0f) / 100.0f);
    const float mix = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_MIX", 100.0f) / 100.0f);
    const float requestedOutputDb = getFloatParam(apvts, "PREAMP_OUTPUT", 0.0f);
    const bool safeMode = getBoolParam(apvts, "PREAMP_SAFE", true);
    const float headroomDb = safeMode ? getFloatParam(apvts, "PREAMP_HEADROOM", 12.0f) : 0.0f;
    const float ceilingDb = getFloatParam(apvts, "PREAMP_CEILING", -1.0f);
    const int mode = getChoiceParam(apvts, "PREAMP_MODE", 0);

    const float harmonicAmount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_HARMONICS", 45.0f) / 100.0f);
    const float transformerAmount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_TRANSFORMER", 35.0f) / 100.0f);
    const float colourAmount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_COLOR", 50.0f) / 100.0f);
    const float dynamicsAmount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_DYNAMICS", 45.0f) / 100.0f);
    const float recoveryAmount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_RECOVERY", 55.0f) / 100.0f);
    const float transientSoftness = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_TRANSIENT", 35.0f) / 100.0f);
    const float harmonicBalance = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "PREAMP_BALANCE", 62.0f) / 100.0f);

    const float modeDriveScale = [mode]() noexcept
    {
        switch (mode)
        {
            case 1: return 4.55f; // Metal / pentode emphasis.
            case 2: return 1.18f; // Nature.
            case 3: return 2.80f; // Vintage / tape-like.
            case 4: return 1.04f; // Clean.
            default: return 3.20f; // Tube / triode.
        }
    }();

    const float driveCurve = std::pow(driveAmount, 1.18f);
    const float baseDrive = 1.0f + driveCurve * modeDriveScale;
    const float bias = juce::jmap(biasAmount, -0.18f, 0.18f);

    // Safety and analogue-feel chain:
    // input headroom -> level-dependent drive -> frequency-dependent drive -> tube recovery
    // -> auto loudness compensation -> DC block -> slew limiting -> soft ceiling.
    const float headroomGain = juce::Decibels::decibelsToGain(-juce::jlimit(0.0f, 18.0f, headroomDb));
    const float outputGain = juce::Decibels::decibelsToGain(juce::jlimit(-18.0f, 12.0f, requestedOutputDb));
    const float autoTrimDb = -(driveCurve * (safeMode ? 8.5f : 5.8f))
                           - (harmonicAmount * colourAmount * 1.5f);

    // Recover only part of the input headroom. Fully restoring all 12 dB here made
    // the following compressor and limiter work too hard when Drive was high.
    const float headroomRecoveryDb = safeMode ? headroomDb * 0.70f : headroomDb * 0.85f;
    const float targetMakeup = outputGain
                             * juce::Decibels::decibelsToGain(autoTrimDb + headroomRecoveryDb);
    const float ceiling = juce::Decibels::decibelsToGain(juce::jlimit(-12.0f, -0.3f, ceilingDb));

    const float dcCoefficient = makeOnePoleCoefficient(18.0f, sampleRate);
    const float outputSmoothCoefficient = makeCoefficient(18.0f, sampleRate);
    const float levelAttackCoefficient = makeCoefficient(3.0f, sampleRate);
    const float levelReleaseCoefficient = makeCoefficient(80.0f + recoveryAmount * 220.0f, sampleRate);
    const float tubeGainAttackCoefficient = makeCoefficient(8.0f, sampleRate);
    const float tubeGainReleaseCoefficient = makeCoefficient(90.0f + recoveryAmount * 300.0f, sampleRate);
    const float loudnessCoefficient = makeCoefficient(120.0f, sampleRate);
    const float transformerCoefficient = makeOnePoleCoefficient(155.0f, sampleRate);
    const float lowDriveCoefficient = makeOnePoleCoefficient(180.0f, sampleRate);
    const float highDriveCoefficient = makeOnePoleCoefficient(5400.0f, sampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float protectedInput = juce::jlimit(-2.0f, 2.0f, dry * headroomGain);

            const float inputLevel = std::abs(protectedInput);
            const float levelCoef = inputLevel > state.preampLevelEnvelope ? levelAttackCoefficient : levelReleaseCoefficient;
            state.preampLevelEnvelope = levelCoef * state.preampLevelEnvelope + (1.0f - levelCoef) * inputLevel;

            const float levelDrive = 0.82f + dynamicsAmount * 0.52f * (1.0f - std::exp(-state.preampLevelEnvelope * 3.0f));
            const float dynamicDrive = baseDrive * levelDrive;

            const float lowBand = onePoleLowPass(protectedInput, lowDriveCoefficient, state.preampLowState);
            const float highBand = onePoleHighPassFromLowPass(protectedInput, highDriveCoefficient, state.preampHighState);
            const float midBand = protectedInput - lowBand - highBand;

            const float lowDrive = juce::jmax(1.0f, dynamicDrive * (0.70f + 0.18f * transformerAmount));
            const float midDrive = dynamicDrive * (1.08f + 0.26f * colourAmount);
            const float highDrive = juce::jmax(1.0f, dynamicDrive * (0.76f + 0.12f * harmonicAmount));

            const float transformerLow = onePoleLowPass(protectedInput, transformerCoefficient, state.preampTransformerState);
            const float transformerInput = protectedInput + transformerLow * (0.16f * transformerAmount);

            const float triode = tubeShape(lowBand, lowDrive, bias * 0.65f) * 0.22f
                               + tubeShape(midBand, midDrive, bias) * 0.60f
                               + tubeShape(highBand, highDrive, bias * 0.35f) * 0.18f;

            const float pentode = 0.54f * softClip(midBand, midDrive * 1.72f)
                                + 0.24f * softClip(midBand * 1.90f, midDrive * 0.92f)
                                + 0.14f * softClip(highBand, highDrive * 1.15f)
                                + 0.08f * protectedInput;

            const float vintage = tapeShape(transformerInput, dynamicDrive * 0.92f);
            const float clean = 0.94f * protectedInput + 0.06f * softClip(protectedInput, dynamicDrive * 0.65f);
            const float transformer = transformerShape(transformerInput, dynamicDrive * 0.72f, transformerAmount);

            float wet = triode;

            switch (mode)
            {
                case 1: wet = pentode; break;
                case 2: wet = 0.90f * protectedInput + 0.10f * asymSoftClip(protectedInput, dynamicDrive * 0.72f, bias * 0.18f); break;
                case 3: wet = vintage; break;
                case 4: wet = clean; break;
                default: wet = triode; break;
            }

            const float secondHarm = asymSoftClip(protectedInput, dynamicDrive * 0.64f, bias * 0.58f)
                                   - softClip(protectedInput, dynamicDrive * 0.60f);
            const float thirdHarm = softClip(protectedInput * 1.55f, dynamicDrive * 0.48f) - protectedInput * 0.72f;
            const float fifthHarm = softClip(protectedInput * 2.15f, dynamicDrive * 0.30f) * 0.35f - protectedInput * 0.12f;

            const float evenWeight = 0.58f + harmonicBalance * 0.30f;
            const float oddWeight = 1.0f - evenWeight;
            const float harmonicBlend = (secondHarm * evenWeight
                                      + thirdHarm * oddWeight * 0.72f
                                      + fifthHarm * oddWeight * 0.28f)
                                      * harmonicAmount * colourAmount;

            wet = wet * (1.0f - transformerAmount * 0.22f)
                + transformer * (transformerAmount * 0.22f)
                + harmonicBlend * 0.28f;

            const float overThreshold = juce::jmax(0.0f, std::abs(wet) - (0.20f + 0.10f * (1.0f - driveAmount)));
            const float tubeReductionDb = juce::jlimit(0.0f, 2.4f, overThreshold * (3.0f + dynamicsAmount * 6.0f));
            const float targetTubeGain = juce::Decibels::decibelsToGain(-tubeReductionDb);
            const float tubeGainCoef = targetTubeGain < state.preampTubeGain ? tubeGainAttackCoefficient : tubeGainReleaseCoefficient;
            state.preampTubeGain = tubeGainCoef * state.preampTubeGain + (1.0f - tubeGainCoef) * targetTubeGain;
            wet *= state.preampTubeGain;

            wet = dry * (1.0f - mix) + wet * mix;

            state.preampOutputSmooth = outputSmoothCoefficient * state.preampOutputSmooth
                                     + (1.0f - outputSmoothCoefficient) * targetMakeup;
            wet *= state.preampOutputSmooth;

            state.preampLoudnessIn = loudnessCoefficient * state.preampLoudnessIn
                                   + (1.0f - loudnessCoefficient) * (dry * dry);
            state.preampLoudnessOut = loudnessCoefficient * state.preampLoudnessOut
                                    + (1.0f - loudnessCoefficient) * (wet * wet);

            const float loudnessTarget = std::sqrt((state.preampLoudnessIn + 0.000001f)
                                                 / (state.preampLoudnessOut + 0.000001f));
            const float loudnessTrimTarget = juce::jlimit(0.55f, 1.35f, loudnessTarget);
            state.preampLoudnessTrim = 0.995f * state.preampLoudnessTrim + 0.005f * loudnessTrimTarget;
            wet *= safeMode ? state.preampLoudnessTrim : 1.0f;

            // DC blocker after asymmetric shaping.
            state.preampDcState = dcCoefficient * state.preampDcState + (1.0f - dcCoefficient) * wet;
            wet -= state.preampDcState;

            // Gentle slew-rate control: it removes hard clicks from heavy drive without dulling normal vocal.
            const float maxDelta = 0.18f - transientSoftness * 0.11f;
            const float delta = juce::jlimit(-maxDelta, maxDelta, wet - state.preampSlewState);
            state.preampSlewState += delta;
            wet = state.preampSlewState;

            if (safeMode)
            {
                const float normalised = wet / juce::jmax(0.0001f, ceiling);
                wet = ceiling * std::tanh(normalised);
            }
            else
            {
                wet = juce::jlimit(-1.5f, 1.5f, wet);
            }

            data[i] = juce::jlimit(-1.2f, 1.2f, wet);
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
    const float mix = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "COMP_MIX", 75.0f) / 100.0f);
    const float thresholdDb = getFloatParam(apvts, "COMP_THRESHOLD", -18.0f);
    const int ratioChoice = getChoiceParam(apvts, "COMP_RATIO", 0);
    const bool allButtons = getBoolParam(apvts, "COMP_ALL_BUTTONS", false);
    const bool autoMakeup = getBoolParam(apvts, "COMP_AUTO_MAKEUP", true);

    const float ratio = allButtons ? 20.0f
                                   : (ratioChoice == 1 ? 8.0f
                                      : (ratioChoice == 2 ? 12.0f
                                         : (ratioChoice == 3 ? 20.0f : 4.0f)));

    // 1176 style: attack/release controls are deliberately fast.
    // Higher UI values move toward faster response, but clamped for safety.
    const float attackKnob = juce::jlimit(0.0f, 100.0f, getFloatParam(apvts, "COMP_ATTACK", 12.0f));
    const float releaseKnob = juce::jlimit(0.0f, 100.0f, getFloatParam(apvts, "COMP_RELEASE", 80.0f));

    const float attackTimeMs = juce::jmap(attackKnob, 0.0f, 100.0f, 0.80f, 0.05f);
    const float baseReleaseMs = juce::jmap(releaseKnob, 0.0f, 100.0f, 520.0f, 45.0f);

    const float scHpfHz = juce::jlimit(20.0f, 400.0f, getFloatParam(apvts, "COMP_SC_HPF", 90.0f));
    const float scHpfCoeff = makeOnePoleCoefficient(scHpfHz, sampleRate);
    const float attackCoef = makeCoefficient(attackTimeMs, sampleRate);

    // Makeup is derived from actual gain reduction below. A fixed ratio-based boost
    // caused loudness jumps even when the signal was below threshold.
    float maxReduction = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float driven = dry * inputGain;

            // Sidechain HPF reduces low-frequency pumping without changing audio tone.
            const float detectorSignal = onePoleHighPassFromLowPass(driven, scHpfCoeff, state.compressorDetectorHpState);
            const float rectified = std::abs(detectorSignal);

            const float detectorCoef = rectified > state.compressorEnvelope ? attackCoef : state.compressorProgramRelease;
            state.compressorEnvelope = detectorCoef * state.compressorEnvelope + (1.0f - detectorCoef) * rectified;

            const float levelDb = safeDb(state.compressorEnvelope);
            const float overDb = juce::jmax(0.0f, levelDb - thresholdDb);

            // Soft knee and All Buttons colour. All Buttons has harder knee and more attitude.
            const float kneeDb = allButtons ? 1.5f : 4.0f;
            const float kneeAmount = juce::jlimit(0.0f, 1.0f, overDb / kneeDb);
            float effectiveOverDb = overDb * kneeAmount;

            if (allButtons)
                effectiveOverDb *= 1.18f + juce::jlimit(0.0f, 0.18f, effectiveOverDb * 0.012f);

            const float reductionDb = effectiveOverDb - (effectiveOverDb / ratio);
            const float targetGain = juce::Decibels::decibelsToGain(-reductionDb);

            // Program-dependent release: heavier compression recovers more slowly.
            const float releaseScale = 0.60f + juce::jlimit(0.0f, 1.65f, reductionDb / 10.0f);
            const float programReleaseMs = juce::jlimit(25.0f, 900.0f, baseReleaseMs * releaseScale);
            const float releaseCoef = makeCoefficient(programReleaseMs, sampleRate);
            state.compressorProgramRelease = 0.996f * state.compressorProgramRelease + 0.004f * releaseCoef;

            const float gainCoef = targetGain < state.compressorGain ? attackCoef : state.compressorProgramRelease;
            state.compressorGain = gainCoef * state.compressorGain + (1.0f - gainCoef) * targetGain;

            float compressed = driven * state.compressorGain;

            // Mild FET colour: subtle and level dependent, kept below clipping.
            if (allButtons)
                compressed = std::tanh(compressed * 1.08f) * 0.965f;
            else
                compressed = compressed + 0.012f * (compressed * compressed * compressed);

            const float dynamicMakeupDb = autoMakeup
                ? juce::jlimit(0.0f, 4.0f, reductionDb * 0.32f)
                : 0.0f;
            const float dynamicMakeup = juce::Decibels::decibelsToGain(dynamicMakeupDb);
            compressed *= dynamicMakeup * outputGain;

            const float wet = dry * (1.0f - mix) + compressed * mix;
            data[i] = softCeiling(wet, juce::Decibels::decibelsToGain(-1.5f), 0.22f);
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
            const float blended = dry * (1.0f - mix) + smoothed * mix;
            data[i] = softCeiling(blended, juce::Decibels::decibelsToGain(-1.8f), 0.18f);
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
    const float autoTrim = juce::Decibels::decibelsToGain(-amount * (0.55f + 0.45f * mix));

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
            const float wet = (dry + harmonics + airy) * autoTrim;
            data[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

void DspEngine::applyLimiterAndOutput(juce::AudioBuffer<float>& buffer,
                                      juce::AudioProcessorValueTreeState& apvts)
{
    const bool limiterOn = getBoolParam(apvts, "LIMITER_ON", true);
    const bool ispOn = getBoolParam(apvts, "LIMITER_ISP", true);
    const float outputGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "OUTPUT_GAIN", 0.0f));
    const float driveGain = juce::Decibels::decibelsToGain(getFloatParam(apvts, "LIMITER_DRIVE", 0.0f));
    const float ceilingDb = juce::jlimit(-12.0f, -0.1f, getFloatParam(apvts, "LIMITER_CEILING", -1.0f));
    const float ceiling = juce::Decibels::decibelsToGain(ceilingDb);
    const float releaseMs = juce::jlimit(5.0f, 500.0f, getFloatParam(apvts, "LIMITER_RELEASE", 75.0f));
    const float lookaheadMs = juce::jlimit(0.0f, 5.0f, getFloatParam(apvts, "LIMITER_LOOKAHEAD", 1.5f));
    const float softClipAmount = juce::jlimit(0.0f, 1.0f, getFloatParam(apvts, "LIMITER_SOFTCLIP", 35.0f) / 100.0f);

    const int maxDelay = ChannelState::limiterDelaySize - 1;
    const int lookaheadSamples = juce::jlimit(0, maxDelay, static_cast<int>(sampleRate * lookaheadMs * 0.001));
    const float releaseCoeff = makeCoefficient(releaseMs, sampleRate);
    const float attackCoeff = makeCoefficient(0.05f, sampleRate);
    float maxLimiterReduction = gainReductionDb.load();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = channelStates[static_cast<size_t>(ch)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float input = data[i] * outputGain * driveGain;
            state.limiterDelay[static_cast<size_t>(state.limiterDelayWrite)] = input;

            int readIndex = state.limiterDelayWrite - lookaheadSamples;
            if (readIndex < 0)
                readIndex += ChannelState::limiterDelaySize;

            state.limiterDelayWrite = (state.limiterDelayWrite + 1) & (ChannelState::limiterDelaySize - 1);

            float delayed = lookaheadSamples > 0
                ? state.limiterDelay[static_cast<size_t>(readIndex)]
                : input;

            state.limiterDcState = state.limiterDcState * 0.995f + delayed * 0.005f;
            delayed -= state.limiterDcState;

            float detectorPeak = std::abs(input);
            if (ispOn)
                detectorPeak = juce::jmax(detectorPeak, detectTruePeakProxy(input, state.limiterPrevInput));
            state.limiterPrevInput = input;

            float targetGain = 1.0f;
            if (limiterOn && detectorPeak > ceiling && detectorPeak > 1.0e-6f)
                targetGain = ceiling / detectorPeak;

            const float coef = targetGain < state.limiterGain ? attackCoeff : releaseCoeff;
            state.limiterGain = coef * state.limiterGain + (1.0f - coef) * targetGain;
            state.limiterGain = juce::jlimit(0.02f, 1.0f, state.limiterGain);

            float sample = delayed * state.limiterGain;

            if (limiterOn && softClipAmount > 0.001f)
            {
                const float clipCeiling = ceiling * (1.0f + softClipAmount * 0.18f);
                sample = softCeiling(sample, clipCeiling, softClipAmount);
            }

            if (limiterOn)
                sample = juce::jlimit(-ceiling, ceiling, sample);

            if (! std::isfinite(sample))
                sample = 0.0f;

            const float reductionDb = -juce::Decibels::gainToDecibels(state.limiterGain, -80.0f);
            maxLimiterReduction = juce::jmax(maxLimiterReduction, reductionDb);
            data[i] = sample;
        }
    }

    gainReductionDb.store(maxLimiterReduction);
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
