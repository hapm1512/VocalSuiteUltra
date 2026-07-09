#include "VocalEQ.h"

void VocalEQ::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused(samplesPerBlock, numChannels);
    sampleRate = juce::jlimit(8000.0, 384000.0, newSampleRate);
}

void VocalEQ::reset() noexcept
{
}

void VocalEQ::process(juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts) noexcept
{
    juce::ignoreUnused(buffer, apvts, sampleRate);
}
