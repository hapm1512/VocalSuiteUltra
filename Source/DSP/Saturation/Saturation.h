#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>

class Saturation final
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

    static int getChoice(juce::AudioProcessorValueTreeState& apvts,
                         const char* id,
                         int fallback) noexcept;

    static float softClip(float x, float drive) noexcept;
    static float tube(float x, float drive, float bias) noexcept;
    static float tape(float x, float drive) noexcept;

    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Saturation)
};
