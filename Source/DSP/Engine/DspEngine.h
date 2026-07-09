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
    struct ChannelState
    {
        float hpf1PreviousInput = 0.0f;
        float hpf1PreviousOutput = 0.0f;
        float hpf2PreviousInput = 0.0f;
        float hpf2PreviousOutput = 0.0f;

        float noiseEnvelope = 0.0f;
        float noiseGain = 1.0f;

        float gateEnvelope = 0.0f;
        float gateGain = 1.0f;
        int gateHoldSamples = 0;
        bool gateOpen = true;

        float compressorGain = 1.0f;

        float toneLowState = 0.0f;
        float toneMidLowState = 0.0f;
        float toneMidHighState = 0.0f;
        float toneHighState = 0.0f;
        float hiEndState = 0.0f;
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

    static float safeDb(float linear) noexcept;
    static float makeCoefficient(float timeMs, double sr) noexcept;
    static float onePoleHighPass(float input,
                                 float alpha,
                                 float& previousInput,
                                 float& previousOutput) noexcept;
    static float softClip(float sample, float drive) noexcept;
    static float asymSoftClip(float sample, float drive, float bias) noexcept;
    static float tapeShape(float sample, float drive) noexcept;
    static float onePoleLowPass(float input, float coefficient, float& state) noexcept;
    static float onePoleHighPassFromLowPass(float input, float coefficient, float& state) noexcept;
    static float makeOnePoleCoefficient(float frequency, double sr) noexcept;
    static void sanitize(juce::AudioBuffer<float>& buffer) noexcept;

    void ensureChannelState(int numChannels);
    void updateInputMeters(const juce::AudioBuffer<float>& buffer);
    void updateOutputMeters(const juce::AudioBuffer<float>& buffer);

    void applyInputGain(juce::AudioBuffer<float>& buffer,
                        juce::AudioProcessorValueTreeState& apvts) const;

    void applyAdaptiveNoiseReduction(juce::AudioBuffer<float>& buffer,
                                     juce::AudioProcessorValueTreeState& apvts);

    void applySoftGate(juce::AudioBuffer<float>& buffer,
                       juce::AudioProcessorValueTreeState& apvts);

    void applyHighPass(juce::AudioBuffer<float>& buffer,
                       juce::AudioProcessorValueTreeState& apvts);

    void applyPreamp(juce::AudioBuffer<float>& buffer,
                     juce::AudioProcessorValueTreeState& apvts) const;

    void applyCoreCompressor(juce::AudioBuffer<float>& buffer,
                             juce::AudioProcessorValueTreeState& apvts);

    void applyToneEq(juce::AudioBuffer<float>& buffer,
                     juce::AudioProcessorValueTreeState& apvts);

    void applySaturationStage(juce::AudioBuffer<float>& buffer,
                              juce::AudioProcessorValueTreeState& apvts) const;

    void applyHiEndExciter(juce::AudioBuffer<float>& buffer,
                           juce::AudioProcessorValueTreeState& apvts);

    void applyLimiterAndOutput(juce::AudioBuffer<float>& buffer,
                               juce::AudioProcessorValueTreeState& apvts);

    double sampleRate = 44100.0;
    int lastBlockSize = 0;
    std::vector<ChannelState> channelStates;

    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> inputRms { 0.0f };
    std::atomic<float> outputRms { 0.0f };
    std::atomic<float> gainReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspEngine)
};
