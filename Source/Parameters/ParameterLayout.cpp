#include "ParameterLayout.h"

namespace Parameters
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "INPUT_GAIN", "Input Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "OUTPUT_GAIN", "Output Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "NOISE_ON", "Noise Reduction", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "PREAMP_ON", "Preamp", true));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "PREAMP_MODE", "Preamp Mode",
            juce::StringArray { "Tube", "Metal", "Nature", "Vintage" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "PREAMP_DRIVE", "Preamp Drive",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "GATE_ON", "Gate", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "EQ_ON", "EQ", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "DEESSER_ON", "DeEsser", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "COMP_ON", "Compressor", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "SATURATION_ON", "Saturation", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "HIEND_ON", "Hi-End", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "LIMITER_ON", "Limiter", true));

        return { params.begin(), params.end() };
    }
}

