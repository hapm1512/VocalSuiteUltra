#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <vector>
#include "../Meter/MeterEngine.h"

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
    float getTruePeak() const noexcept;
    float getTruePeakDb() const noexcept;
    float getOutputPeakDb() const noexcept;
    float getOutputRmsDb() const noexcept;

private:
    struct BiquadCoefficients
    {
        float b0 = 1.0f;
        float b1 = 0.0f;
        float b2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
    };

    struct BiquadState
    {
        float z1 = 0.0f;
        float z2 = 0.0f;

        void reset() noexcept
        {
            z1 = 0.0f;
            z2 = 0.0f;
        }

        float process(float input, const BiquadCoefficients& c) noexcept
        {
            const float output = c.b0 * input + z1;
            z1 = c.b1 * input - c.a1 * output + z2;
            z2 = c.b2 * input - c.a2 * output;
            return output;
        }
    };

    struct ChannelState
    {
        float noiseEnvelope = 0.0f;
        float noiseGain = 1.0f;

        float gateEnvelope = 0.0f;
        float gateGain = 1.0f;
        int gateHoldSamples = 0;
        bool gateOpen = true;

        float deEsserEnvelope = 0.0f;
        float deEsserGain = 1.0f;
        float compressorGain = 1.0f;
        float compressorEnvelope = 0.0f;
        float hiEndState = 0.0f;
        float tapeLowState = 0.0f;
        float tapeHighState = 0.0f;
        float transformerLowState = 0.0f;
        float saturationMemory = 0.0f;

        BiquadState hpf1;
        BiquadState hpf2;
        BiquadState hpf3;
        BiquadState hpf4;

        BiquadState mudCut;
        BiquadState harshCut;
        BiquadState surgicalNotch;
        BiquadState lowShelf;
        BiquadState midPeak;
        BiquadState highShelf;
        BiquadState airShelf;
        BiquadState deEsserBand;

        void resetFilters() noexcept
        {
            hpf1.reset();
            hpf2.reset();
            hpf3.reset();
            hpf4.reset();
            mudCut.reset();
            harshCut.reset();
            surgicalNotch.reset();
            lowShelf.reset();
            midPeak.reset();
            highShelf.reset();
            airShelf.reset();
            deEsserBand.reset();
        }
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
    static float softClip(float sample, float drive) noexcept;
    static float asymSoftClip(float sample, float drive, float bias) noexcept;
    static float tapeShape(float sample, float drive) noexcept;
    static float tubeShape(float sample, float drive, float bias) noexcept;
    static float transformerShape(float sample, float drive, float lowWeight) noexcept;
    static float onePoleLowPass(float input, float coefficient, float& state) noexcept;
    static float onePoleHighPassFromLowPass(float input, float coefficient, float& state) noexcept;
    static float makeOnePoleCoefficient(float frequency, double sr) noexcept;
    static void sanitize(juce::AudioBuffer<float>& buffer) noexcept;

    static BiquadCoefficients makeHighPass(float frequency, float q, double sr) noexcept;
    static BiquadCoefficients makeBandPass(float frequency, float q, double sr) noexcept;
    static BiquadCoefficients makePeak(float frequency, float q, float gainDb, double sr) noexcept;
    static BiquadCoefficients makeLowShelf(float frequency, float gainDb, double sr) noexcept;
    static BiquadCoefficients makeHighShelf(float frequency, float gainDb, double sr) noexcept;
    static BiquadCoefficients normalise(float b0, float b1, float b2, float a0, float a1, float a2) noexcept;

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

    void applyProfessionalVocalEq(juce::AudioBuffer<float>& buffer,
                                  juce::AudioProcessorValueTreeState& apvts);

    void applyProfessionalDeEsser(juce::AudioBuffer<float>& buffer,
                                  juce::AudioProcessorValueTreeState& apvts);

    void applyPreamp(juce::AudioBuffer<float>& buffer,
                     juce::AudioProcessorValueTreeState& apvts) const;

    void applyCoreCompressor(juce::AudioBuffer<float>& buffer,
                             juce::AudioProcessorValueTreeState& apvts);

    void applySaturationStage(juce::AudioBuffer<float>& buffer,
                              juce::AudioProcessorValueTreeState& apvts);

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
    std::atomic<float> truePeak { 0.0f };
    std::atomic<float> truePeakDb { -100.0f };
    std::atomic<float> outputPeakDb { -100.0f };
    std::atomic<float> outputRmsDb { -100.0f };

    MeterEngine outputMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspEngine)
};
