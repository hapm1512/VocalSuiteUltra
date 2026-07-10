#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "LedButton.h"
#include "ProKnob.h"

class ModulePanel final : public juce::Component,
                          private juce::AudioProcessorValueTreeState::Listener,
                          private juce::Timer
{
public:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    ModulePanel();
    ~ModulePanel() override;

    void configure(juce::AudioProcessorValueTreeState& state,
                   const juce::String& newTitle,
                   const juce::StringArray& knobNames,
                   const juce::StringArray& parameterIds,
                   const juce::String& powerParameterId,
                   juce::Colour newAccent,
                   const juce::String& newModeText = {});

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::String title;
    juce::String modeText;
    juce::StringArray labels;
    juce::StringArray sliderParamIds;
    juce::String powerParamId;
    juce::Colour accent { 0xff34d6ff };

    LedButton powerButton { "ON" };
    juce::OwnedArray<ProKnob> knobs;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::unique_ptr<ButtonAttachment> powerAttachment;

    juce::AudioProcessorValueTreeState* parameterState = nullptr;
    std::atomic<bool> graphDirty { true };

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void timerCallback() override;
    void detachParameterListeners();

    bool hasInteractiveGraph() const noexcept;
    float getParameterValue(const juce::String& parameterId, float fallback = 0.0f) const noexcept;
    float getNormalisedParameterValue(const juce::String& parameterId, float fallback = 0.0f) const noexcept;

    void drawMiniDisplay(juce::Graphics& g, juce::Rectangle<int> area);
    void drawNoiseGraph(juce::Graphics& g, juce::Rectangle<float> area);
    void drawGateGraph(juce::Graphics& g, juce::Rectangle<float> area);
    void drawSurgicalEqGraph(juce::Graphics& g, juce::Rectangle<float> area);
    void drawToneEqGraph(juce::Graphics& g, juce::Rectangle<float> area);
    void drawKnobLabels(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePanel)
};
