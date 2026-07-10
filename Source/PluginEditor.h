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
#include "PresetBrowser.h"

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
    juce::TooltipWindow tooltipWindow { this, 700 };

    GlassPanel inputPanel { "INPUT" };
    GlassPanel outputPanel { "OUTPUT" };
    GlassPanel spectrumPanel { "VOCAL EQ" };
    GlassPanel levelsPanel { "LEVELS" };
    GlassPanel lufsPanel { "LOUDNESS" };
    GlassPanel correlationPanel { "STEREO" };
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
    PresetBrowser presetBrowser;

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> bodyArea;
    juce::Rectangle<int> footerArea;

    bool snapshotAActive = true;

    void configurePanels();
    void configureWorkflow();
    void addAllComponents();
    void drawBackground(juce::Graphics&);
    void showSettingsMenu(juce::Component* target);

    void selectSnapshotA();
    void selectSnapshotB();
    void toggleSnapshots();
    void toggleOversampling();
    void toggleGlobalBypass();
    void syncHeaderStates();

    static bool getBoolParameter(juce::AudioProcessorValueTreeState& state,
                                 const juce::String& id,
                                 bool fallback = false);
    static int getChoiceParameterIndex(juce::AudioProcessorValueTreeState& state,
                                       const juce::String& id,
                                       int fallback = 0);
    static void setParameterValue(juce::AudioProcessorValueTreeState& state,
                                  const juce::String& id,
                                  float plainValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalSuiteUltraProAudioProcessorEditor)
};
