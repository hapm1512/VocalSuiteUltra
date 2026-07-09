#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"
#include "LevelMeter.h"
#include "LufsMeter.h"
#include "CorrelationMeter.h"
#include "SpectrumAnalyzer.h"

class VocalSuiteUltraProAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VocalSuiteUltraProAudioProcessorEditor(VocalSuiteUltraProAudioProcessor&);
    ~VocalSuiteUltraProAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VocalSuiteUltraProAudioProcessor& processor;

    LevelMeter inputMeter;
    LevelMeter outputMeter;
    LevelMeter levelMeter;
    LufsMeter lufsMeter;
    CorrelationMeter correlationMeter;
    SpectrumAnalyzer spectrumAnalyzer;

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> bodyArea;
    juce::Rectangle<int> footerArea;

    void drawBackground(juce::Graphics&);
    void drawHeader(juce::Graphics&);
    void drawFooter(juce::Graphics&);
    void drawGlassPanel(juce::Graphics&, juce::Rectangle<int>, juce::String title);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalSuiteUltraProAudioProcessorEditor)
};
