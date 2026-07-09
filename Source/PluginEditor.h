#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"
#include "HeaderBar.h"
#include "FooterBar.h"
#include "GlassPanel.h"
#include "ModulePanel.h"
#include "LevelMeter.h"
#include "LufsMeter.h"
#include "CorrelationMeter.h"
#include "SpectrumAnalyzer.h"

class VocalSuiteUltraProAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit VocalSuiteUltraProAudioProcessorEditor(VocalSuiteUltraProAudioProcessor&);
    ~VocalSuiteUltraProAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    VocalSuiteUltraProAudioProcessor& processor;

    HeaderBar headerBar;
    FooterBar footerBar;

    GlassPanel inputPanel { "INPUT" };
    GlassPanel outputPanel { "OUTPUT" };
    GlassPanel spectrumPanel { "SPECTRUM ANALYZER" };
    GlassPanel levelsPanel { "LEVELS" };
    GlassPanel lufsPanel { "LOUDNESS" };
    GlassPanel correlationPanel { "CORRELATION" };
    GlassPanel presetPanel { "PRESETS" };

    ModulePanel correctionPanel;
    ModulePanel noisePanel;
    ModulePanel preampPanel;
    ModulePanel gatePanel;
    ModulePanel highPassPanel;
    ModulePanel surgicalEqPanel;
    ModulePanel deesserPanel;
    ModulePanel compressorPanel;
    ModulePanel toneEqPanel;
    ModulePanel saturationPanel;
    ModulePanel hiEndPanel;
    ModulePanel mixPanel;
    ModulePanel widthPanel;

    LevelMeter inputMeter;
    LevelMeter outputMeter;
    LevelMeter levelMeter;
    LufsMeter lufsMeter;
    CorrelationMeter correlationMeter;
    SpectrumAnalyzer spectrumAnalyzer;

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> bodyArea;
    juce::Rectangle<int> footerArea;

    void configurePanels();
    void addAllComponents();
    void drawBackground(juce::Graphics&);
    void drawPresetList(juce::Graphics&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalSuiteUltraProAudioProcessorEditor)
};
