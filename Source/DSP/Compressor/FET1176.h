#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

class FET1176 final
{
public:
    void prepare(double newSampleRate, int maxBlockSize, int numChannels);
    void reset() noexcept;

    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts) noexcept;

    float getGainReductionDb() const noexcept { return gainReductionDb; }

private:
    struct ChannelState
    {
        float envelope = 0.0f;
        float gain = 1.0f;
        float detectorLow = 0.0f;
        float programRelease = 1.0f;
    };

    static float safeDb(float linear) noexcept;
    static float makeCoefficient(float timeMs, double sr) noexcept;
    static float onePoleCoefficient(float frequency, double sr) noexcept;
    static float highPass(float input, float coefficient, float& lowState) noexcept;
    static float getFloat(juce::AudioProcessorValueTreeState& apvts, const char* id, float fallback) noexcept;
    static bool getBool(juce::AudioProcessorValueTreeState& apvts, const char* id, bool fallback) noexcept;
    static int getChoice(juce::AudioProcessorValueTreeState& apvts, const char* id, int fallback) noexcept;

    double sampleRate = 44100.0;
    std::vector<ChannelState> states;
    float gainReductionDb = 0.0f;
};
