#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>

class MeterEngine final
{
public:
    struct Frame
    {
        float peak = 0.0f;
        float rms = 0.0f;
        float truePeak = 0.0f;
        float peakDb = -100.0f;
        float rmsDb = -100.0f;
        float truePeakDb = -100.0f;
        float gainReductionDb = 0.0f;
    };

    void prepare(double newSampleRate, int maxBlockSize, int numChannels);
    void reset() noexcept;

    Frame analyse(const juce::AudioBuffer<float>& buffer, float gainReductionDb) noexcept;

    Frame getLastFrame() const noexcept;
    float getPeak() const noexcept;
    float getRms() const noexcept;
    float getTruePeak() const noexcept;
    float getPeakDb() const noexcept;
    float getRmsDb() const noexcept;
    float getTruePeakDb() const noexcept;
    float getGainReductionDb() const noexcept;

private:
    static float safeDb(float linear) noexcept;
    static float smoothTowards(float current, float target, float attack, float release) noexcept;

    double sampleRate = 44100.0;
    int blockSize = 512;
    int channels = 2;

    float smoothedPeak = 0.0f;
    float smoothedRms = 0.0f;
    float smoothedTruePeak = 0.0f;
    float smoothedGainReduction = 0.0f;

    std::atomic<float> lastPeak { 0.0f };
    std::atomic<float> lastRms { 0.0f };
    std::atomic<float> lastTruePeak { 0.0f };
    std::atomic<float> lastPeakDb { -100.0f };
    std::atomic<float> lastRmsDb { -100.0f };
    std::atomic<float> lastTruePeakDb { -100.0f };
    std::atomic<float> lastGainReductionDb { 0.0f };
};
