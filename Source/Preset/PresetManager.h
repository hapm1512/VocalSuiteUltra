#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager final
{
public:
    static constexpr int factoryPresetCount = 6;

    static juce::String getFactoryPresetName(int index);
    static void applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts, int index);

private:
    static void setFloat(juce::AudioProcessorValueTreeState& apvts,
                         const char* parameterId,
                         float value) noexcept;

    static void setBool(juce::AudioProcessorValueTreeState& apvts,
                        const char* parameterId,
                        bool value) noexcept;

    static void setChoice(juce::AudioProcessorValueTreeState& apvts,
                          const char* parameterId,
                          int value) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
