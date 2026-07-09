#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class VocalEQ final
{
public:
    void prepare(double newSampleRate, int samplesPerBlock, int numChannels);
    void reset() noexcept;
    void process(juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts) noexcept;

private:
    double sampleRate = 44100.0;
};
