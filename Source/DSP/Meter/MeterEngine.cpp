#include "MeterEngine.h"

void MeterEngine::prepare(double newSampleRate, int maxBlockSize, int numChannels)
{
    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
    blockSize = juce::jmax(1, maxBlockSize);
    channels = juce::jmax(1, numChannels);
    reset();
}

void MeterEngine::reset() noexcept
{
    smoothedPeak = 0.0f;
    smoothedRms = 0.0f;
    smoothedTruePeak = 0.0f;
    smoothedGainReduction = 0.0f;

    lastPeak.store(0.0f);
    lastRms.store(0.0f);
    lastTruePeak.store(0.0f);
    lastPeakDb.store(-100.0f);
    lastRmsDb.store(-100.0f);
    lastTruePeakDb.store(-100.0f);
    lastGainReductionDb.store(0.0f);
}

MeterEngine::Frame MeterEngine::analyse(const juce::AudioBuffer<float>& buffer,
                                        float gainReductionDb) noexcept
{
    float peak = 0.0f;
    float truePeak = 0.0f;
    double squareSum = 0.0;
    int sampleCount = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        float previous = buffer.getNumSamples() > 0 ? data[0] : 0.0f;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float sample = data[i];
            const float absSample = std::abs(sample);
            peak = juce::jmax(peak, absSample);
            squareSum += static_cast<double>(sample) * static_cast<double>(sample);
            ++sampleCount;

            // Lightweight 4-point inter-sample approximation.
            // This is not a full oversampled ISP meter, but catches common sample-to-sample overs.
            const float d = sample - previous;
            const float p25 = std::abs(previous + d * 0.25f);
            const float p50 = std::abs(previous + d * 0.50f);
            const float p75 = std::abs(previous + d * 0.75f);
            truePeak = juce::jmax(truePeak, juce::jmax(absSample, juce::jmax(p25, juce::jmax(p50, p75))));
            previous = sample;
        }
    }

    const float rms = sampleCount > 0
        ? std::sqrt(static_cast<float>(squareSum / static_cast<double>(sampleCount)))
        : 0.0f;

    const float attack = 0.32f;
    const float release = 0.045f;

    smoothedPeak = smoothTowards(smoothedPeak, peak, attack, release);
    smoothedRms = smoothTowards(smoothedRms, rms, attack * 0.45f, release * 0.35f);
    smoothedTruePeak = smoothTowards(smoothedTruePeak, truePeak, attack, release);
    smoothedGainReduction = smoothTowards(smoothedGainReduction,
                                          juce::jmax(0.0f, gainReductionDb),
                                          0.45f,
                                          0.055f);

    const Frame frame {
        juce::jlimit(0.0f, 2.0f, smoothedPeak),
        juce::jlimit(0.0f, 2.0f, smoothedRms),
        juce::jlimit(0.0f, 2.0f, smoothedTruePeak),
        safeDb(smoothedPeak),
        safeDb(smoothedRms),
        safeDb(smoothedTruePeak),
        smoothedGainReduction
    };

    lastPeak.store(frame.peak);
    lastRms.store(frame.rms);
    lastTruePeak.store(frame.truePeak);
    lastPeakDb.store(frame.peakDb);
    lastRmsDb.store(frame.rmsDb);
    lastTruePeakDb.store(frame.truePeakDb);
    lastGainReductionDb.store(frame.gainReductionDb);

    return frame;
}

MeterEngine::Frame MeterEngine::getLastFrame() const noexcept
{
    return {
        lastPeak.load(),
        lastRms.load(),
        lastTruePeak.load(),
        lastPeakDb.load(),
        lastRmsDb.load(),
        lastTruePeakDb.load(),
        lastGainReductionDb.load()
    };
}

float MeterEngine::getPeak() const noexcept { return lastPeak.load(); }
float MeterEngine::getRms() const noexcept { return lastRms.load(); }
float MeterEngine::getTruePeak() const noexcept { return lastTruePeak.load(); }
float MeterEngine::getPeakDb() const noexcept { return lastPeakDb.load(); }
float MeterEngine::getRmsDb() const noexcept { return lastRmsDb.load(); }
float MeterEngine::getTruePeakDb() const noexcept { return lastTruePeakDb.load(); }
float MeterEngine::getGainReductionDb() const noexcept { return lastGainReductionDb.load(); }

float MeterEngine::safeDb(float linear) noexcept
{
    return juce::Decibels::gainToDecibels(juce::jmax(linear, 0.000001f));
}

float MeterEngine::smoothTowards(float current, float target, float attack, float release) noexcept
{
    const float coefficient = target > current ? attack : release;
    return current + (target - current) * juce::jlimit(0.0f, 1.0f, coefficient);
}
