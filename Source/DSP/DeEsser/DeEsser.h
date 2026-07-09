#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class DeEsser final
{
public:
    void prepare(double newSampleRate, int maxBlockSize, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts);

    float getGainReductionDb() const noexcept { return gainReductionDb; }

private:
    struct ChannelState
    {
        juce::dsp::IIR::Filter<float> band;
        float envelope = 0.0f;
        float gain = 1.0f;
    };

    static float coeff(float timeMs, double sr) noexcept;
    static float safeDb(float value) noexcept;

    void updateFilter(float frequency, float q);

    double sampleRate = 44100.0;
    std::vector<ChannelState> states;
    juce::dsp::ProcessSpec spec {};
    float gainReductionDb = 0.0f;
};
