#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class VocalSuiteUltraProAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VocalSuiteUltraProAudioProcessorEditor(VocalSuiteUltraProAudioProcessor&);
    ~VocalSuiteUltraProAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VocalSuiteUltraProAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalSuiteUltraProAudioProcessorEditor)
};

