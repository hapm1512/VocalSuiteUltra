#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <vector>

class DspEngine final
{
public:
    DspEngine() = default;

    void prepare(double newSampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts);

    float getInputPeak() const noexcept;
    float getOutputPeak() const noexcept;
    float getInputRms() const noexcept;
    float getOutputRms() const noexcept;
    float getGainReductionDb() const noexcept;

private:
    struct HpfState
    {
        float previousInput = 0.0f;
        float previousOutput = 0.0f;
    };

    static float getFloatParam(juce::AudioProcessorValueTreeState& apvts,
                               const char* parameterId,
                               float fallback) noexcept;

    static bool getBoolParam(juce::AudioProcessorValueTreeState& apvts,
                             const char* parameterId,
                             bool fallback) noexcept;

    static int getChoiceParam(juce::AudioProcessorValueTreeState& apvts,
                              const char* parameterId,
                              int fallback) noexcept;

    void ensureChannelState(int numChannels);
    void updateInputMeters(const juce::AudioBuffer<float>& buffer);
    void updateOutputMeters(const juce::AudioBuffer<float>& buffer);

    void applyInputGain(juce::AudioBuffer<float>& buffer,
                        juce::AudioProcessorValueTreeState& apvts) const;

    void applyNoiseAndGate(juce::AudioBuffer<float>& buffer,
                           juce::AudioProcessorValueTreeState& apvts);

    void applyHighPass(juce::AudioBuffer<float>& buffer,
                       juce::AudioProcessorValueTreeState& apvts);

    void applyPreamp(juce::AudioBuffer<float>& buffer,
                     juce::AudioProcessorValueTreeState& apvts) const;

    void applyCoreCompressor(juce::AudioBuffer<float>& buffer,
                             juce::AudioProcessorValueTreeState& apvts);

    void applyLimiterAndOutput(juce::AudioBuffer<float>& buffer,
                               juce::AudioProcessorValueTreeState& apvts);

    static void sanitize(juce::AudioBuffer<float>& buffer) noexcept;

    double sampleRate = 44100.0;
    std::vector<HpfState> hpfStates;
    std::vector<float> gateGain;
    std::vector<float> compressorGain;

    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> inputRms { 0.0f };
    std::atomic<float> outputRms { 0.0f };
    std::atomic<float> gainReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspEngine)
};
