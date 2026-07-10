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
        addFloat(params, "NOISE_FLOOR", "Noise Floor", -90.0f, -30.0f, -58.0f);
        addFloat(params, "NOISE_ATTACK", "Noise Attack", 1.0f, 80.0f, 12.0f);
        addFloat(params, "NOISE_RELEASE", "Noise Release", 30.0f, 500.0f, 180.0f);

        addBool(params, "PREAMP_ON", "Preamp", true);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "PREAMP_MODE", "Preamp Mode", juce::StringArray { "Tube", "Metal", "Nature", "Vintage", "Clean" }, 0));
        addFloat(params, "PREAMP_DRIVE", "Preamp Drive", 0.0f, 100.0f, 20.0f);
        addFloat(params, "PREAMP_BIAS", "Preamp Bias", -100.0f, 100.0f, 0.0f);
        addFloat(params, "PREAMP_MIX", "Preamp Mix", 0.0f, 100.0f, 100.0f);
        addFloat(params, "PREAMP_OUTPUT", "Preamp Output", -18.0f, 18.0f, 0.0f);
        addBool(params, "PREAMP_SAFE", "Preamp Safe Mode", true);
        addFloat(params, "PREAMP_HEADROOM", "Preamp Headroom", 0.0f, 18.0f, 12.0f);
        addFloat(params, "PREAMP_CEILING", "Preamp Ceiling", -12.0f, -0.3f, -1.0f);
        addFloat(params, "PREAMP_HARMONICS", "Preamp Harmonics", 0.0f, 100.0f, 45.0f);
        addFloat(params, "PREAMP_TRANSFORMER", "Preamp Transformer", 0.0f, 100.0f, 35.0f);
        addFloat(params, "PREAMP_COLOR", "Preamp Color", 0.0f, 100.0f, 50.0f);
        addFloat(params, "PREAMP_DYNAMICS", "Preamp Tube Dynamics", 0.0f, 100.0f, 45.0f);
        addFloat(params, "PREAMP_RECOVERY", "Preamp Recovery", 0.0f, 100.0f, 55.0f);
        addFloat(params, "PREAMP_TRANSIENT", "Preamp Transient Softness", 0.0f, 100.0f, 35.0f);
        addFloat(params, "PREAMP_BALANCE", "Preamp Harmonic Balance", 0.0f, 100.0f, 62.0f);

        addBool(params, "GATE_ON", "Gate", true);
        addFloat(params, "GATE_THRESH", "Gate Threshold", -80.0f, 0.0f, -45.0f);
        addFloat(params, "GATE_RANGE", "Gate Range", 0.0f, 80.0f, 30.0f);
        addFloat(params, "GATE_ATTACK", "Gate Attack", 0.1f, 50.0f, 5.0f);
        addFloat(params, "GATE_RELEASE", "Gate Release", 10.0f, 500.0f, 120.0f);
        addFloat(params, "GATE_HOLD", "Gate Hold", 0.0f, 250.0f, 45.0f);
        addFloat(params, "GATE_HYSTERESIS", "Gate Hysteresis", 0.0f, 12.0f, 4.0f);

        addBool(params, "HPF_ON", "High Pass", true);
        addFloat(params, "HPF_FREQ", "High Pass Frequency", 20.0f, 220.0f, 85.0f);
        addFloat(params, "HPF_SLOPE", "High Pass Slope", 12.0f, 48.0f, 24.0f, 12.0f);

        addBool(params, "EQ_ON", "Surgical EQ", true);
        addFloat(params, "EQ_FREQ", "Surgical EQ Frequency", 120.0f, 6000.0f, 320.0f);
        addFloat(params, "EQ_GAIN", "Surgical EQ Gain", -12.0f, 12.0f, -2.5f);
        addFloat(params, "EQ_Q", "Surgical EQ Q", 0.2f, 12.0f, 4.0f);
        addFloat(params, "EQ_MIX", "Surgical EQ Mix", 0.0f, 100.0f, 100.0f);
        addFloat(params, "EQ_MUD_FREQ", "Mud Frequency", 200.0f, 500.0f, 320.0f);
        addFloat(params, "EQ_MUD_GAIN", "Mud Gain", -12.0f, 3.0f, -2.5f);
        addFloat(params, "EQ_MUD_Q", "Mud Q", 0.4f, 10.0f, 3.5f);
        addFloat(params, "EQ_HARSH_FREQ", "Harsh Frequency", 2000.0f, 4500.0f, 3200.0f);
        addFloat(params, "EQ_HARSH_GAIN", "Harsh Gain", -12.0f, 3.0f, -2.0f);
        addFloat(params, "EQ_HARSH_Q", "Harsh Q", 0.4f, 10.0f, 3.2f);
        addFloat(params, "EQ_NOTCH_FREQ", "Notch Frequency", 800.0f, 7000.0f, 2400.0f);
        addFloat(params, "EQ_NOTCH_GAIN", "Notch Gain", -12.0f, 6.0f, 0.0f);
        addFloat(params, "EQ_NOTCH_Q", "Notch Q", 1.0f, 18.0f, 8.0f);


        addBool(params, "DYN_EQ_ON", "Dynamic EQ", true);
        addFloat(params, "DYN_EQ_MIX", "Dynamic EQ Mix", 0.0f, 100.0f, 100.0f);
        addFloat(params, "DYN_EQ_ATTACK", "Dynamic EQ Attack", 0.5f, 80.0f, 8.0f);
        addFloat(params, "DYN_EQ_RELEASE", "Dynamic EQ Release", 20.0f, 600.0f, 160.0f);
        addFloat(params, "EQ_TILT", "EQ Tilt", -6.0f, 6.0f, 0.0f);
        addFloat(params, "EQ_AUTO_GAIN", "EQ Auto Gain", 0.0f, 100.0f, 70.0f);
        addFloat(params, "EQ_ANALOG_Q", "EQ Analog Q", 0.0f, 100.0f, 60.0f);
        addFloat(params, "EQ_LOW_PROTECT", "EQ Low Protect", 0.0f, 100.0f, 55.0f);
        addFloat(params, "EQ_HIGH_PROTECT", "EQ High Protect", 0.0f, 100.0f, 65.0f);

        addFloat(params, "DYN_EQ1_FREQ", "Dynamic EQ 1 Frequency", 120.0f, 600.0f, 260.0f);
        addFloat(params, "DYN_EQ1_Q", "Dynamic EQ 1 Q", 0.3f, 8.0f, 1.6f);
        addFloat(params, "DYN_EQ1_THRESH", "Dynamic EQ 1 Threshold", -70.0f, -10.0f, -38.0f);
        addFloat(params, "DYN_EQ1_AMOUNT", "Dynamic EQ 1 Amount", 0.0f, 12.0f, 2.2f);

        addFloat(params, "DYN_EQ2_FREQ", "Dynamic EQ 2 Frequency", 250.0f, 1200.0f, 520.0f);
        addFloat(params, "DYN_EQ2_Q", "Dynamic EQ 2 Q", 0.3f, 8.0f, 1.4f);
        addFloat(params, "DYN_EQ2_THRESH", "Dynamic EQ 2 Threshold", -70.0f, -10.0f, -36.0f);
        addFloat(params, "DYN_EQ2_AMOUNT", "Dynamic EQ 2 Amount", 0.0f, 12.0f, 1.8f);

        addFloat(params, "DYN_EQ3_FREQ", "Dynamic EQ 3 Frequency", 1500.0f, 5000.0f, 2800.0f);
        addFloat(params, "DYN_EQ3_Q", "Dynamic EQ 3 Q", 0.3f, 10.0f, 2.2f);
        addFloat(params, "DYN_EQ3_THRESH", "Dynamic EQ 3 Threshold", -70.0f, -10.0f, -34.0f);
        addFloat(params, "DYN_EQ3_AMOUNT", "Dynamic EQ 3 Amount", 0.0f, 12.0f, 2.4f);

        addFloat(params, "DYN_EQ4_FREQ", "Dynamic EQ 4 Frequency", 4000.0f, 10000.0f, 6200.0f);
        addFloat(params, "DYN_EQ4_Q", "Dynamic EQ 4 Q", 0.3f, 10.0f, 2.0f);
        addFloat(params, "DYN_EQ4_THRESH", "Dynamic EQ 4 Threshold", -70.0f, -10.0f, -36.0f);
        addFloat(params, "DYN_EQ4_AMOUNT", "Dynamic EQ 4 Amount", 0.0f, 12.0f, 1.6f);

        addBool(params, "DEESSER_ON", "DeEsser", true);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "DEESSER_MODE", "DeEsser Mode", juce::StringArray { "Wide", "Split" }, 1));
        addFloat(params, "DEESSER_FREQ", "DeEsser Frequency", 4000.0f, 10000.0f, 6500.0f);
        addFloat(params, "DEESSER_THRESH", "DeEsser Threshold", -60.0f, 0.0f, -28.0f);
        addFloat(params, "DEESSER_REDUCE", "DeEsser Reduce", 0.0f, 24.0f, 7.0f);
        addFloat(params, "DEESSER_AMOUNT", "DeEsser Amount", 0.0f, 100.0f, 70.0f);
        addFloat(params, "DEESSER_ATTACK", "DeEsser Attack", 0.1f, 25.0f, 2.5f);
        addFloat(params, "DEESSER_RELEASE", "DeEsser Release", 15.0f, 250.0f, 85.0f);
        addBool(params, "DEESSER_LISTEN", "DeEsser Listen", false);

        addBool(params, "COMP_ON", "Compressor", true);
        addFloat(params, "COMP_THRESHOLD", "Compressor Threshold", -40.0f, 0.0f, -18.0f);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "COMP_RATIO", "Compressor Ratio", juce::StringArray { "4:1", "8:1", "12:1", "20:1" }, 0));
        addFloat(params, "COMP_ATTACK", "Compressor Attack", 0.1f, 50.0f, 12.0f);
        addFloat(params, "COMP_RELEASE", "Compressor Release", 10.0f, 300.0f, 80.0f);
        addFloat(params, "COMP_INPUT", "Compressor Input", -24.0f, 24.0f, 0.0f);
        addFloat(params, "COMP_OUTPUT", "Compressor Output", -24.0f, 24.0f, 0.0f);
        addFloat(params, "COMP_MIX", "Compressor Mix", 0.0f, 100.0f, 75.0f);
        addFloat(params, "COMP_SC_HPF", "Compressor Sidechain HPF", 20.0f, 400.0f, 90.0f);
        addBool(params, "COMP_AUTO_MAKEUP", "Compressor Auto Makeup", true);
        addBool(params, "COMP_ALL_BUTTONS", "Compressor All Buttons", false);

        addBool(params, "TONE_ON", "Tone EQ", true);
        addFloat(params, "TONE_LOW", "Tone Low", -12.0f, 12.0f, 0.0f);
        addFloat(params, "TONE_MID", "Tone Mid", -12.0f, 12.0f, -1.5f);
        addFloat(params, "TONE_HIGH", "Tone High", -12.0f, 12.0f, 1.5f);
        addFloat(params, "TONE_AIR", "Tone Air", -6.0f, 6.0f, 1.2f);
        addFloat(params, "TONE_GAIN", "Tone Gain", -12.0f, 12.0f, 0.0f);
        addFloat(params, "TONE_LOW_FREQ", "Tone Low Frequency", 80.0f, 350.0f, 160.0f);
        addFloat(params, "TONE_MID_FREQ", "Tone Mid Frequency", 350.0f, 2500.0f, 900.0f);
        addFloat(params, "TONE_HIGH_FREQ", "Tone High Frequency", 3500.0f, 12000.0f, 6500.0f);
        addFloat(params, "AIR_FREQ", "Air Frequency", 9000.0f, 20000.0f, 12000.0f);

        addBool(params, "SATURATION_ON", "Saturation", true);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "SATURATION_MODE", "Saturation Mode", juce::StringArray { "Blend", "Tube", "Tape", "Transformer" }, 0));
        addFloat(params, "SATURATION_DRIVE", "Saturation Drive", 0.0f, 100.0f, 18.0f);
        addFloat(params, "SATURATION_MIX", "Saturation Mix", 0.0f, 100.0f, 35.0f);
        addFloat(params, "SATURATION_TUBE", "Tube Harmonics", 0.0f, 100.0f, 55.0f);
        addFloat(params, "SATURATION_TAPE", "Tape Colour", 0.0f, 100.0f, 35.0f);
        addFloat(params, "SATURATION_TRANSFORMER", "Transformer", 0.0f, 100.0f, 25.0f);
        addFloat(params, "SATURATION_BIAS", "Saturation Bias", -100.0f, 100.0f, 8.0f);
        addFloat(params, "SATURATION_OUTPUT", "Saturation Output", -12.0f, 12.0f, 0.0f);

        addBool(params, "HIEND_ON", "Hi-End", true);
        addFloat(params, "HIEND_FREQ", "Hi-End Frequency", 8000.0f, 18000.0f, 12000.0f);
        addFloat(params, "HIEND_AMOUNT", "Hi-End Amount", 0.0f, 100.0f, 22.0f);
        addFloat(params, "HIEND_SPARKLE", "Hi-End Sparkle", 0.0f, 100.0f, 35.0f);
        addFloat(params, "HIEND_AIR", "Hi-End Air", 0.0f, 100.0f, 45.0f);
        addFloat(params, "HIEND_MIX", "Hi-End Mix", 0.0f, 100.0f, 65.0f);

        addBool(params, "MIX_ON", "Mix", true);
        addFloat(params, "MIX_AMOUNT", "Mix Amount", 0.0f, 100.0f, 100.0f);

        addBool(params, "WIDTH_ON", "Width", true);
        addFloat(params, "WIDTH_AMOUNT", "Width Amount", 0.0f, 200.0f, 100.0f);

        addBool(params, "LIMITER_ON", "Limiter", true);
        addFloat(params, "LIMITER_DRIVE", "Limiter Drive", -6.0f, 12.0f, 0.0f);
        addFloat(params, "LIMITER_CEILING", "Limiter Ceiling", -12.0f, -0.1f, -1.0f);
        addFloat(params, "LIMITER_LOOKAHEAD", "Limiter Lookahead", 0.0f, 5.0f, 1.5f);
        addFloat(params, "LIMITER_RELEASE", "Limiter Release", 5.0f, 500.0f, 75.0f);
        addFloat(params, "LIMITER_SOFTCLIP", "Limiter Soft Clip", 0.0f, 100.0f, 35.0f);
        addBool(params, "LIMITER_ISP", "Limiter ISP Protection", true);


        addBool(params, "APP_CPU_SAFE", "CPU Safe Mode", false);
        addBool(params, "APP_ANALYZER_ON", "Analyzer Enabled", true);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "APP_OVERSAMPLING", "Oversampling", juce::StringArray { "Off", "2x", "4x" }, 0));
        addBool(params, "PITCH_ON", "Pitch Correction Hook", false);
        addFloat(params, "PITCH_AMOUNT", "Pitch Correction Amount", 0.0f, 100.0f, 0.0f);
        addFloat(params, "PITCH_SPEED", "Pitch Correction Speed", 0.0f, 100.0f, 45.0f);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "PITCH_KEY", "Pitch Key", juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "PITCH_SCALE", "Pitch Scale", juce::StringArray { "Major", "Minor", "Chromatic" }, 2));

        return { params.begin(), params.end() };
    }
}
