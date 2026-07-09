#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>
#include <vector>

class HiEndExciter final
{
public:
    void prepare(double newSampleRate, int maximumBlockSize, int channels);
    void reset();

    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts);

private:
    static float getFloat(juce::AudioProcessorValueTreeState& apvts,
                          const char* id,
                          float fallback) noexcept;

    static bool getBool(juce::AudioProcessorValueTreeState& apvts,
                        const char* id,
                        bool fallback) noexcept;

    static float softClip(float x, float drive) noexcept;
    static float makeOnePoleCoefficient(float frequency, double sampleRate) noexcept;

    double sampleRate = 44100.0;
    std::vector<float> lowPassState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiEndExciter)
};
