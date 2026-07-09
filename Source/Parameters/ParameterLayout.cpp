#include "ParameterLayout.h"

namespace Parameters
{
    namespace
    {
        void addFloat(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                      const juce::String& id,
                      const juce::String& name,
                      float minValue,
                      float maxValue,
                      float defaultValue,
                      float step = 0.1f)
        {
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                id,
                name,
                juce::NormalisableRange<float>(minValue, maxValue, step),
                defaultValue));
        }

        void addBool(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                     const juce::String& id,
                     const juce::String& name,
                     bool defaultValue = true)
        {
            params.push_back(std::make_unique<juce::AudioParameterBool>(id, name, defaultValue));
        }
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        addFloat(params, "INPUT_GAIN", "Input Gain", -24.0f, 24.0f, 0.0f);
        addFloat(params, "OUTPUT_GAIN", "Output Gain", -24.0f, 24.0f, 0.0f);

        addBool(params, "CORRECTION_ON", "Correction", true);
        addFloat(params, "CORRECTION_SPEED", "Correction Speed", 0.0f, 100.0f, 55.0f);
        addFloat(params, "CORRECTION_MIX", "Correction Mix", 0.0f, 100.0f, 75.0f);

        addBool(params, "NOISE_ON", "Noise Reduction", true);
        addFloat(params, "NOISE_REDUCE", "Noise Reduce", 0.0f, 100.0f, 35.0f);
        addFloat(params, "NOISE_DETAIL", "Noise Detail", 0.0f, 100.0f, 55.0f);

        addBool(params, "PREAMP_ON", "Preamp", true);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "PREAMP_MODE", "Preamp Mode", juce::StringArray { "Tube", "Metal", "Nature", "Vintage" }, 0));
        addFloat(params, "PREAMP_DRIVE", "Preamp Drive", 0.0f, 100.0f, 20.0f);
        addFloat(params, "PREAMP_OUTPUT", "Preamp Output", -18.0f, 18.0f, 0.0f);

        addBool(params, "GATE_ON", "Gate", true);
        addFloat(params, "GATE_THRESH", "Gate Threshold", -80.0f, 0.0f, -45.0f);
        addFloat(params, "GATE_RANGE", "Gate Range", 0.0f, 80.0f, 30.0f);
        addFloat(params, "GATE_ATTACK", "Gate Attack", 0.1f, 50.0f, 5.0f);
        addFloat(params, "GATE_RELEASE", "Gate Release", 10.0f, 500.0f, 120.0f);

        addBool(params, "HPF_ON", "High Pass", true);
        addFloat(params, "HPF_FREQ", "High Pass Frequency", 20.0f, 220.0f, 85.0f);
        addFloat(params, "HPF_SLOPE", "High Pass Slope", 6.0f, 24.0f, 12.0f, 1.0f);

        addBool(params, "EQ_ON", "Surgical EQ", true);
        addFloat(params, "EQ_FREQ", "Surgical EQ Frequency", 120.0f, 6000.0f, 320.0f);
        addFloat(params, "EQ_GAIN", "Surgical EQ Gain", -12.0f, 12.0f, -2.5f);
        addFloat(params, "EQ_Q", "Surgical EQ Q", 0.2f, 12.0f, 4.0f);

        addBool(params, "DEESSER_ON", "DeEsser", true);
        addFloat(params, "DEESSER_THRESH", "DeEsser Threshold", -60.0f, 0.0f, -24.0f);
        addFloat(params, "DEESSER_REDUCE", "DeEsser Reduce", 0.0f, 24.0f, 6.0f);

        addBool(params, "COMP_ON", "Compressor", true);
        addFloat(params, "COMP_ATTACK", "Compressor Attack", 0.1f, 50.0f, 12.0f);
        addFloat(params, "COMP_RELEASE", "Compressor Release", 10.0f, 300.0f, 80.0f);
        addFloat(params, "COMP_INPUT", "Compressor Input", -24.0f, 24.0f, 0.0f);
        addFloat(params, "COMP_OUTPUT", "Compressor Output", -24.0f, 24.0f, 0.0f);
        addFloat(params, "COMP_MIX", "Compressor Mix", 0.0f, 100.0f, 75.0f);

        addBool(params, "TONE_ON", "Tone EQ", true);
        addFloat(params, "TONE_LOW", "Tone Low", -12.0f, 12.0f, 0.0f);
        addFloat(params, "TONE_MID", "Tone Mid", -12.0f, 12.0f, -1.5f);
        addFloat(params, "TONE_HIGH", "Tone High", -12.0f, 12.0f, 1.5f);
        addFloat(params, "TONE_AIR", "Tone Air", -6.0f, 6.0f, 1.2f);
        addFloat(params, "TONE_GAIN", "Tone Gain", -12.0f, 12.0f, 0.0f);

        addBool(params, "SATURATION_ON", "Saturation", true);
        addFloat(params, "SATURATION_DRIVE", "Saturation Drive", 0.0f, 100.0f, 18.0f);
        addFloat(params, "SATURATION_MIX", "Saturation Mix", 0.0f, 100.0f, 35.0f);

        addBool(params, "HIEND_ON", "Hi-End", true);
        addFloat(params, "HIEND_FREQ", "Hi-End Frequency", 8000.0f, 18000.0f, 12000.0f);
        addFloat(params, "HIEND_AMOUNT", "Hi-End Amount", 0.0f, 100.0f, 22.0f);

        addBool(params, "MIX_ON", "Mix", true);
        addFloat(params, "MIX_AMOUNT", "Mix Amount", 0.0f, 100.0f, 100.0f);

        addBool(params, "WIDTH_ON", "Width", true);
        addFloat(params, "WIDTH_AMOUNT", "Width Amount", 0.0f, 200.0f, 100.0f);

        addBool(params, "LIMITER_ON", "Limiter", true);

        return { params.begin(), params.end() };
    }
}
